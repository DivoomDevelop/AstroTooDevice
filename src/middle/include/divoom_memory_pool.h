#ifndef _divoom_memory_pool_h_
#define _divoom_memory_pool_h_

/*********************************************************************
  Function:        divoom_memory_pool_init
  Description:     内存池初始化
  Input:           total_size   总大小
                   block_size   内存块的大小
  Return:          none
**********************************************************************/

void divoom_memory_pool_init(int total_size, int block_size, int max_total_size, int max_block_size);


/*********************************************************************
  Function:        divoom_memory_pool_malloc
  Description:     内存池分配一个内存，如果内存池没有可用的，就使用系统分配内存
  Input:           
  Return:          内存指针
**********************************************************************/


void *divoom_memory_pool_malloc(int size);

/*********************************************************************
  Function:        divoom_memory_pool_free
  Description:     内存池释放一个内存
  Input:           ptr
  Return:          none
**********************************************************************/


int divoom_memory_pool_free(void *ptr);

/*********************************************************************
  Function:        divoom_memory_pool_check_ptr
  Description:     检查这个指针是否在内存池中
  Input:           ptr
  Return:          0 ir 1
**********************************************************************/


int divoom_memory_pool_check_ptr(void *ptr);

#endif



#include "divoom.h"

#define DIVOOM_MEMORY_POOL_32_INDEX         (0)
#define DIVOOM_MEMORY_POOL_80_INDEX         (1)
#define DIVOOM_MEMORY_POOL_104_INDEX        (2)
#define DIVOOM_MEMORY_POOL_160_INDEX        (3)
#define DIVOOM_MEMORY_POOL_204_212_INDEX    (4)
#define DIVOOM_MEMORY_POOL_NUM              (5)

typedef struct _divoom_memory_pool_item
{
    int total_size;
    int block_num;
    int bit_map_num;
    int block_size;
    int max_use;
    int cur_use;
    uint32 *bit_map;
    uint8 *pool_buf;
}divoom_memory_pool_item, *Pdivoom_memory_pool_item;

typedef struct _divoom_memory_pool_ctrl
{
    divoom_memory_pool_item ram_pool_info[DIVOOM_MEMORY_POOL_NUM];
    void *lock;
}divoom_memory_pool_ctrl, *Pdivoom_memory_pool_ctrl;

Pdivoom_memory_pool_ctrl pdivoom_memory_pool_handle = NULL;

/* 初始化一个固定块内存池；单个扩展池失败时保留系统堆回退能力。 */
static int divoom_memory_pool_item_init(Pdivoom_memory_pool_item pool_info, int total_size, int block_size)
{
    pool_info->total_size = total_size;
    pool_info->block_size = block_size;
    pool_info->block_num = total_size / block_size;
    pool_info->bit_map_num = (pool_info->block_num + 31) / 32;
    pool_info->pool_buf = malloc(total_size);
    if(pool_info->pool_buf == NULL)
    {
        memset(pool_info, 0, sizeof(divoom_memory_pool_item));
        return 0;
    }
    pool_info->bit_map = malloc(pool_info->bit_map_num * sizeof(uint32));
    if(pool_info->bit_map == NULL)
    {
        free(pool_info->pool_buf);
        memset(pool_info, 0, sizeof(divoom_memory_pool_item));
        return 0;
    }
    memset(pool_info->pool_buf, 0, total_size);
    memset(pool_info->bit_map, 0, pool_info->bit_map_num * sizeof(uint32));
    return 1;
}

/* 扩展池只接收日志确认的精确尺寸，避免其他短期对象占满专用池。 */
static Pdivoom_memory_pool_item divoom_memory_pool_select_item(int size)
{
    if(size <= pdivoom_memory_pool_handle->ram_pool_info[DIVOOM_MEMORY_POOL_32_INDEX].block_size)
    {
        return &pdivoom_memory_pool_handle->ram_pool_info[DIVOOM_MEMORY_POOL_32_INDEX];
    }
    if(size <= pdivoom_memory_pool_handle->ram_pool_info[DIVOOM_MEMORY_POOL_80_INDEX].block_size)
    {
        return &pdivoom_memory_pool_handle->ram_pool_info[DIVOOM_MEMORY_POOL_80_INDEX];
    }
#if DIVOOM_RAM_POOL_104_ENABLE
    if(size == 104)
    {
        return &pdivoom_memory_pool_handle->ram_pool_info[DIVOOM_MEMORY_POOL_104_INDEX];
    }
#endif
#if DIVOOM_RAM_POOL_160_ENABLE
    if(size == 160)
    {
        return &pdivoom_memory_pool_handle->ram_pool_info[DIVOOM_MEMORY_POOL_160_INDEX];
    }
#endif
#if DIVOOM_RAM_POOL_204_212_ENABLE
    if((size == 204) || (size == 212))
    {
        return &pdivoom_memory_pool_handle->ram_pool_info[DIVOOM_MEMORY_POOL_204_212_INDEX];
    }
#endif
    return NULL;
}

/*********************************************************************
  Function:        divoom_memory_pool_init
  Description:     初始化原有通用池和三类高频固定尺寸池
  Input:           min/max total_size 总大小，min/max block_size 内存块大小
  Return:          none
**********************************************************************/
void divoom_memory_pool_init(int min_total_size, int min_block_size, int max_total_size, int max_block_size)
{
    if(pdivoom_memory_pool_handle)
    {
        return;
    }
    pdivoom_memory_pool_handle = malloc(sizeof(divoom_memory_pool_ctrl));
    if(pdivoom_memory_pool_handle == NULL)
    {
        return;
    }
    memset(pdivoom_memory_pool_handle, 0, sizeof(divoom_memory_pool_ctrl));
    divoom_memory_pool_item_init(&pdivoom_memory_pool_handle->ram_pool_info[DIVOOM_MEMORY_POOL_32_INDEX], min_total_size, min_block_size);
    divoom_memory_pool_item_init(&pdivoom_memory_pool_handle->ram_pool_info[DIVOOM_MEMORY_POOL_80_INDEX], max_total_size, max_block_size);

#if DIVOOM_RAM_POOL_104_ENABLE
    divoom_memory_pool_item_init(&pdivoom_memory_pool_handle->ram_pool_info[DIVOOM_MEMORY_POOL_104_INDEX],
        DIVOOM_RAM_POOL_104_BLOCK_NUM * DIVOOM_RAM_POOL_104_BLOCK_SIZE,
        DIVOOM_RAM_POOL_104_BLOCK_SIZE);
#endif
#if DIVOOM_RAM_POOL_160_ENABLE
    divoom_memory_pool_item_init(&pdivoom_memory_pool_handle->ram_pool_info[DIVOOM_MEMORY_POOL_160_INDEX],
        DIVOOM_RAM_POOL_160_BLOCK_NUM * DIVOOM_RAM_POOL_160_BLOCK_SIZE,
        DIVOOM_RAM_POOL_160_BLOCK_SIZE);
#endif
#if DIVOOM_RAM_POOL_204_212_ENABLE
    divoom_memory_pool_item_init(&pdivoom_memory_pool_handle->ram_pool_info[DIVOOM_MEMORY_POOL_204_212_INDEX],
        DIVOOM_RAM_POOL_204_212_BLOCK_NUM * DIVOOM_RAM_POOL_204_212_BLOCK_SIZE,
        DIVOOM_RAM_POOL_204_212_BLOCK_SIZE);
#endif

    pdivoom_memory_pool_handle->lock = malloc(sizeof(pthread_mutex_t));
    memset(pdivoom_memory_pool_handle->lock, 0, sizeof(pthread_mutex_t));
    pthread_mutex_init(pdivoom_memory_pool_handle->lock, NULL);
}

/*********************************************************************
  Function:        divoom_memory_pool_malloc
  Description:     从匹配的固定块池分配内存；池满或尺寸不匹配时由调用方回退系统堆
  Input:           size 申请字节数
  Return:          内存指针，无法从池分配时返回 NULL
**********************************************************************/
void *divoom_memory_pool_malloc(int size)
{
    Pdivoom_memory_pool_item ram_pool_info;

    if(pdivoom_memory_pool_handle == NULL)
    {
        return NULL;
    }
    ram_pool_info = divoom_memory_pool_select_item(size);
    if((ram_pool_info == NULL) || (ram_pool_info->pool_buf == NULL))
    {
        return NULL;
    }

    divoom_system_get_lock_direct(pdivoom_memory_pool_handle->lock);
    for(int i = 0; i < ram_pool_info->bit_map_num; i++)
    {
        if(ram_pool_info->bit_map[i] != U32_MAX)
        {
            for(int j = 0; j < 32; j++)
            {
                if(0 == (ram_pool_info->bit_map[i] & ((uint32)1 << j)))
                {
                    int ram_id = i * 32 + j;

                    if(ram_id >= ram_pool_info->block_num)
                    {
                        divoom_system_free_lock_direct(pdivoom_memory_pool_handle->lock);
                        return NULL;
                    }
                    else
                    {
                        void *buf_ptr;
                        ram_pool_info->cur_use++;
                        if(ram_pool_info->cur_use > ram_pool_info->max_use)
                        {
                            ram_pool_info->max_use = ram_pool_info->cur_use;
                        }
                        ram_pool_info->bit_map[i] |= ((uint32)1 << j);
                        buf_ptr = (void *)&ram_pool_info->pool_buf[ram_id * ram_pool_info->block_size];
                        memset(buf_ptr, 0, ram_pool_info->block_size);
                        divoom_system_free_lock_direct(pdivoom_memory_pool_handle->lock);
                        return buf_ptr;
                    }
                }
            }
        }
    }
    divoom_system_free_lock_direct(pdivoom_memory_pool_handle->lock);
    return NULL;
}

void divoom_memory_pool_info(void)
{
    if(pdivoom_memory_pool_handle == NULL)
    {
        return;
    }
    divoom_print_err("divoom memory poll: %d, %d %d, %d; 104:%d,%d 160:%d,%d 204/212:%d,%d\n",
        pdivoom_memory_pool_handle->ram_pool_info[DIVOOM_MEMORY_POOL_32_INDEX].cur_use,
        pdivoom_memory_pool_handle->ram_pool_info[DIVOOM_MEMORY_POOL_32_INDEX].max_use,
        pdivoom_memory_pool_handle->ram_pool_info[DIVOOM_MEMORY_POOL_80_INDEX].cur_use,
        pdivoom_memory_pool_handle->ram_pool_info[DIVOOM_MEMORY_POOL_80_INDEX].max_use,
        pdivoom_memory_pool_handle->ram_pool_info[DIVOOM_MEMORY_POOL_104_INDEX].cur_use,
        pdivoom_memory_pool_handle->ram_pool_info[DIVOOM_MEMORY_POOL_104_INDEX].max_use,
        pdivoom_memory_pool_handle->ram_pool_info[DIVOOM_MEMORY_POOL_160_INDEX].cur_use,
        pdivoom_memory_pool_handle->ram_pool_info[DIVOOM_MEMORY_POOL_160_INDEX].max_use,
        pdivoom_memory_pool_handle->ram_pool_info[DIVOOM_MEMORY_POOL_204_212_INDEX].cur_use,
        pdivoom_memory_pool_handle->ram_pool_info[DIVOOM_MEMORY_POOL_204_212_INDEX].max_use);
}

/*********************************************************************
  Function:        divoom_memory_pool_check_ptr
  Description:     检查指针是否属于任一内存池
  Input:           ptr 待检查指针
  Return:          所属池的物理块大小，不属于内存池时返回 0
**********************************************************************/
int divoom_memory_pool_check_ptr(void *ptr)
{
    if(pdivoom_memory_pool_handle == NULL)
    {
        return 0;
    }
    for(int i = 0; i < DIVOOM_MEMORY_POOL_NUM; i++)
    {
        Pdivoom_memory_pool_item ram_pool_info = &pdivoom_memory_pool_handle->ram_pool_info[i];

        if((ram_pool_info->pool_buf != NULL) &&
            ((uint8 *)ptr >= ram_pool_info->pool_buf) &&
            ((uint8 *)ptr < (ram_pool_info->pool_buf + ram_pool_info->total_size)))
        {
            return ram_pool_info->block_size;
        }
    }
    return 0;
}

/*********************************************************************
  Function:        divoom_memory_pool_free
  Description:     释放任一内存池中的固定块
  Input:           ptr 待释放指针
  Return:          属于内存池并完成释放返回 1，否则返回 0
**********************************************************************/
int divoom_memory_pool_free(void *ptr)
{
    if(pdivoom_memory_pool_handle == NULL)
    {
        return 0;
    }
    for(int i = 0; i < DIVOOM_MEMORY_POOL_NUM; i++)
    {
        Pdivoom_memory_pool_item ram_pool_info = &pdivoom_memory_pool_handle->ram_pool_info[i];

        if((ram_pool_info->pool_buf != NULL) &&
            ((uint8 *)ptr >= ram_pool_info->pool_buf) &&
            ((uint8 *)ptr < (ram_pool_info->pool_buf + ram_pool_info->total_size)))
        {
            int ram_id = (int)(((uint8 *)ptr - ram_pool_info->pool_buf) / ram_pool_info->block_size);

            divoom_system_get_lock_direct(pdivoom_memory_pool_handle->lock);
            ram_pool_info->cur_use--;
            ram_pool_info->bit_map[ram_id / 32] &= ~((uint32)1 << (ram_id % 32));
            divoom_system_free_lock_direct(pdivoom_memory_pool_handle->lock);
            return 1;
        }
    }
    return 0;
}
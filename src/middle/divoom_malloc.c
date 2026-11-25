#include "divoom.h"

#if defined(DIVOOM_MEMORY_LEAK_DIAG_ENABLE) && DIVOOM_MEMORY_LEAK_DIAG_ENABLE
#include <stdint.h>
#include "divoom_build_config.h"

/* 运行时泄漏追踪保留正常内存池；记录器使用底层 malloc，避免递归进入自身。
 * 哈希桶只用于索引，记录节点动态分配；所有节点和开关都由同一静态锁保护。
 * 从启动起记录全部经过本模块的分配；stop 只停止新增记录，释放仍会注销。 */
#define DIVOOM_MALLOC_TRACE_BUCKETS 256U

typedef struct divoom_malloc_trace_item
{
    void *ptr;
    const char *file;
    uint32 size;
    uint32 line;
    uint32 baseline_size;
    uint32 blocks; /* 快照聚合用，真实节点恒为 1。 */
    uint8 baseline;
    struct divoom_malloc_trace_item *next;
} divoom_malloc_trace_item;

static struct
{
    divoom_malloc_trace_item *buckets[DIVOOM_MALLOC_TRACE_BUCKETS];
    uint32 dropped;
    uint8 enabled;
    uint8 baseline_valid;
    uint8 coverage_lost; /* 曾暂停就无法声称启动以来的记录完整。 */
} divoom_malloc_trace_ctrl = { { NULL }, 0, 1, 0 };
/* Windows shim 的 PTHREAD_MUTEX_INITIALIZER 是未初始化的 CRITICAL_SECTION，
 * 这里使用可静态初始化的 SRWLOCK；真机使用标准 pthread 静态锁。 */
#if defined(_WIN32)
static SRWLOCK divoom_malloc_trace_lock = SRWLOCK_INIT;
#else
static pthread_mutex_t divoom_malloc_trace_lock = PTHREAD_MUTEX_INITIALIZER;
#endif

static void divoom_malloc_trace_get_lock(void)
{
#if defined(_WIN32)
    AcquireSRWLockExclusive(&divoom_malloc_trace_lock);
#else
    pthread_mutex_lock(&divoom_malloc_trace_lock);
#endif
}

static void divoom_malloc_trace_free_lock(void)
{
#if defined(_WIN32)
    ReleaseSRWLockExclusive(&divoom_malloc_trace_lock);
#else
    pthread_mutex_unlock(&divoom_malloc_trace_lock);
#endif
}

static uint32 divoom_malloc_trace_hash(void *ptr)
{
    return (uint32)(((uintptr_t)ptr >> 3) % DIVOOM_MALLOC_TRACE_BUCKETS);
}

/* 调用者持锁；底层 free 之前摘除，避免地址被其它线程复用后误删新记录。 */
static divoom_malloc_trace_item *divoom_malloc_trace_take(void *ptr)
{
    divoom_malloc_trace_item **link = &divoom_malloc_trace_ctrl.buckets[divoom_malloc_trace_hash(ptr)];
    while (*link != NULL)
    {
        if ((*link)->ptr == ptr)
        {
            divoom_malloc_trace_item *item = *link;
            *link = item->next;
            return item;
        }
        link = &(*link)->next;
    }
    return NULL;
}

static void divoom_malloc_trace_add(void *ptr, uint32 size, const char *file, uint32 line)
{
    divoom_malloc_trace_item *item;
    uint32 bucket;
    if (ptr == NULL)
    {
        return;
    }
    divoom_malloc_trace_get_lock();
    if (divoom_malloc_trace_ctrl.enabled)
    {
        item = malloc(sizeof(*item));
        if (item != NULL)
        {
            bucket = divoom_malloc_trace_hash(ptr);
            item->ptr = ptr;
            item->size = size;
            item->blocks = 1;
            item->file = file;
            item->line = line;
            item->baseline = 0;
            item->baseline_size = 0;
            item->next = divoom_malloc_trace_ctrl.buckets[bucket];
            divoom_malloc_trace_ctrl.buckets[bucket] = item;
        }
        else
        {
            /* 丢记录后只报告 INCOMPLETE，不把空差集当成无泄漏。 */
            divoom_malloc_trace_ctrl.dropped++;
        }
    }
    divoom_malloc_trace_free_lock();
}

static void divoom_malloc_trace_remove(void *ptr)
{
    divoom_malloc_trace_get_lock();
    free(divoom_malloc_trace_take(ptr));
    divoom_malloc_trace_free_lock();
}

/* 内存池扩容走申请/复制/释放；迁移旧记录的基线，避免误报为新泄漏。
 * 暂停期间仍迁移已有记录，保持 stop 之后的释放可追溯。 */
static void divoom_malloc_trace_move(void *old_ptr, void *new_ptr, uint32 size, const char *file, uint32 line)
{
    divoom_malloc_trace_item *old_item, *new_item;
    uint32 bucket = divoom_malloc_trace_hash(new_ptr);
    divoom_malloc_trace_get_lock();
    old_item = divoom_malloc_trace_take(old_ptr);
    if (old_item != NULL)
    {
        new_item = divoom_malloc_trace_take(new_ptr);
        free(new_item);
        old_item->ptr = new_ptr;
        old_item->size = size;
        old_item->file = file;
        old_item->line = line;
        old_item->next = divoom_malloc_trace_ctrl.buckets[bucket];
        divoom_malloc_trace_ctrl.buckets[bucket] = old_item;
    }
    divoom_malloc_trace_free_lock();
}

/* 正数 realloc 失败保留原记录及原指针；成功时保持基线归属并更新申请位置。
 * 持锁覆盖 realloc 和记录迁移，避免旧地址复用窗口造成假泄漏。 */
static void *divoom_malloc_trace_realloc(void *ptr, uint32 size, const char *file, uint32 line)
{
    void *new_ptr;
    divoom_malloc_trace_item *item;
    uint32 bucket;
    divoom_malloc_trace_get_lock();
    bucket = divoom_malloc_trace_hash(ptr);
    item = divoom_malloc_trace_take(ptr);
    new_ptr = realloc(ptr, size);
    if (new_ptr == NULL && item != NULL)
    {
        item->next = divoom_malloc_trace_ctrl.buckets[bucket];
        divoom_malloc_trace_ctrl.buckets[bucket] = item;
    }
    if (new_ptr != NULL)
    {
        if (item == NULL && divoom_malloc_trace_ctrl.enabled)
        {
            item = malloc(sizeof(*item));
            if (item != NULL)
            {
                item->baseline = 0;
                item->baseline_size = 0;
            }
            else
            {
                divoom_malloc_trace_ctrl.dropped++;
            }
        }
        if (item != NULL)
        {
            bucket = divoom_malloc_trace_hash(new_ptr);
            item->ptr = new_ptr;
            item->size = size;
            item->blocks = 1;
            item->file = file;
            item->line = line;
            item->next = divoom_malloc_trace_ctrl.buckets[bucket];
            divoom_malloc_trace_ctrl.buckets[bucket] = item;
        }
    }
    divoom_malloc_trace_free_lock();
    return new_ptr;
}

/* 先按申请位置合并，再按存活块数、字节数降序，优先看到高频未释放位置。 */
static int divoom_malloc_trace_site_compare(const void *left, const void *right)
{
    const divoom_malloc_trace_item *a = left, *b = right;
    int ret = strcmp(a->file, b->file);
    if (ret != 0)
    {
        return ret;
    }
    return (a->line > b->line) - (a->line < b->line);
}

static int divoom_malloc_trace_count_compare(const void *left, const void *right)
{
    const divoom_malloc_trace_item *a = left, *b = right;
    if (a->blocks != b->blocks)
    {
        return (a->blocks < b->blocks) - (a->blocks > b->blocks);
    }
    return (a->size < b->size) - (a->size > b->size);
}

/* 复制后解锁再输出串口，避免串口 I/O 持锁阻塞全部分配线程。
 * diff 为跨线程新增存活对象，不自动判为泄漏；需回到同一表盘等待异步退出后比较。
 * begin 覆盖上一基线；快照缓冲、节点开销单列，不计入业务字节数。 */
static void divoom_malloc_trace_report(uint8 begin, uint8 diff, uint8 detail)
{
    divoom_malloc_trace_item *item, *snapshot;
    uint32 bucket, count = 0, selected = 0, bytes = 0, dropped, sites = 0;
    uint8 valid, enabled, coverage_lost;
    divoom_malloc_trace_get_lock();
    for (bucket = 0; bucket < DIVOOM_MALLOC_TRACE_BUCKETS; bucket++)
    {
        for (item = divoom_malloc_trace_ctrl.buckets[bucket]; item != NULL; item = item->next)
        {
            count++;
            bytes += item->size;
            if (begin)
            {
                item->baseline = 1;
                item->baseline_size = item->size;
            }
        }
    }
    if (begin)
    {
        divoom_malloc_trace_ctrl.baseline_valid = divoom_malloc_trace_ctrl.enabled;
    }
    dropped = divoom_malloc_trace_ctrl.dropped;
    valid = divoom_malloc_trace_ctrl.baseline_valid;
    enabled = divoom_malloc_trace_ctrl.enabled;
    coverage_lost = divoom_malloc_trace_ctrl.coverage_lost;
    snapshot = (!begin && count > 0) ? malloc(count * sizeof(*snapshot)) : NULL;
    if (snapshot != NULL)
    {
        for (bucket = 0; bucket < DIVOOM_MALLOC_TRACE_BUCKETS; bucket++)
        {
            for (item = divoom_malloc_trace_ctrl.buckets[bucket]; item != NULL; item = item->next)
            {
                if (!diff || !item->baseline || item->size > item->baseline_size)
                {
                    snapshot[selected++] = *item;
                }
            }
        }
    }
    divoom_malloc_trace_free_lock();
    divoom_print("[malloc_trace] %s active=%u baseline=%u live_blocks=%u live_bytes=%u metadata_bytes=%u dropped=%u coverage_lost=%u status=%s\n",
        begin ? "begin" : (diff ? "diff" : "dump"), enabled, valid, count, bytes,
        (uint32)(count * sizeof(*item)), dropped, coverage_lost,
        (dropped || coverage_lost || !enabled || (diff && !valid) || (!begin && count && !snapshot)) ? "INCOMPLETE" : "OK");
    bytes = 0;
    for (bucket = 0; bucket < selected; bucket++)
    {
        item = &snapshot[bucket];
        bytes += item->size;
        if (detail)
        {
            divoom_print("[malloc_trace] live ptr=%p bytes=%u baseline_bytes=%u site=%s:%u\n",
                item->ptr, item->size, item->baseline_size, item->file, item->line);
        }
    }
    if (!begin)
    {
        divoom_print("[malloc_trace] selected_blocks=%u selected_bytes=%u snapshot_bytes=%u\n",
            selected, bytes, (uint32)(snapshot ? count * sizeof(*snapshot) : 0));
    }
    if (selected > 0)
    {
        qsort(snapshot, selected, sizeof(*snapshot), divoom_malloc_trace_site_compare);
        for (bucket = 0; bucket < selected; bucket++)
        {
            if (sites > 0 && divoom_malloc_trace_site_compare(&snapshot[sites - 1], &snapshot[bucket]) == 0)
            {
                snapshot[sites - 1].blocks += snapshot[bucket].blocks;
                snapshot[sites - 1].size += snapshot[bucket].size;
                snapshot[sites - 1].baseline_size += snapshot[bucket].baseline_size;
            }
            else
            {
                snapshot[sites++] = snapshot[bucket];
            }
        }
        qsort(snapshot, sites, sizeof(*snapshot), divoom_malloc_trace_count_compare);
        for (bucket = 0; bucket < sites; bucket++)
        {
            item = &snapshot[bucket];
            divoom_print("[malloc_trace] rank=%u blocks=%u bytes=%u baseline_bytes=%u site=%s:%u\n",
                bucket + 1, item->blocks, item->size, item->baseline_size, item->file, item->line);
        }
    }
    free(snapshot);
}

#if DIVOOM_BUILD_RTOS_DEVICE
#include <console.h>
/* 串口操作在命令线程完成，不在按键回调里输出大量记录。 */
static int divoom_malloc_trace_cli(int argc, char **argv)
{
    if (argc == 2 && (!strcmp(argv[1], "start") || !strcmp(argv[1], "stop")))
    {
        divoom_malloc_trace_get_lock();
        divoom_malloc_trace_ctrl.enabled = !strcmp(argv[1], "start");
        divoom_malloc_trace_ctrl.baseline_valid = 0;
        if (!divoom_malloc_trace_ctrl.enabled)
        {
            divoom_malloc_trace_ctrl.coverage_lost = 1;
        }
        divoom_malloc_trace_free_lock();
        divoom_print("[malloc_trace] %s; use begin before switching, diff after returning\n", argv[1]);
    }
    else if (argc == 2 && (!strcmp(argv[1], "begin") || !strcmp(argv[1], "diff") || !strcmp(argv[1], "dump") || !strcmp(argv[1], "detail")))
    {
        divoom_malloc_trace_report(!strcmp(argv[1], "begin"), !strcmp(argv[1], "diff"), !strcmp(argv[1], "detail"));
    }
    else
    {
        divoom_print("malloc_trace start|begin|diff|dump|detail|stop\n");
        return -1;
    }
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(divoom_malloc_trace_cli, malloc_trace, heap live allocation diagnostics);
#endif
#endif


#if defined(DIVOOM_SIMULATOR)
#include "sim_malloc_leak_track.h"
#endif

#ifndef DIVOOM_MALLOC_DEBUG
//#define DIVOOM_MALLOC_DEBUG
#endif

#define DIVOOM_OS_MALLOC_TAIL_FLAG  0X5AA55AA5

struct _divoom_malloc_debug_ctrl;

typedef struct _divoom_malloc_debug_ctrl
{
    uint32 use_time;
    uint32 model_id;
    uint32 model_line;
    uint32 size;
    char * file_name;
    void *ptr;
    struct _divoom_malloc_debug_ctrl *next;
}divoom_malloc_debug_ctrl, *Pdivoom_malloc_debug_ctrl;

Pdivoom_malloc_debug_ctrl pdivoom_malloc_debug_handle = NULL;

void  *pdivoom_malloc_lock = NULL;
uint8 lock_init_flag = 0;
void divoom_malloc_print(void);

uint8 divoom_start_debug_flag = 0;
void divoom_malloc_debug_start_ctrl(uint8 flag)
{
    divoom_start_debug_flag = flag;
}

#if 0
#define TRACING_NUM (1000)

int start_trace() {
    static  heap_trace_record_t* record = NULL;
    if (record == NULL) {
        record = (heap_trace_record_t*) malloc(TRACING_NUM*sizeof(heap_trace_record_t));
        heap_trace_init_standalone(record, TRACING_NUM);
        heap_trace_start(HEAP_TRACE_LEAKS);
    } else {
        heap_trace_resume();
    }
    return 0;    
}
void stop_trace() {
    heap_trace_stop();
    heap_trace_dump();
}
#endif
/**
 * *  @brief       	divoom_malloc_debug_insert_item
 				将分配的内存插入到队列中
 * *  @date       	2014-10-28
 * *  @param[in]    ptr
                           id
                           line
				
 * *  @return        void
 * */

void divoom_malloc_debug_insert_item(void *ptr, uint32 size, uint32 id, uint32 line, const char *file_name)
{
    Pdivoom_malloc_debug_ctrl new_item, head;

    if(divoom_start_debug_flag == 0)
    {
        return;
    }
    new_item = (Pdivoom_malloc_debug_ctrl)malloc(sizeof(divoom_malloc_debug_ctrl));
    if(new_item == NULL)
    {
        divoom_print("divoom err at malloc debug item!\n");
        return;
    }
    new_item->use_time = 1;
    new_item->model_id = id;
    new_item->model_line = line;
    new_item->size = size;
    new_item->ptr = ptr;
    new_item->file_name = (char *)file_name;
    new_item->next = NULL;

    head = pdivoom_malloc_debug_handle;
    if(head == NULL)
    {
        pdivoom_malloc_debug_handle = new_item;
    }
    else
    {
        while(head->next)
        {
            head = head->next;
        }
        head->next = new_item;
    }
        
}

void divoom_malloc_print(void)
{
#if defined(DIVOOM_MEMORY_LEAK_DIAG_ENABLE) && DIVOOM_MEMORY_LEAK_DIAG_ENABLE
    /* 兼容原打印入口，统一输出正常内存池路径下的未释放位置排行。 */
    divoom_malloc_trace_report(0, 0, 0);
#else
    int ram_size = 0, ram_number = 0;
    Pdivoom_malloc_debug_ctrl head;
    head = pdivoom_malloc_debug_handle;
	while(head != NULL)
	{
        //if(head->use_time > 2)
		divoom_print("ptr = %p, model_line = %d, size = %d, File:%s, model_id:%d,time: %d\n", head->ptr,
				head->model_line, head->size, head->file_name, head->model_id, head->use_time);
        head->use_time ++;
        ram_size += head->size;
		head = head->next;
        ram_number ++;
	}
    divoom_print("sys use ram size: %d, %d\n", ram_size, ram_number);
#endif
}

#ifdef DIVOOM_MALLOC_DEBUG

void divoom_malloc_check_all(void)
{
    uint32 *flag, ram_num = 0;
    Pdivoom_malloc_debug_ctrl head;
    
    divoom_system_get_lock_direct(pdivoom_malloc_lock);
    head = pdivoom_malloc_debug_handle;
	while(head != NULL)
	{
        //if(head->use_time > 2)
        flag = (uint32 *)((uint32)head->ptr + head->size);
        if(*flag != DIVOOM_OS_MALLOC_TAIL_FLAG)
        {
            divoom_print("\n\n*******************check ram head tail err!%p:%d: %s:%d! %x,%x\n", head->ptr, head->size, head->file_name, head->model_line, *flag , DIVOOM_OS_MALLOC_TAIL_FLAG);
        }
        ram_num ++;
		head = head->next;
	}
    divoom_system_free_lock_direct(pdivoom_malloc_lock);
    divoom_print("check over!: %d\n",ram_num);
	
}

void divoom_malloc_check_ptr(void *ptr, int size,char *file_name, int line)
{
    uint32 *flag = (uint32 *)ptr;
    
    flag = (uint32 *)((uint32)ptr + size);
    if(*flag != DIVOOM_OS_MALLOC_TAIL_FLAG)
    {
        divoom_print("\n\n*******************check ram head tail err!:%p %s:%d! %x,%x\n",flag, file_name, line, *flag , DIVOOM_OS_MALLOC_TAIL_FLAG);
    }
    
}

int divoom_malloc_debug_get_ram_size(void *ptr)
{
    Pdivoom_malloc_debug_ctrl pre_item = NULL, head;
    
    if(divoom_start_debug_flag == 0)
    {
        divoom_print("pass!\n");
        return 0;
    }
    head = pdivoom_malloc_debug_handle;

    while(head)
    {
        if(head->ptr == ptr)
        {
            break;
        }
        pre_item = head;
        head = head->next;
    }
    if(head)
    {
        return head->size;
    }
    return 0;
    
}


#endif


int divoom_malloc_number = 0;
void *divoom_malloc_raw(uint32 size, uint32 mode_id, uint32 model_line, const char *file_name, int zero_flag)
{
	void *ptr = NULL;
    
#ifdef DIVOOM_MALLOC_DEBUG
    if(lock_init_flag == 0)
    {
        lock_init_flag = 1;
        pdivoom_malloc_lock = malloc(sizeof(pthread_mutex_t));
        
        pthread_mutex_init(pdivoom_malloc_lock, NULL);
    }
    divoom_system_get_lock_direct(pdivoom_malloc_lock);
#else
    /* 扩展池内部按 104、160、204/212 精确匹配，未命中仍回退系统堆。 */
    if(size <= DIVOOM_RAM_POOL_EXT_MAX_REQUEST_SIZE)
    {
        ptr = divoom_memory_pool_malloc(size);
        if(ptr)
        {
#if defined(DIVOOM_SIMULATOR)
            sim_malloc_leak_track_on_alloc(ptr, size, mode_id, model_line, file_name);
#endif
#if defined(DIVOOM_MEMORY_LEAK_DIAG_ENABLE) && DIVOOM_MEMORY_LEAK_DIAG_ENABLE
            divoom_malloc_trace_add(ptr, size, file_name, model_line);
#endif
            return ptr;
        }
    }
#endif
    #ifdef DIVOOM_MALLOC_DEBUG
    size = (size + 3) /4*4;
	ptr = (void *)malloc(size + 4);
    #else
	ptr = (void *)malloc(size);
    #endif
	if(ptr)
	{
        divoom_malloc_number ++;
#if defined(DIVOOM_MEMORY_LEAK_DIAG_ENABLE) && DIVOOM_MEMORY_LEAK_DIAG_ENABLE
        divoom_malloc_trace_add(ptr, size, file_name, model_line);
#endif
        //divoom_print("malloc: %x:%x, %s:%d\n", (T_U32)ptr, (T_U32)ptr + size,file_name, model_line);
        if(zero_flag)
        {
		    memset(ptr, 0, size);
        }
#if defined(DIVOOM_SIMULATOR)
        sim_malloc_leak_track_on_alloc(ptr, size, mode_id, model_line, file_name);
#endif
	}
    else
    {
        divoom_print("divoom_malloc FAIL: %u,free:%u %s:%u\n",
            (unsigned int)size, (unsigned int)divoom_system_get_ram_free_size(), file_name, (unsigned int)model_line);
    }
#ifdef DIVOOM_MALLOC_DEBUG
    if(ptr && divoom_start_debug_flag)
    {
        uint32 *flag = (uint32 *)ptr;
        
        flag = (uint32 *)((uint32)ptr + size);
        *flag = DIVOOM_OS_MALLOC_TAIL_FLAG;
        
    	divoom_malloc_debug_insert_item(ptr,size, mode_id,model_line, file_name);
    	//divoom_print("[divoom malloc debug]ptr:%x,file:%s line:%d, \n",ptr, file_name, model_line);
    }
    divoom_system_free_lock_direct(pdivoom_malloc_lock);
#endif
	return ptr;
}
void *divoom_malloc(uint32 size, uint32 mode_id, uint32 model_line, const char *file_name)
{
    return divoom_malloc_raw(size, mode_id, model_line, file_name, 1);
}
/**
 * *  @brief       	divoom_remalloc
 				重新分配内存
 * *  @date       	2014-10-28
 * *  @param[in]    ptr
 * *  @return        void *
 * */

void *divoom_remalloc(void *ptr,uint32 size, uint32 model_line, const char *file_name)
{
    int block_size;
#if defined(DIVOOM_MEMORY_LEAK_DIAG_ENABLE) && DIVOOM_MEMORY_LEAK_DIAG_ENABLE
    /* 明确定义零长度释放，避免不同 libc 的 realloc(ptr, 0) 语义造成漏记。 */
    if (size == 0)
    {
        divoom_free_line(ptr, file_name, model_line);
        return NULL;
    }
#endif
#ifndef DIVOOM_MALLOC_DEBUG
    if((block_size = divoom_memory_pool_check_ptr(ptr)) > 0)
    {
        void *new_ptr;

#if defined(DIVOOM_MEMORY_LEAK_DIAG_ENABLE) && DIVOOM_MEMORY_LEAK_DIAG_ENABLE
        new_ptr = divoom_malloc_raw(size, 0, model_line, file_name, 1);
        /* 申请失败时保留旧块和追踪记录，不能 memcpy 到 NULL。 */
        if (new_ptr == NULL)
        {
            return NULL;
        }
#else
        new_ptr = divoom_system_malloc(size);
#endif
        if(size > block_size)
        {
            memcpy(new_ptr, ptr, block_size);
        }
        else
        {
            memcpy(new_ptr, ptr, size);
        }
#if defined(DIVOOM_MEMORY_LEAK_DIAG_ENABLE) && DIVOOM_MEMORY_LEAK_DIAG_ENABLE
        divoom_malloc_trace_move(ptr, new_ptr, size, file_name, model_line);
        divoom_free_line(ptr, file_name, model_line);
#else
        divoom_memory_pool_free(ptr);
#endif
        return new_ptr;
    }
    else
    #endif
    {
#ifdef DIVOOM_MALLOC_DEBUG
		divoom_system_get_lock_direct(pdivoom_malloc_lock);
        block_size = divoom_malloc_debug_get_ram_size(ptr);
		divoom_system_free_lock_direct(pdivoom_malloc_lock);
        if(block_size)
        {
            void *new_ptr;

#if defined(DIVOOM_MEMORY_LEAK_DIAG_ENABLE) && DIVOOM_MEMORY_LEAK_DIAG_ENABLE
            new_ptr = divoom_malloc_raw(size, 0, model_line, file_name, 1);
            /* 申请失败时保留旧块和追踪记录，不能 memcpy 到 NULL。 */
            if (new_ptr == NULL)
            {
                return NULL;
            }
#else
            new_ptr = divoom_system_malloc(size);
#endif
            if(size > block_size)
            {
                memcpy(new_ptr, ptr, block_size);
            }
            else
            {
                memcpy(new_ptr, ptr, size);
            }
#if defined(DIVOOM_MEMORY_LEAK_DIAG_ENABLE) && DIVOOM_MEMORY_LEAK_DIAG_ENABLE
            divoom_malloc_trace_move(ptr, new_ptr, size, file_name, model_line);
#endif
            divoom_system_free(ptr);        
            return new_ptr;
        }
        else if(ptr == NULL)
        {
            return divoom_system_malloc(size); 
        }
        else
        {
            void *new_ptr;

#if defined(DIVOOM_MEMORY_LEAK_DIAG_ENABLE) && DIVOOM_MEMORY_LEAK_DIAG_ENABLE
            new_ptr = divoom_malloc_trace_realloc(ptr, size, file_name, model_line);
#else
            new_ptr = realloc(ptr, size);
#endif
            divoom_print("remalloc: %p, %p\n", ptr, new_ptr);
            return new_ptr;
        }
        #else
#if defined(DIVOOM_MEMORY_LEAK_DIAG_ENABLE) && DIVOOM_MEMORY_LEAK_DIAG_ENABLE
        return divoom_malloc_trace_realloc(ptr, size, file_name, model_line);
#else
        return realloc(ptr, size);
#endif
        #endif
    }
}
#ifdef DIVOOM_MALLOC_DEBUG

/**
 * *  @brief       	divoom_malloc_debug_del_item
 				将释放的内存从队列中移出
 * *  @date       	2014-10-28
 * *  @param[in]    ptr
 * *  @return        void
 * */

bool divoom_malloc_debug_del_item(void *ptr)
{
    Pdivoom_malloc_debug_ctrl pre_item = NULL, head;
    uint32 *flag = (uint32 *)ptr;
    
    if(divoom_start_debug_flag == 0)
    {
        return 1;
    }
    head = pdivoom_malloc_debug_handle;

    while(head)
    {
        if(head->ptr == ptr)
        {
            break;
        }
        pre_item = head;
        head = head->next;
    }
    if(head == NULL)
    {
        return false;
    }
    if(pre_item == NULL)
    {
        pdivoom_malloc_debug_handle = head->next;
    }
    else
    {
        pre_item->next = head->next;
    }
    flag = (uint32 *)((uint32)ptr + head->size);
    if(*flag != DIVOOM_OS_MALLOC_TAIL_FLAG)
    {
        divoom_print("\n\n*******************check ram tail err!:%p:%d %s:%d! %x, %x\n",ptr, head->size, head->file_name, head->model_line, *flag, DIVOOM_OS_MALLOC_TAIL_FLAG);
    }
    free(head);
    return true;
    
}
#endif

char *g_malloc_test_file_name = NULL;
int   g_malloc_test_file_line = 0;
/**
 * NAME               divoom_free
 * *  @brief       	释放内存，主要是为了适应不同平台，等同于free
 * *  @date       	2014-10-28
 * *  @param[in] 	ptr     divoom_malloc的返回值
 * *  @return       void
 * */

void divoom_free_line(void *ptr, const char *file_name, int line)
{
	if(ptr)
	{
#if defined(DIVOOM_MEMORY_LEAK_DIAG_ENABLE) && DIVOOM_MEMORY_LEAK_DIAG_ENABLE
        divoom_malloc_trace_remove(ptr);
#endif
        #ifndef DIVOOM_MALLOC_DEBUG
        if(divoom_memory_pool_free(ptr))
        {
            return;
        }
        #else
	if(lock_init_flag && divoom_start_debug_flag)
	{
		divoom_system_get_lock_direct(pdivoom_malloc_lock);
		if(divoom_malloc_debug_del_item(ptr))
		{
		}
		divoom_system_free_lock_direct(pdivoom_malloc_lock);
	}
#endif
        divoom_malloc_number --;

        g_malloc_test_file_name = (char *)file_name;
        g_malloc_test_file_line = line;

        //divoom_print("free: %p:%s:%d\n", ptr, file_name, line);
#if defined(DIVOOM_SIMULATOR)
        sim_malloc_leak_track_on_free(ptr);
#endif
		free(ptr);  //malloc_usable_size这个是获取指针大小
		ptr = NULL;
	}
}



/*********************************************************************
  Function:       divoom_malloc_check_all_ram_in
  Description:	   进入模块之前检查内存分配情况
  Input:          NONE
  Return:         NONE
  Author:         wangft
  Data:           2019-11-21
**********************************************************************/
void divoom_malloc_check_all_ram_in(void)
{
/*
#ifdef DIVOOM_MALLOC_DEBUG
	int i, use_num = 0;

	memset(pdivoom_malloc_debug_handle->ram_in_use, 0, sizeof(pdivoom_malloc_debug_handle->ram_in_use));
	for(i = 0; i < DIVOOM_OS_MALLOC_DBG_ITEMS; i++)
	{
		if(pdivoom_malloc_debug_handle->all_ram_info[i].use_flag)
		{

			use_num++;
			pdivoom_malloc_debug_handle->ram_in_use[i] = 1;         	
		}
	}
	divoom_print("[%s:%d] ram use num %d\n", __func__, __LINE__, use_num);
    #endif
*/
}

/*********************************************************************
  Function:       divoom_malloc_check_all_ram_out
  Description:	   退出模块之后检查内存分配情况
  Input:          NONE
  Return:         1 前后一样 0 前后不一样
  Author:         wangft
  Data:           2019-11-21
**********************************************************************/
uint8 divoom_malloc_check_all_ram_out(const char *func_name)
{
/*
#ifdef DIVOOM_MALLOC_DEBUG
	T_U32 i, use_num = 0;
    T_U8 ret = AK_TRUE;

	memset(pdivoom_malloc_debug_handle->ram_out_use, 0, sizeof(pdivoom_malloc_debug_handle->ram_out_use));
    for(i = 0; i < DIVOOM_OS_MALLOC_DBG_ITEMS; i++)
    {
        if(pdivoom_malloc_debug_handle->all_ram_info[i].use_flag)
        {
			 use_num++;
			 pdivoom_malloc_debug_handle->ram_out_use[i] = 1;
        }
    }

    for(i = 0; i < DIVOOM_OS_MALLOC_DBG_ITEMS; i++)
    {
		if (pdivoom_malloc_debug_handle->ram_out_use[i] == 1 && pdivoom_malloc_debug_handle->ram_in_use[i] == 0)
		{
			 divoom_print("[%s:%s]: add ram at: %x, file: %s;line:%d\n",__func__, func_name, pdivoom_malloc_debug_handle->all_ram_info[i].ptr,
               pdivoom_malloc_debug_handle->all_ram_info[i].file_name, pdivoom_malloc_debug_handle->all_ram_info[i].file_line);
			 ret = AK_FALSE;
		}
	}
	divoom_print("[%s:%d] ram use num %d\n", __func__, __LINE__, use_num);
	return ret;
    #else
    return AK_TRUE;
    #endif
*/
    return 1;
}



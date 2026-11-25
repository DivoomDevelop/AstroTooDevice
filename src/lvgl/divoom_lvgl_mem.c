/* LVGL uses the application's shared pool and heap fallback. */
#include "lvgl.h"
#include "divoom.h"

#if LV_USE_STDLIB_MALLOC == LV_STDLIB_CUSTOM

void lv_mem_init(void)
{
    /* Idempotent; normal boot and production test share this pool. */
    divoom_memory_pool_init(
        DIVOOM_RAM_POOL_TOTAL_RAM_NUM * DIVOOM_RAM_POOL_BLOCK_RAM_SIZE,
        DIVOOM_RAM_POOL_BLOCK_RAM_SIZE,
        DIVOOM_RAM_POOL_TOTAL_MAX_RAM_NUM * DIVOOM_RAM_POOL_BLOCK_MAX_RAM_SIZE,
        DIVOOM_RAM_POOL_BLOCK_MAX_RAM_SIZE);
}

void lv_mem_deinit(void)
{
    /* The pool belongs to the application and outlives LVGL. */
}

void * lv_malloc_core(size_t size)
{
    return divoom_system_malloc_no_zero(size);
}

void * lv_realloc_core(void * p, size_t new_size)
{
    /* Divoom's realloc(NULL, n) otherwise bypasses the small-block pool. */
    if(p == NULL) return lv_malloc_core(new_size);
    return divoom_system_remalloc(p, new_size);
}

void lv_free_core(void * p)
{
    divoom_system_free(p);
}

lv_mem_pool_t lv_mem_add_pool(void * mem, size_t bytes)
{
    LV_UNUSED(mem);
    LV_UNUSED(bytes);
    return NULL;
}

void lv_mem_remove_pool(lv_mem_pool_t pool)
{
    LV_UNUSED(pool);
}

void lv_mem_monitor_core(lv_mem_monitor_t * mon_p)
{
    /* As with the previous CLIB backend, use Divoom's pool diagnostics. */
    LV_UNUSED(mon_p);
}

lv_result_t lv_mem_test_core(void)
{
    return LV_RESULT_OK;
}

#endif

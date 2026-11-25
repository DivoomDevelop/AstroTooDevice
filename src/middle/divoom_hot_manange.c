#include "divoom.h"
#include "divoom_subscribe_local_pic.h"
#include <sys/times.h>
#include <unistd.h>
#include <time.h>
#if defined(DIVOOM_SIMULATOR)
#include <stdio.h>
#include <stdlib.h>
#endif

#define DIVOOM_HOT_MANAGE_MAX_FILE_NUM 300
#define DIVOOM_HOT_MANAGE_LOAD_NEW_FILE_TIME 600
#if defined(DIVOOM_SIMULATOR)
#define DIVOOM_HOT_MANAGE_FIRST_LOAD_FILE_TIME (300)
#define DIVOOM_HOT_MANAGE_LOAD_FILE_TIME (600)
#else
#define DIVOOM_HOT_MANAGE_FIRST_LOAD_FILE_TIME (600)
#define DIVOOM_HOT_MANAGE_LOAD_FILE_TIME (6*3600)
#endif
#define DIVOOM_HOT_MANAGE_SIZE_TYPE_NUM 5
#define DIVOOM_HOT_MANAGE_DOWNLOAD_LOW_WATERMARK 8
/* 本地兜底文件扫描的初始容量；扫描触顶后会继续扩容。 */
#define DIVOOM_HOT_MANAGE_INITIAL_CAPACITY 16

int divoom_hot_manage_request_hot_image(int image_size, int wait_flag);

typedef struct _divoom_hot_manage_item
{
    int use_time;
    int image_num;
    int image_capacity;
    Pdivoom_image_hot_item image_list;
}divoom_hot_manage_item, *Pdivoom_hot_manage_item;

typedef struct _divoom_hot_manage_ready_item
{
    uint32 image_id;
    uint32 image_size;
}divoom_hot_manage_ready_item, *Pdivoom_hot_manage_ready_item;

typedef struct _divoom_hot_manage_ctrl
{
    int first_request_flag;
    int update_time;
    int last_request_time;
    /* 分别缓存严格小于 16/32/64/128/256 的图片数，避免随机选择时重复汇总。 */
    uint32 less_than_size_num[DIVOOM_HOT_MANAGE_SIZE_TYPE_NUM];
    uint32 valid_size_num;
    /* 已落盘索引；[0, local_ready_lt_128_end) 固定为严格小于 128 档。 */
    uint32 local_ready_num;
    uint32 local_ready_lt_128_end;
    divoom_hot_manage_ready_item local_ready_list[DIVOOM_HOT_MANAGE_MAX_FILE_NUM];
    int32 image_id_index_num;
    uint32 image_id_index[DIVOOM_HOT_MANAGE_MAX_FILE_NUM];
    uint8 pending_pump_running;
    int pending_index;
    void *state_lock;
    void *pall_random_handle;
    uint32 random_max_size;
    Pdivoom_devcice_image_list pending_list;
    divoom_hot_manage_item    pic_queue;
}divoom_hot_manage_ctrl, *Pdivoom_hot_manage_ctrl;

Pdivoom_hot_manage_ctrl pdivoom_hot_manage_handle = NULL;

static const uint16 g_divoom_hot_manage_size_list[DIVOOM_HOT_MANAGE_SIZE_TYPE_NUM] =
{
    DIVOOM_HOT_MANAGE_IMAGE_SIZE_16,
    DIVOOM_HOT_MANAGE_IMAGE_SIZE_32,
    DIVOOM_HOT_MANAGE_IMAGE_SIZE_64,
    DIVOOM_HOT_MANAGE_IMAGE_SIZE_128,
    DIVOOM_HOT_MANAGE_IMAGE_SIZE_256
};

static int divoom_hot_manage_compare_uint32(
    const void *item1, const void *item2)
{
    uint32 value1 = *(const uint32 *)item1;
    uint32 value2 = *(const uint32 *)item2;

    return value1 < value2 ? -1 : value1 > value2;
}

/* 调用方持有 state_lock；主列表保持更新时间顺序，独立 ID 索引用于二分查重。 */
static void divoom_hot_manage_rebuild_image_id_index_locked(void)
{
    int32 i;
    int32 image_num;

    if(pdivoom_hot_manage_handle == NULL)
    {
        return;
    }
    image_num = pdivoom_hot_manage_handle->pic_queue.image_num;
    if(image_num < 0)
    {
        image_num = 0;
    }
    if(image_num > DIVOOM_HOT_MANAGE_MAX_FILE_NUM)
    {
        image_num = DIVOOM_HOT_MANAGE_MAX_FILE_NUM;
    }
    for(i = 0; i < image_num; i++)
    {
        pdivoom_hot_manage_handle->image_id_index[i] =
            pdivoom_hot_manage_handle->pic_queue.image_list[i].image_id;
    }
    pdivoom_hot_manage_handle->image_id_index_num = image_num;
    if(image_num > 1)
    {
        qsort(pdivoom_hot_manage_handle->image_id_index,
            (size_t)image_num, sizeof(uint32),
            divoom_hot_manage_compare_uint32);
    }
}

static int32 divoom_hot_manage_find_image_id_index_locked(uint32 image_id)
{
    int32 low = 0;
    int32 high;
    int32 mid;

    if(pdivoom_hot_manage_handle == NULL ||
        pdivoom_hot_manage_handle->image_id_index_num <= 0)
    {
        return -1;
    }
    high = pdivoom_hot_manage_handle->image_id_index_num - 1;
    while(low <= high)
    {
        mid = low + (high - low) / 2;
        if(pdivoom_hot_manage_handle->image_id_index[mid] == image_id)
        {
            return mid;
        }
        if(pdivoom_hot_manage_handle->image_id_index[mid] < image_id)
        {
            low = mid + 1;
        }
        else
        {
            high = mid - 1;
        }
    }
    return -1;
}

static void divoom_hot_manage_add_image_id_index_locked(uint32 image_id)
{
    int32 low = 0;
    int32 high;
    int32 mid;

    if(pdivoom_hot_manage_handle == NULL ||
        pdivoom_hot_manage_handle->image_id_index_num >=
            DIVOOM_HOT_MANAGE_MAX_FILE_NUM)
    {
        return;
    }
    high = pdivoom_hot_manage_handle->image_id_index_num;
    while(low < high)
    {
        mid = low + (high - low) / 2;
        if(pdivoom_hot_manage_handle->image_id_index[mid] < image_id)
        {
            low = mid + 1;
        }
        else
        {
            high = mid;
        }
    }
    if(low < pdivoom_hot_manage_handle->image_id_index_num &&
        pdivoom_hot_manage_handle->image_id_index[low] == image_id)
    {
        return;
    }
    if(low < pdivoom_hot_manage_handle->image_id_index_num)
    {
        memmove(&pdivoom_hot_manage_handle->image_id_index[low + 1],
            &pdivoom_hot_manage_handle->image_id_index[low],
            (uint32)(pdivoom_hot_manage_handle->image_id_index_num - low) *
                sizeof(uint32));
    }
    pdivoom_hot_manage_handle->image_id_index[low] = image_id;
    pdivoom_hot_manage_handle->image_id_index_num++;
}

static void divoom_hot_manage_remove_image_id_index_locked(uint32 image_id)
{
    int32 index;

    index = divoom_hot_manage_find_image_id_index_locked(image_id);
    if(index < 0)
    {
        return;
    }
    pdivoom_hot_manage_handle->image_id_index_num--;
    if(index < pdivoom_hot_manage_handle->image_id_index_num)
    {
        memmove(&pdivoom_hot_manage_handle->image_id_index[index],
            &pdivoom_hot_manage_handle->image_id_index[index + 1],
            (uint32)(pdivoom_hot_manage_handle->image_id_index_num - index) *
                sizeof(uint32));
    }
}

/* HOT 列表按需扩容；申请失败时保留旧数组，调用方可安全继续使用原数据。 */
static int32 divoom_hot_manage_reserve_image_list(
    Pdivoom_hot_manage_item pitem, int32 required_num)
{
    int32 max_capacity;
    int32 new_capacity;
    Pdivoom_image_hot_item new_list;

    if(pitem == NULL || required_num <= 0)
    {
        return 0;
    }
    if(required_num <= pitem->image_capacity)
    {
        return 1;
    }
    max_capacity = (int32)(
        U32_MAX / (uint32)sizeof(divoom_image_hot_item));
    if(required_num > max_capacity)
    {
        return 0;
    }

    new_capacity = pitem->image_capacity;
    if(new_capacity <= 0)
    {
        new_capacity = required_num;
    }
    while(new_capacity < required_num)
    {
        if(new_capacity > max_capacity / 2)
        {
            new_capacity = required_num;
            break;
        }
        new_capacity *= 2;
    }

    new_list = divoom_system_malloc(
        (uint32)new_capacity * sizeof(divoom_image_hot_item));
    if(new_list == NULL)
    {
        return 0;
    }
    if(pitem->image_list != NULL && pitem->image_num > 0)
    {
        memcpy(new_list, pitem->image_list,
            (uint32)pitem->image_num * sizeof(divoom_image_hot_item));
    }
    if(pitem->image_list != NULL)
    {
        divoom_system_free(pitem->image_list);
    }
    pitem->image_list = new_list;
    pitem->image_capacity = new_capacity;
    return 1;
}

/* image_name 保存作者名，由 HOT 项深拷贝持有，不能引用临时网络响应。 */
static char *divoom_hot_manage_copy_author_name(const char *author_name)
{
    uint32 name_size;
    char *name_copy;

    if(author_name == NULL)
    {
        author_name = "";
    }
    name_size = (uint32)strlen(author_name) + 1;
    name_copy = divoom_system_malloc(name_size);
    if(name_copy != NULL)
    {
        memcpy(name_copy, author_name, name_size);
    }
    return name_copy;
}

/* 按尺寸、ID、作者名、用户 ID 四项保存；作者名深拷贝成功后数量才递增。 */
static int32 divoom_hot_manage_append_image(Pdivoom_hot_manage_item pitem,
    uint32 image_size, uint32 image_id, const char *author_name, uint32 user_id)
{
    char *name_copy;

    if(pitem == NULL || pitem->image_num < 0 ||
        (uint32)pitem->image_num >=
            U32_MAX / (uint32)sizeof(divoom_image_hot_item))
    {
        return 0;
    }
    if(divoom_hot_manage_reserve_image_list(
        pitem, pitem->image_num + 1) == 0)
    {
        return 0;
    }
    name_copy = divoom_hot_manage_copy_author_name(author_name);
    if(name_copy == NULL)
    {
        return 0;
    }
    pitem->image_list[pitem->image_num].image_size = image_size;
    pitem->image_list[pitem->image_num].image_id = image_id;
    pitem->image_list[pitem->image_num].image_name = name_copy;
    pitem->image_list[pitem->image_num].user_id = user_id;
    pitem->image_num++;
    return 1;
}

/* 本地兜底文件按需扩容扫描，但不突破 HOT 模块的业务数量上限。 */
static divoom_image_base_info *divoom_hot_manage_load_local_pic(int32 *image_num)
{
    int32 capacity = DIVOOM_HOT_MANAGE_INITIAL_CAPACITY;
    int32 loaded_num;
    divoom_image_base_info *local_pic_list;

    if(image_num == NULL)
    {
        return NULL;
    }
    *image_num = 0;
    while(capacity <= DIVOOM_HOT_MANAGE_MAX_FILE_NUM)
    {
        local_pic_list = divoom_system_malloc(
            (uint32)capacity * sizeof(divoom_image_base_info));
        if(local_pic_list == NULL)
        {
            return NULL;
        }
        loaded_num = 0;
        divoom_subscribe_local_pic_load(
            DIVOOM_MANAGE_IMAGE_SUBSCRIBE_DEFAULT_16,
            local_pic_list,
            &loaded_num,
            capacity,
            0);
        if(loaded_num < capacity ||
            capacity == DIVOOM_HOT_MANAGE_MAX_FILE_NUM)
        {
            *image_num = loaded_num;
            return local_pic_list;
        }
        divoom_system_free(local_pic_list);
        if(capacity > DIVOOM_HOT_MANAGE_MAX_FILE_NUM / 2)
        {
            capacity = DIVOOM_HOT_MANAGE_MAX_FILE_NUM;
        }
        else
        {
            capacity *= 2;
        }
    }
    return NULL;
}

#if defined(DIVOOM_SIMULATOR)
/* 仿真自检临时列表使用完整释放路径，校验每个动态名称的所有权。 */
static void divoom_hot_manage_sim_clear_image_list(
    Pdivoom_hot_manage_item pitem)
{
    int32 i;

    if(pitem == NULL)
    {
        return;
    }
    for(i = 0; i < pitem->image_num; i++)
    {
        if(pitem->image_list[i].image_name != NULL)
        {
            divoom_system_free(pitem->image_list[i].image_name);
        }
    }
    if(pitem->image_list != NULL)
    {
        divoom_system_free(pitem->image_list);
    }
    pitem->image_list = NULL;
    pitem->image_num = 0;
    pitem->image_capacity = 0;
}

/* 仅仿真验收启用：构造真实所有权和队列路径，不改变服务器协议或分页字段。 */
static void divoom_hot_manage_sim_memory_verify(void)
{
    int32 i;
    int32 file_num;
    uint32 alloc_size;
    divoom_hot_manage_item verify_item = {0};
    const char *verify_env = getenv("DIVOOM_SIM_HOT_VERIFY_COUNT");
    Pdivoom_devcice_image_list pimage_list;

    if(verify_env == NULL || verify_env[0] == '\0')
    {
        return;
    }
    file_num = atoi(verify_env);
    if(file_num <= 0)
    {
        return;
    }
    if((uint32)file_num >
        (U32_MAX - (uint32)sizeof(divoom_devcice_image_list)) /
            (uint32)sizeof(divoom_devcice_image_file))
    {
        divoom_print("[hot_mem] sim response count too large=%d\n", file_num);
        return;
    }
    alloc_size = (uint32)sizeof(divoom_devcice_image_list) +
        (uint32)file_num * sizeof(divoom_devcice_image_file);
    pimage_list = divoom_system_malloc(alloc_size);
    if(pimage_list == NULL)
    {
        divoom_print("[hot_mem] sim response malloc failed count=%d\n", file_num);
        return;
    }
    pimage_list->num = file_num;
    pimage_list->pfile_list = (Pdivoom_devcice_image_file)&pimage_list[1];
    for(i = 0; i < file_num; i++)
    {
        pimage_list->pfile_list[i].image_id = 900000000 + i;
        pimage_list->pfile_list[i].user_id = 800000000 + i;
        pimage_list->pfile_list[i].utc_time = 1900000000 + i;
        pimage_list->pfile_list[i].size = DIVOOM_HOT_MANAGE_IMAGE_SIZE_16;
        snprintf(pimage_list->pfile_list[i].file_id,
            sizeof(pimage_list->pfile_list[i].file_id), "sim/hot/%d", i);
        snprintf(pimage_list->pfile_list[i].image_name,
            sizeof(pimage_list->pfile_list[i].image_name), "sim_author_%d", i);
    }
    for(i = 0; i < file_num; i++)
    {
        if(divoom_hot_manage_append_image(
            &verify_item,
            (uint32)pimage_list->pfile_list[i].size,
            pimage_list->pfile_list[i].image_id,
            pimage_list->pfile_list[i].image_name,
            pimage_list->pfile_list[i].user_id) == 0)
        {
            break;
        }
    }
    i = file_num - 1;
    if(verify_item.image_num == file_num &&
        verify_item.image_list[i].image_size ==
            (uint32)pimage_list->pfile_list[i].size &&
        verify_item.image_list[i].image_id ==
            (uint32)pimage_list->pfile_list[i].image_id &&
        strcmp(verify_item.image_list[i].image_name,
            pimage_list->pfile_list[i].image_name) == 0 &&
        verify_item.image_list[i].user_id ==
            (uint32)pimage_list->pfile_list[i].user_id)
    {
        divoom_print("[hot_mem] dynamic list pass fields=4 count=%d capacity=%d\n",
            verify_item.image_num, verify_item.image_capacity);
    }
    else
    {
        divoom_print("[hot_mem] dynamic list fail count=%d expected=%d\n",
            verify_item.image_num, file_num);
    }
    divoom_hot_manage_sim_clear_image_list(&verify_item);
    /* 提交路径用 user_id=0 跳过真实作者查询，避免自检向服务器发送测试账号。 */
    for(i = 0; i < file_num; i++)
    {
        pimage_list->pfile_list[i].user_id = 0;
    }
    if(divoom_hot_manage_submit_hot_list(pimage_list, NULL) == 0)
    {
        divoom_system_free(pimage_list);
    }
}
#endif

/* 将服务端尺寸限制在五种协议档位内，缓存计数只接纳有效档位。 */
static int32 divoom_hot_manage_get_size_index(uint32 image_size)
{
    int32 i;

    for(i = 0; i < DIVOOM_HOT_MANAGE_SIZE_TYPE_NUM; i++)
    {
        if(image_size == g_divoom_hot_manage_size_list[i])
        {
            return i;
        }
    }
    return -1;
}

/* 调用方持有 state_lock；HOT 元数据仍以 pic_queue 为唯一真源。 */
static Pdivoom_image_hot_item divoom_hot_manage_find_image_locked(uint32 image_id, int *index)
{
    int32 i;

    for(i = 0; i < pdivoom_hot_manage_handle->pic_queue.image_num; i++)
    {
        if(pdivoom_hot_manage_handle->pic_queue.image_list[i].image_id == image_id)
        {
            if(index)
            {
                *index = i;
            }
            return &pdivoom_hot_manage_handle->pic_queue.image_list[i];
        }
    }
    return NULL;
}

static int32 divoom_hot_manage_find_ready_index_locked(uint32 image_id)
{
    uint32 i;

    for(i = 0; i < pdivoom_hot_manage_handle->local_ready_num; i++)
    {
        if(pdivoom_hot_manage_handle->local_ready_list[i].image_id == image_id)
        {
            return (int32)i;
        }
    }
    return -1;
}

/* 所有候选计数、取图和随机状态迁移共用同一尺寸规则。 */
static int divoom_hot_manage_size_matches(uint32 image_size, uint32 max_size)
{
    if(max_size == DIVOOM_HOT_MANAGE_IMAGE_SIZE_128)
    {
        return image_size == DIVOOM_HOT_MANAGE_IMAGE_SIZE_128;
    }
    if(max_size == DIVOOM_HOT_MANAGE_IMAGE_SIZE_256_ONLY)
    {
        return image_size == DIVOOM_HOT_MANAGE_IMAGE_SIZE_256;
    }
    return image_size <= max_size;
}

/* 调用方持有 state_lock；得到该就绪项在当前随机候选集合中的位置。 */
static uint32 divoom_hot_manage_ready_random_index_locked(uint32 ready_index)
{
    uint32 i;
    uint32 index = 0;
    for(i = 0; i < ready_index; i++)
    {
        if(divoom_hot_manage_size_matches(
            pdivoom_hot_manage_handle->local_ready_list[i].image_size,
            pdivoom_hot_manage_handle->random_max_size))
        {
            index++;
        }
    }
    return index;
}

/* 删除后保持小图段连续，边界位置随之同步左移。 */
static void divoom_hot_manage_mark_not_ready_locked(uint32 image_id)
{
    int32 ready_index;
    uint32 move_num;

    ready_index = divoom_hot_manage_find_ready_index_locked(image_id);
    if(ready_index < 0)
    {
        return;
    }
    if(pdivoom_hot_manage_handle->pall_random_handle != NULL &&
        divoom_hot_manage_size_matches(
            pdivoom_hot_manage_handle->local_ready_list[ready_index].image_size,
            pdivoom_hot_manage_handle->random_max_size))
    {
        divoom_random_remove(pdivoom_hot_manage_handle->pall_random_handle,
            (int)divoom_hot_manage_ready_random_index_locked((uint32)ready_index));
    }
    if((uint32)ready_index < pdivoom_hot_manage_handle->local_ready_lt_128_end)
    {
        pdivoom_hot_manage_handle->local_ready_lt_128_end--;
    }
    move_num = pdivoom_hot_manage_handle->local_ready_num -
        (uint32)ready_index - 1U;
    if(move_num > 0)
    {
        memmove(&pdivoom_hot_manage_handle->local_ready_list[ready_index],
            &pdivoom_hot_manage_handle->local_ready_list[ready_index + 1],
            move_num * sizeof(divoom_hot_manage_ready_item));
    }
    pdivoom_hot_manage_handle->local_ready_num--;
}

/* 下载完成后只登记一次；小于 128 档插在边界前，其余档追加在边界后。 */
static void divoom_hot_manage_mark_ready_locked(uint32 image_id)
{
    uint32 insert_index;
    uint32 move_num;
    Pdivoom_image_hot_item image_item;

    if(divoom_hot_manage_find_ready_index_locked(image_id) >= 0 ||
        pdivoom_hot_manage_handle->local_ready_num >= DIVOOM_HOT_MANAGE_MAX_FILE_NUM)
    {
        return;
    }
    image_item = divoom_hot_manage_find_image_locked(image_id, NULL);
    if(image_item == NULL || divoom_hot_manage_get_size_index(image_item->image_size) < 0)
    {
        return;
    }

    if(image_item->image_size < DIVOOM_HOT_MANAGE_IMAGE_SIZE_128)
    {
        insert_index = pdivoom_hot_manage_handle->local_ready_lt_128_end;
        move_num = pdivoom_hot_manage_handle->local_ready_num - insert_index;
        if(move_num > 0)
        {
            memmove(&pdivoom_hot_manage_handle->local_ready_list[insert_index + 1],
                &pdivoom_hot_manage_handle->local_ready_list[insert_index],
                move_num * sizeof(divoom_hot_manage_ready_item));
        }
        pdivoom_hot_manage_handle->local_ready_lt_128_end++;
    }
    else
    {
        insert_index = pdivoom_hot_manage_handle->local_ready_num;
    }
    pdivoom_hot_manage_handle->local_ready_list[insert_index].image_id = image_id;
    pdivoom_hot_manage_handle->local_ready_list[insert_index].image_size =
        image_item->image_size;
    pdivoom_hot_manage_handle->local_ready_num++;
    if(pdivoom_hot_manage_handle->pall_random_handle != NULL &&
        divoom_hot_manage_size_matches(image_item->image_size,
            pdivoom_hot_manage_handle->random_max_size))
    {
        divoom_random_insert(pdivoom_hot_manage_handle->pall_random_handle,
            (int)divoom_hot_manage_ready_random_index_locked(insert_index));
    }
}

/* 新增图片时同步维护累计数量，保证计数与 pic_queue 始终一致。 */
static void divoom_hot_manage_adjust_size_num(uint32 image_size, int32 delta)
{
    int32 i;

    if(pdivoom_hot_manage_handle == NULL || divoom_hot_manage_get_size_index(image_size) < 0)
    {
        return;
    }

    if(delta > 0)
    {
        pdivoom_hot_manage_handle->valid_size_num++;
    }
    else if(pdivoom_hot_manage_handle->valid_size_num > 0)
    {
        pdivoom_hot_manage_handle->valid_size_num--;
    }

    for(i = 0; i < DIVOOM_HOT_MANAGE_SIZE_TYPE_NUM; i++)
    {
        if(image_size >= g_divoom_hot_manage_size_list[i])
        {
            continue;
        }
        if(delta > 0)
        {
            pdivoom_hot_manage_handle->less_than_size_num[i]++;
        }
        else if(pdivoom_hot_manage_handle->less_than_size_num[i] > 0)
        {
            pdivoom_hot_manage_handle->less_than_size_num[i]--;
        }
    }
}

/* 调用方持有 state_lock；只统计已落盘内存索引，不访问文件系统。 */
static uint32 divoom_hot_manage_get_local_eligible_num_locked(uint32 max_size)
{
    uint32 i;
    uint32 local_num;

    local_num = 0;
    for(i = 0; i < pdivoom_hot_manage_handle->local_ready_num; i++)
    {
        if(divoom_hot_manage_size_matches(
            pdivoom_hot_manage_handle->local_ready_list[i].image_size, max_size))
        {
            local_num++;
        }
    }
    return local_num;
}

uint8 divoom_hot_manage_has_local_pic(uint32 max_size)
{
    uint8 has_local_pic;

    if(pdivoom_hot_manage_handle == NULL)
    {
        return 0;
    }
    divoom_system_get_lock_direct(pdivoom_hot_manage_handle->state_lock);
    has_local_pic = divoom_hot_manage_get_local_eligible_num_locked(max_size) > 0;
    divoom_system_free_lock_direct(pdivoom_hot_manage_handle->state_lock);
    return has_local_pic;
}

void divoom_hot_manage_update_local_ready(uint32 image_id, uint8 ready)
{
    if(pdivoom_hot_manage_handle == NULL || image_id == 0)
    {
        return;
    }
    divoom_system_get_lock_direct(pdivoom_hot_manage_handle->state_lock);
    if(ready)
    {
        divoom_hot_manage_mark_ready_locked(image_id);
    }
    else
    {
        divoom_hot_manage_mark_not_ready_locked(image_id);
    }
    divoom_system_free_lock_direct(pdivoom_hot_manage_handle->state_lock);
}

/* 调用方持有 state_lock；满额时同步清理内存、数据库和下载缓存。 */
/* Subscription cleanup also removes HOT's resident metadata and random indices. */
int divoom_hot_manage_clear_unused_batch(int *deleted_num)
{
    uint32 deleted_ids[DIVOOM_SUBSCRIBE_CLEAN_BATCH_SIZE];
    int result, i, j, write_pos = 0;
    Pdivoom_image_hot_item image_list;

    *deleted_num = 0;
    if(pdivoom_hot_manage_handle == NULL)
    {
        return 0;
    }
    divoom_system_get_lock_direct(pdivoom_hot_manage_handle->state_lock);
    if(pdivoom_hot_manage_handle->pending_list != NULL ||
        pdivoom_hot_manage_handle->pending_pump_running)
    {
        divoom_system_free_lock_direct(pdivoom_hot_manage_handle->state_lock);
        return 0;
    }
    result = divoom_database_manage_clear_unused_subscribe_batch(
        DIVOOM_DISP_SUBSCRIBE_MODE_HOT, deleted_ids, deleted_num);
    image_list = pdivoom_hot_manage_handle->pic_queue.image_list;
    for(i = 0; i < pdivoom_hot_manage_handle->pic_queue.image_num; i++)
    {
        for(j = 0; j < *deleted_num; j++)
        {
            if(image_list[i].image_id == deleted_ids[j])
            {
                break;
            }
        }
        if(j < *deleted_num)
        {
            divoom_hot_manage_mark_not_ready_locked(image_list[i].image_id);
            divoom_hot_manage_adjust_size_num(image_list[i].image_size, -1);
            divoom_system_free(image_list[i].image_name);
        }
        else
        {
            image_list[write_pos++] = image_list[i];
        }
    }
    pdivoom_hot_manage_handle->pic_queue.image_num = write_pos;
    divoom_hot_manage_rebuild_image_id_index_locked();
    divoom_system_free_lock_direct(pdivoom_hot_manage_handle->state_lock);
    return result;
}

static void divoom_hot_manage_remove_first_image_locked(int32 remove_num)
{
    int32 i;
    uint32 image_id;
    uint32 image_size;

    if(pdivoom_hot_manage_handle == NULL || remove_num <= 0)
    {
        return;
    }
    if(remove_num > pdivoom_hot_manage_handle->pic_queue.image_num)
    {
        remove_num = pdivoom_hot_manage_handle->pic_queue.image_num;
    }
    for(i = 0; i < remove_num; i++)
    {
        image_id = pdivoom_hot_manage_handle->pic_queue.image_list[i].image_id;
        image_size =
            pdivoom_hot_manage_handle->pic_queue.image_list[i].image_size;
        divoom_hot_manage_remove_image_id_index_locked(image_id);
        divoom_hot_manage_mark_not_ready_locked(image_id);
        divoom_database_manage_delete_subscribe_image_item(
            DIVOOM_HOT_IMAGE_TABLE_NAME, (int)image_size, (int)image_id);
        if(image_size != 0)
        {
            /* 同时清理旧版 image_type=0 的兼容记录。 */
            divoom_database_manage_delete_subscribe_image_item(
                DIVOOM_HOT_IMAGE_TABLE_NAME, 0, (int)image_id);
        }
        divoom_manage_image_del(DIVOOM_MANAGE_IMAGE_SUBSCRIBE_DEFAULT_16,
            &image_id, 1, 1);
        divoom_hot_manage_adjust_size_num(image_size, -1);
    }
    divoom_subscribe_local_pic_remove_first_image(
        pdivoom_hot_manage_handle->pic_queue.image_list,
        &pdivoom_hot_manage_handle->pic_queue.image_num, remove_num);
}

/* 仅在开机、HOT 下载尚未启动时调用：先删文件，再统一提交模块数据。 */
void divoom_hot_manage_clear_files_on_boot(void)
{
    int32 i, remove_num = 0;
    uint32 image_id, image_size;
    uint32 local_num;
    char file_name[128];

    if(pdivoom_hot_manage_handle == NULL)
    {
        return;
    }
    local_num = pdivoom_hot_manage_handle->local_ready_num;
    while(remove_num < pdivoom_hot_manage_handle->pic_queue.image_num &&
        local_num > 5 &&
        (uint32)divoom_port_fs_free_space(DIVOOM_USER_ROOT) <= DIVOOM_ROOM_WARNING_MIN)
    {
        image_id = pdivoom_hot_manage_handle->pic_queue.image_list[remove_num].image_id;
        divoom_manage_image_download_get_file_name(
            DIVOOM_MANAGE_IMAGE_SUBSCRIBE_DEFAULT_16, image_id, file_name);
        divoom_port_remove_file(file_name);
        if(divoom_hot_manage_find_ready_index_locked(image_id) >= 0)
            local_num--;
        remove_num++;
    }
    for(i = 0; i < remove_num; i++)
    {
        image_id = pdivoom_hot_manage_handle->pic_queue.image_list[i].image_id;
        image_size = pdivoom_hot_manage_handle->pic_queue.image_list[i].image_size;
        divoom_hot_manage_mark_not_ready_locked(image_id);
        divoom_hot_manage_adjust_size_num(image_size, -1);
        divoom_database_manage_delete_subscribe_image_item(
            DIVOOM_HOT_IMAGE_TABLE_NAME, -1, (int)image_id);
    }
    if(remove_num > 0)
    {
        divoom_subscribe_local_pic_remove_first_image(
            pdivoom_hot_manage_handle->pic_queue.image_list,
            &pdivoom_hot_manage_handle->pic_queue.image_num, remove_num);
        divoom_hot_manage_rebuild_image_id_index_locked();
    }
}




/*********************************************************************
  Function:        divoom_hot_manage_init
  Description:     初始化模块，将相关的文件从spi中load出来
  Input:           file_path  SD路径 
                    
  Return:           none
**********************************************************************/

void divoom_hot_manage_init(void)
{
    int32 i;
    int32 image_type;
    int32 pic_num = 0;
    int32 size_index;
    int32 total_pic_num = 0;
    int32 type_update_time;
    uint32 item_image_size;
    uint32 item_user_id;
    uint32 ready_image_id;
    char file_name[128];
    divoom_image_base_info *local_pic_list;
    Pdivoom_image_hot_item db_pic_list;
    if(pdivoom_hot_manage_handle)
    {
        return;
    }

    pdivoom_hot_manage_handle = divoom_system_malloc(sizeof(divoom_hot_manage_ctrl));
    if(pdivoom_hot_manage_handle == NULL)
    {
        return;
    }
    pdivoom_hot_manage_handle->local_ready_num = 0;
    pdivoom_hot_manage_handle->local_ready_lt_128_end = 0;
    pdivoom_hot_manage_handle->image_id_index_num = 0;
    pdivoom_hot_manage_handle->state_lock = divoom_system_create_lock();
    if(pdivoom_hot_manage_handle->state_lock == NULL)
    {
        divoom_system_free(pdivoom_hot_manage_handle);
        pdivoom_hot_manage_handle = NULL;
        return;
    }

    
	char *table_item = "image_id INT PRIMARY KEY     NOT NULL," \
		"image_type INTEGER DEFAULT 0," \
		"image_size INTEGER DEFAULT 0," \
		"author_id INTEGER DEFAULT 0," \
		"image_name CHAR(200)," \
		"file_addr CHAR(64)," \
		"update_time INTEGER DEFAULT 0";

	divoom_database_manage_add_new_table(DIVOOM_HOT_IMAGE_TABLE_NAME, table_item);
    divoom_database_manage_check_column_in_table(
        DIVOOM_HOT_IMAGE_TABLE_NAME, "image_size", "INTEGER DEFAULT 0");
    divoom_database_manage_add_table_index(DIVOOM_HOT_IMAGE_TABLE_NAME, "update_time");
    pdivoom_hot_manage_handle->pall_random_handle = NULL;
    pdivoom_hot_manage_handle->random_max_size = 0;
    /* image_type 保存图片尺寸；先兼容读取旧版 image_type=0，再读取五个尺寸档。 */
    for(size_index = -1; size_index < DIVOOM_HOT_MANAGE_SIZE_TYPE_NUM; size_index++)
    {
        image_type = size_index < 0 ? 0:(int32)g_divoom_hot_manage_size_list[size_index];
        pic_num = 0;
        db_pic_list = divoom_database_manage_get_subscribe_image_pic_list(DIVOOM_HOT_IMAGE_TABLE_NAME, image_type, &pic_num);
        type_update_time = divoom_database_manage_get_subscribe_image_last_update_time(  DIVOOM_HOT_IMAGE_TABLE_NAME, image_type);
        if(type_update_time > pdivoom_hot_manage_handle->update_time)
        {
            pdivoom_hot_manage_handle->update_time = type_update_time;
        }
        if(db_pic_list == NULL || pic_num <= 0)
        {
            if(db_pic_list != NULL)
            {
                divoom_subscribe_local_pic_free_image_list(db_pic_list, pic_num);
            }
            continue;
        }
        if(pic_num > 0x7fffffff - pdivoom_hot_manage_handle->pic_queue.image_num ||divoom_hot_manage_reserve_image_list(&pdivoom_hot_manage_handle->pic_queue,
                pdivoom_hot_manage_handle->pic_queue.image_num + pic_num) == 0)
        {
            divoom_print("hot image list malloc failed count=%d\n", pic_num);
            divoom_subscribe_local_pic_free_image_list(db_pic_list, pic_num);
            continue;
        }
        for(i = 0; i < pic_num; i++)
        {
            item_user_id = db_pic_list[i].user_id;
            item_image_size = db_pic_list[i].image_size;
            if(item_image_size == 0)
            {
                item_image_size = size_index < 0 ? DIVOOM_HOT_MANAGE_IMAGE_SIZE_16 : (uint32)image_type;
            }
            if(size_index < 0 &&divoom_hot_manage_get_size_index(item_user_id) >= 0)
            {
                /* 兼容上一版把尺寸暂存在 author_id 的记录；该记录没有用户 ID。 */
                item_image_size = item_user_id;
                item_user_id = 0;
            }
            if(divoom_hot_manage_append_image(   &pdivoom_hot_manage_handle->pic_queue,    item_image_size, db_pic_list[i].image_id, item_user_id == 0 ? "" : db_pic_list[i].image_name, item_user_id) == 0)
            {
                divoom_print("hot image item malloc failed index=%d\n", i);
                break;
            }
            divoom_hot_manage_adjust_size_num(item_image_size, 1);
            total_pic_num++;
        }
        divoom_subscribe_local_pic_free_image_list(db_pic_list, pic_num);
    }

    if(pdivoom_hot_manage_handle->pic_queue.image_num >   DIVOOM_HOT_MANAGE_MAX_FILE_NUM)
    {
        divoom_hot_manage_remove_first_image_locked(   pdivoom_hot_manage_handle->pic_queue.image_num -    DIVOOM_HOT_MANAGE_MAX_FILE_NUM);
    }
    total_pic_num = pdivoom_hot_manage_handle->pic_queue.image_num;

    local_pic_list = divoom_hot_manage_load_local_pic(&pic_num); 
    /* 库里没有 HOT 记录时，把目录扫到的本地文件按 16 档写入队列和数据库，避免开机后队列一直是空的黑屏。 */
    if(total_pic_num == 0)
    {
        if(local_pic_list != NULL && pic_num > 0 && divoom_hot_manage_reserve_image_list(&pdivoom_hot_manage_handle->pic_queue, pic_num))
        {
            for(i = 0; i < pic_num; i++)
            {
                if(divoom_hot_manage_append_image(&pdivoom_hot_manage_handle->pic_queue, DIVOOM_HOT_MANAGE_IMAGE_SIZE_16, local_pic_list[i].image_id, "", 0) == 0)
                    break;
                divoom_hot_manage_adjust_size_num(DIVOOM_HOT_MANAGE_IMAGE_SIZE_16, 1);
                divoom_database_manage_add_subscribe_image_item_with_size(DIVOOM_HOT_IMAGE_TABLE_NAME, DIVOOM_HOT_MANAGE_IMAGE_SIZE_16, DIVOOM_HOT_MANAGE_IMAGE_SIZE_16, local_pic_list[i].image_id, 0, "", 0, "");
            }
        }
    }
    if(local_pic_list)
        divoom_system_free(local_pic_list);

    /* 队列来自数据库；文件在 /userdata/divoom/12/<id>.bin 才登记。尺寸仍以库为准。 */
    if(pdivoom_hot_manage_handle->pic_queue.image_num > 0)
    {
        for(i = 0; i < pdivoom_hot_manage_handle->pic_queue.image_num; i++)
        {
            item_image_size = pdivoom_hot_manage_handle->pic_queue.image_list[i].image_size;
            if(divoom_hot_manage_get_size_index(item_image_size) < 0)
            {
                continue;
            }
            ready_image_id = pdivoom_hot_manage_handle->pic_queue.image_list[i].image_id;
            divoom_manage_image_download_get_file_name(DIVOOM_MANAGE_IMAGE_SUBSCRIBE_DEFAULT_16, ready_image_id, file_name);
            if(divoom_port_file_exists(file_name) == 0)
            {
                continue;
            }
            divoom_system_get_lock_direct(pdivoom_hot_manage_handle->state_lock);
            divoom_hot_manage_mark_ready_locked(ready_image_id);
            divoom_system_free_lock_direct(pdivoom_hot_manage_handle->state_lock);
        }
    }
    divoom_hot_manage_rebuild_image_id_index_locked();
#if defined(DIVOOM_SIMULATOR)
    divoom_hot_manage_sim_memory_verify();
#endif
}

/*********************************************************************
  Function:        divoom_hot_manage_check_image_id_exist
  Description:     检查这个文件是否已经存在
  Input:           pimage_list
                   
                    
  Return:          返回图片ＩＤ
**********************************************************************/

int divoom_hot_manage_check_image_id_exist(Pdivoom_hot_manage_item pcur_item, int image_id)
{
    int32 i;

    if(pcur_item == NULL || pcur_item->image_num <= 0)
    {
        return 0;
    }
    if(pdivoom_hot_manage_handle != NULL &&
        pcur_item == &pdivoom_hot_manage_handle->pic_queue &&
        pdivoom_hot_manage_handle->image_id_index_num == pcur_item->image_num)
    {
        return divoom_hot_manage_find_image_id_index_locked(
            (uint32)image_id) >= 0;
    }
    for(i = 0; i < pcur_item->image_num; i++)
    {
        if(pcur_item->image_list[i].image_id == image_id)
        {
            return 1;
        }
    }
    return 0;
}

/* 先登记整批元数据，保证作者回包到达时所有同 USERID 图片都可被更新。 */
static int32 divoom_hot_manage_add_file_metadata(
    Pdivoom_devcice_image_file pfile)
{
    int32 i;
    int32 ready_index;
    int32 need_size_update = 0;
    Pdivoom_image_hot_item image_item;
    char author_name[200] = {0};

    if(pdivoom_hot_manage_handle == NULL || pfile == NULL)
    {
        return 0;
    }

    divoom_system_get_lock_direct(pdivoom_hot_manage_handle->state_lock);
    if(divoom_hot_manage_check_image_id_exist(
        &pdivoom_hot_manage_handle->pic_queue, pfile->image_id))
    {
        /* 已有记录但文件没落盘时仍返回 1，submit 会把它放进下载队列。 */
        ready_index = divoom_hot_manage_find_ready_index_locked((uint32)pfile->image_id);
        if(ready_index < 0 && divoom_hot_manage_get_size_index((uint32)pfile->size) >= 0)
        {
            image_item = divoom_hot_manage_find_image_locked((uint32)pfile->image_id, NULL);
            if(image_item != NULL && image_item->image_size != (uint32)pfile->size)
            {
                /* 旧尺寸留在库里时，下次开机 mark_ready 对不上，文件在盘上也不进可用列表。 */
                divoom_hot_manage_adjust_size_num(image_item->image_size, -1);
                image_item->image_size = (uint32)pfile->size;
                divoom_hot_manage_adjust_size_num(image_item->image_size, 1);
                need_size_update = 1;
            }
        }
        divoom_system_free_lock_direct(pdivoom_hot_manage_handle->state_lock);
        if(need_size_update)
        {
            divoom_database_manage_update_subscribe_image_size(DIVOOM_HOT_IMAGE_TABLE_NAME, pfile->image_id, pfile->size, pfile->size);
        }
        return ready_index < 0;
    }
    /* 相同 USERID 直接复用已保存作者名，不在本模块维护额外作者缓存。 */
    for(i = 0; i < pdivoom_hot_manage_handle->pic_queue.image_num; i++)
    {
        if(pdivoom_hot_manage_handle->pic_queue.image_list[i].user_id ==
                (uint32)pfile->user_id &&
            pdivoom_hot_manage_handle->pic_queue.image_list[i].image_name != NULL &&
            pdivoom_hot_manage_handle->pic_queue.image_list[i].image_name[0] != '\0')
        {
            strcpy(author_name,
                pdivoom_hot_manage_handle->pic_queue.image_list[i].image_name);
            break;
        }
    }
    if(pdivoom_hot_manage_handle->pic_queue.image_num >=
        DIVOOM_HOT_MANAGE_MAX_FILE_NUM)
    {
        /* 保留原业务策略：达到 300 条时一次淘汰最旧的 20 条。 */
        divoom_hot_manage_remove_first_image_locked(20);
    }
    /* 四个核心字段分别保存，image_name 的业务含义固定为作者名称。 */
    if(divoom_hot_manage_append_image(
        &pdivoom_hot_manage_handle->pic_queue,
        (uint32)pfile->size,
        pfile->image_id,
        author_name,
        (uint32)pfile->user_id) == 0)
    {
        divoom_system_free_lock_direct(pdivoom_hot_manage_handle->state_lock);
        divoom_print("hot image append failed image_id=%u\n", pfile->image_id);
        return 0;
    }
    divoom_hot_manage_add_image_id_index_locked(pfile->image_id);
    divoom_hot_manage_adjust_size_num((uint32)pfile->size, 1);
    if(pfile->utc_time > pdivoom_hot_manage_handle->update_time)
    {
        pdivoom_hot_manage_handle->update_time = pfile->utc_time;
    }
    divoom_system_free_lock_direct(pdivoom_hot_manage_handle->state_lock);

    divoom_database_manage_add_subscribe_image_item_with_size(
        DIVOOM_HOT_IMAGE_TABLE_NAME, pfile->size, pfile->size, pfile->image_id,
        pfile->utc_time, pfile->file_id, pfile->user_id, author_name);
    return 1;
}

/* 下载仍按水位逐项排队，避免整份响应同时复制到下载模块。 */
static void divoom_hot_manage_queue_file_download(
    Pdivoom_devcice_image_file pfile)
{
    divoom_manage_image_item_info image_item = {0};

    if(pfile == NULL)
    {
        return;
    }
    image_item.image_id = pfile->image_id;
    image_item.utc_time = pfile->utc_time;
    strcpy(image_item.file_addr, pfile->file_id);
    divoom_manage_image_add(DIVOOM_MANAGE_IMAGE_SUBSCRIBE_DEFAULT_16,
        &image_item, 1);
}

/* 兼容直接更新入口：元数据成功登记后再安排该图片下载。 */
static void divoom_hot_manage_process_file(Pdivoom_devcice_image_file pfile)
{
    if(divoom_hot_manage_add_file_metadata(pfile))
    {
        divoom_hot_manage_queue_file_download(pfile);
    }
}

void divoom_hot_manage_update_author_list(
    Pdivoom_device_get_user_info_list_respone_info puser_info)
{
    int32 i;
    int32 j;
    char *author_name_copy;

    if(pdivoom_hot_manage_handle == NULL || puser_info == NULL)
    {
        return;
    }
    for(i = 0; i < puser_info->user_num; i++)
    {
        if(puser_info->puser_list[i].user_id == 0)
        {
            continue;
        }
        divoom_system_get_lock_direct(pdivoom_hot_manage_handle->state_lock);
        for(j = 0; j < pdivoom_hot_manage_handle->pic_queue.image_num; j++)
        {
            if(pdivoom_hot_manage_handle->pic_queue.image_list[j].user_id !=
                puser_info->puser_list[i].user_id)
            {
                continue;
            }
            author_name_copy = divoom_hot_manage_copy_author_name(
                puser_info->puser_list[i].user_name);
            if(author_name_copy == NULL)
            {
                continue;
            }
            divoom_system_free(
                pdivoom_hot_manage_handle->pic_queue.image_list[j].image_name);
            pdivoom_hot_manage_handle->pic_queue.image_list[j].image_name =
                author_name_copy;
        }
        divoom_system_free_lock_direct(pdivoom_hot_manage_handle->state_lock);
    }
    divoom_database_manage_txn_begin();
    for(i = 0; i < puser_info->user_num; i++)
    {
        if(puser_info->puser_list[i].user_id == 0)
        {
            continue;
        }
        /* image_name 在数据库中同样固定表示作者名，按 USERID 批量补写。 */
        divoom_database_manage_update_subscribe_image_name_by_user_id(
            DIVOOM_HOT_IMAGE_TABLE_NAME,
            (int)puser_info->puser_list[i].user_id,
            puser_info->puser_list[i].user_name);
    }
    divoom_database_manage_txn_end();
}

void divoom_hot_manage_update_hot_list(Pdivoom_devcice_image_list pimage_list)
{
    int32 i;
    int32 user_num;

    if(pimage_list == NULL)
    {
        return;
    }
    divoom_database_manage_txn_begin();
    for(i = 0; i < pimage_list->num; i++)
    {
        divoom_hot_manage_process_file(&pimage_list->pfile_list[i]);
    }
    divoom_database_manage_txn_end();
}

int divoom_hot_manage_submit_hot_list(Pdivoom_devcice_image_list pimage_list,
    int32 *add_file_num)
{
    int32 i;
    int32 source_num;
    int32 download_num = 0;

    if(add_file_num != NULL)
    {
        *add_file_num = 0;
    }
    if(pdivoom_hot_manage_handle == NULL || pimage_list == NULL)
    {
        return 0;
    }
    divoom_system_get_lock_direct(pdivoom_hot_manage_handle->state_lock);
    if(pdivoom_hot_manage_handle->pending_list != NULL ||
        pdivoom_hot_manage_handle->pending_pump_running)
    {
        divoom_system_free_lock_direct(pdivoom_hot_manage_handle->state_lock);
        return 0;
    }
    pdivoom_hot_manage_handle->pending_list = pimage_list;
    pdivoom_hot_manage_handle->pending_index = 0;
    /* 登记整批元数据期间阻止下载完成回调抢先启动 pump。 */
    pdivoom_hot_manage_handle->pending_pump_running = 1;
    divoom_system_free_lock_direct(pdivoom_hot_manage_handle->state_lock);

    source_num = pimage_list->num;
    /* 原响应块原地压缩为待下载项，不再为下载队列复制整份列表。 */
    divoom_database_manage_txn_begin();
    for(i = 0; i < source_num; i++)
    {
        if(divoom_hot_manage_add_file_metadata(&pimage_list->pfile_list[i]))
        {
            if(download_num != i)
            {
                pimage_list->pfile_list[download_num] =
                    pimage_list->pfile_list[i];
            }
            download_num++;
        }
    }
    divoom_database_manage_txn_end();
    pimage_list->num = download_num;
    if(add_file_num != NULL)
    {
        *add_file_num = download_num;
    }

    divoom_system_get_lock_direct(pdivoom_hot_manage_handle->state_lock);
    pdivoom_hot_manage_handle->pending_pump_running = 0;
    if(download_num == 0)
    {
        pdivoom_hot_manage_handle->pending_list = NULL;
        pdivoom_hot_manage_handle->pending_index = 0;
    }
    divoom_system_free_lock_direct(pdivoom_hot_manage_handle->state_lock);

    divoom_print("[hot_mem] accept total=%d download=%d  free=%ldKB largest=%ldKB\n",
        source_num, download_num,  divoom_system_get_ram_free_size(),
        divoom_system_get_ram_largest_free_block_size());
    if(download_num == 0)
    {
        divoom_system_free(pimage_list);
        return 1;
    }
    /* HOT 按水位分批入队；每一小批由 pump 独立封口并唤醒下载线程。 */
    divoom_hot_manage_pump_pending();
    return 1;
}

void divoom_hot_manage_pump_pending(void)
{
    int32 remaining_num;
    Pdivoom_devcice_image_list release_list;
    divoom_devcice_image_file image_file;

    if(pdivoom_hot_manage_handle == NULL)
    {
        return;
    }
    divoom_system_get_lock_direct(pdivoom_hot_manage_handle->state_lock);
    if(pdivoom_hot_manage_handle->pending_pump_running ||
        pdivoom_hot_manage_handle->pending_list == NULL)
    {
        divoom_system_free_lock_direct(pdivoom_hot_manage_handle->state_lock);
        return;
    }
    pdivoom_hot_manage_handle->pending_pump_running = 1;
    divoom_system_free_lock_direct(pdivoom_hot_manage_handle->state_lock);

    /*
     * batch_depth 只保护本次小批量入队。若跨越整个 pending_list，
     * 下载线程会等待 depth 归零，而后续 HOT 又要等待队列被下载腾空，形成互等。
     */
    divoom_manage_image_transfer_batch_begin();
    while(divoom_manage_image_get_pending_num() <
        DIVOOM_HOT_MANAGE_DOWNLOAD_LOW_WATERMARK)
    {
        release_list = NULL;
        divoom_system_get_lock_direct(pdivoom_hot_manage_handle->state_lock);
        if(pdivoom_hot_manage_handle->pending_list == NULL ||
            pdivoom_hot_manage_handle->pending_index >=
                pdivoom_hot_manage_handle->pending_list->num)
        {
            release_list = pdivoom_hot_manage_handle->pending_list;
            pdivoom_hot_manage_handle->pending_list = NULL;
            pdivoom_hot_manage_handle->pending_index = 0;
            pdivoom_hot_manage_handle->pending_pump_running = 0;
            divoom_system_free_lock_direct(pdivoom_hot_manage_handle->state_lock);
            if(release_list)
            {
                divoom_system_free(release_list);
            }
            divoom_manage_image_transfer_batch_end();
            return;
        }
        image_file = pdivoom_hot_manage_handle->pending_list->pfile_list[
            pdivoom_hot_manage_handle->pending_index];
        pdivoom_hot_manage_handle->pending_index++;
        remaining_num = pdivoom_hot_manage_handle->pending_list->num -
            pdivoom_hot_manage_handle->pending_index;
        if(remaining_num == 0)
        {
            release_list = pdivoom_hot_manage_handle->pending_list;
            pdivoom_hot_manage_handle->pending_list = NULL;
            pdivoom_hot_manage_handle->pending_index = 0;
        }
        divoom_system_free_lock_direct(pdivoom_hot_manage_handle->state_lock);

        divoom_hot_manage_queue_file_download(&image_file);
        if(release_list)
        {
            divoom_system_free(release_list);
        }
    }

    divoom_system_get_lock_direct(pdivoom_hot_manage_handle->state_lock);
    if(pdivoom_hot_manage_handle->pending_list != NULL)
    {
        remaining_num = pdivoom_hot_manage_handle->pending_list->num -
            pdivoom_hot_manage_handle->pending_index;
    }
    else
    {
        remaining_num = 0;
    }
    pdivoom_hot_manage_handle->pending_pump_running = 0;
    divoom_system_free_lock_direct(pdivoom_hot_manage_handle->state_lock);
    divoom_manage_image_transfer_batch_end();
    divoom_print("[hot_mem] batch queue=%u remaining=%d free=%ldKB largest=%ldKB\n",
        divoom_manage_image_get_pending_num(), remaining_num,
        divoom_system_get_ram_free_size(),
        divoom_system_get_ram_largest_free_block_size());
}

/*********************************************************************
  Function:        divoom_hot_manage_request_hot_image
  Description:     向服务器请请求ＨＯＴ
  Input:           image_size  DIVOOM_HOT_MANAGE_PIC_16之类
                   
                    
  Return:          返回图片ＩＤ
**********************************************************************/

int divoom_hot_manage_request_hot_image(int image_size, int wait_flag)
{
    int modify_flag = 0;
    divoom_devcice_hot_list_request hot_info;
    Pdivoom_net_command pcmd_info;
    
    
    hot_info.size = DIVOOM_HOT_MANAGE_IMAGE_SIZE_128 + DIVOOM_HOT_MANAGE_IMAGE_SIZE_256;
    divoom_system_get_lock_direct(pdivoom_hot_manage_handle->state_lock);
    hot_info.utc_time = pdivoom_hot_manage_handle->update_time;
    divoom_system_free_lock_direct(pdivoom_hot_manage_handle->state_lock);

    if(wait_flag)
    {
        pcmd_info = (Pdivoom_net_command)divoom_net_app_send_packet_wait(DIVOOM_NET_COMM_GET_HOT_LIST, DIVOOM_SERVER_TOPIC, &hot_info);
        if(pcmd_info)
        {
            if(divoom_hot_manage_submit_hot_list(
                (Pdivoom_devcice_image_list)pcmd_info->cmd_data, NULL))
            {
                /* 响应块所有权已转移，网络包释放时不能再次释放。 */
                pcmd_info->cmd_data = NULL;
            }
            divoom_net_app_free_packet(pcmd_info);
        }
    }
    else
    {
        divoom_net_app_send_packet(DIVOOM_NET_COMM_GET_HOT_LIST, DIVOOM_SERVER_TOPIC, &hot_info);
    }
    return modify_flag;
}


/*********************************************************************
  Function:        divoom_hot_manage_get_random_pic_id
  Description:     从 HOT 图片中随机取得图片 ID
  Input:           user_id     返回所选图片的作者 USERID
                   image_name  作者名称输出缓冲区
                   max_size    SIZE_128 只选 128 档；其它值表示小于等于该上限
                    
  Return:          返回图片ＩＤ
**********************************************************************/

uint32 divoom_hot_manage_get_random_pic_id(uint32 *user_id, char *image_name, uint32 max_size)
{
    int request_flag = 0;
    int sys_run_time_s;
    uint32 i = 0;
    uint32 image_id = 0;
    uint32 eligible_num;
    uint32 local_ready_num;
    uint32 random_index, test_raw_index = 0, test_find_index = 0;
    struct tms sys_run_time;

    if(user_id != NULL)
    {
        *user_id = 0;
    }
    if(image_name != NULL)
    {
        image_name[0] = '\0';
    }
    
    if(pdivoom_hot_manage_handle == NULL)
    {
        return 0;
    }

    #if 1
    divoom_system_get_lock_direct(pdivoom_hot_manage_handle->state_lock);
    eligible_num = divoom_hot_manage_get_local_eligible_num_locked(max_size);
    /* 没有符合尺寸的图时不等 30 分钟，只立刻要一次；记下 last_request_time 后仍按 6 小时再要。 */
    if(eligible_num == 0 && pdivoom_hot_manage_handle->last_request_time == 0)
    {
        request_flag = 1;
		/* 库里的 update_time 已经不小于服务器，增量 Time 会返回空列表，这次按 0 要全量。 */
		pdivoom_hot_manage_handle->update_time = 0;
		pdivoom_hot_manage_handle->last_request_time = time(0);
    }
    else if(pdivoom_hot_manage_handle->last_request_time == 0)
    {
        times(&sys_run_time);
        sys_run_time_s = divoom_system_get_ticks();
        if(sys_run_time_s > DIVOOM_HOT_MANAGE_FIRST_LOAD_FILE_TIME)
        {
            request_flag = 1;
            pdivoom_hot_manage_handle->last_request_time = time(0);
        }
    }
    else if(pdivoom_hot_manage_handle->last_request_time +
        DIVOOM_HOT_MANAGE_LOAD_FILE_TIME < time(0))
    {
        request_flag = 1;
        pdivoom_hot_manage_handle->last_request_time = time(0);
    }
    divoom_system_free_lock_direct(pdivoom_hot_manage_handle->state_lock);

    if(request_flag)
    {
        divoom_hot_manage_request_hot_image(0, 0);
    }
    #endif

    divoom_system_get_lock_direct(pdivoom_hot_manage_handle->state_lock);
    /* 按本次候选数量维护随机池，计数和取图使用同一份加锁状态。 */
    eligible_num = divoom_hot_manage_get_local_eligible_num_locked(max_size);
    local_ready_num = pdivoom_hot_manage_handle->local_ready_num;
    if(pdivoom_hot_manage_handle->pall_random_handle == NULL && eligible_num > 0)
    {
        /* 一次预留最大位图容量，后续增删只调整有效范围和状态位。 */
        pdivoom_hot_manage_handle->pall_random_handle =
            divoom_random_create(DIVOOM_HOT_MANAGE_MAX_FILE_NUM);
        if(pdivoom_hot_manage_handle->pall_random_handle != NULL)
        {
            divoom_random_set_max(pdivoom_hot_manage_handle->pall_random_handle, (int)eligible_num);
            pdivoom_hot_manage_handle->random_max_size = max_size;
        }
    }
    else if(pdivoom_hot_manage_handle->pall_random_handle != NULL &&
        pdivoom_hot_manage_handle->random_max_size != max_size)
    {
        /* 切换筛选模式才开启新轮次；列表增删由 ready 路径保留本轮状态。 */
        divoom_random_set_max(pdivoom_hot_manage_handle->pall_random_handle, 0);
        divoom_random_set_max(pdivoom_hot_manage_handle->pall_random_handle, (int)eligible_num);
        pdivoom_hot_manage_handle->random_max_size = max_size;
    }
    if(pdivoom_hot_manage_handle->pall_random_handle != NULL)
    {
        if(eligible_num > 0)
        {
            random_index = (uint32)divoom_random_get(pdivoom_hot_manage_handle->pall_random_handle);
            test_raw_index = random_index;
            for(i = 0; i < pdivoom_hot_manage_handle->local_ready_num; i++)
            {
                Pdivoom_image_hot_item selected_item;

                if(!divoom_hot_manage_size_matches(
                    pdivoom_hot_manage_handle->local_ready_list[i].image_size, max_size))
                {
                    continue;
                }
                if(random_index > 0)
                {
                    random_index--;
                    continue;
                }
                selected_item = divoom_hot_manage_find_image_locked(
                    pdivoom_hot_manage_handle->local_ready_list[i].image_id, &test_find_index);
                if(selected_item == NULL)
                {
                    break;
                }
                if(image_name != NULL && selected_item->image_name != NULL)
                {
                    strcpy(image_name, selected_item->image_name);
                }
                image_id = selected_item->image_id;
                if(user_id != NULL)
                {
                    *user_id = selected_item->user_id;
                }
                break;
            }
        }
    }
    divoom_system_free_lock_direct(pdivoom_hot_manage_handle->state_lock);
    divoom_print("random test image: %d, %d, %d, %d, %d, %d\n", image_id, test_raw_index, test_find_index, eligible_num, local_ready_num, i);
    return image_id;
}


/*********************************************************************
  Function:        divoom_hot_manage_get_url_addr
  Description:     获取指定文件的网络地址
  Input:           model_id
                   image_id
                   url_addr
                    
  Return:          image_flag
**********************************************************************/

int divoom_hot_manage_get_url_addr(int model_id, uint32 image_id, char *url_addr)
{
    int image_flag = divoom_database_manage_get_subscribe_image_pic_addr(DIVOOM_HOT_IMAGE_TABLE_NAME, image_id, url_addr);

    if(strlen(url_addr) == 0)
    {
        divoom_manage_image_del(DIVOOM_MANAGE_IMAGE_SUBSCRIBE_DEFAULT_16,
            &image_id, 1, 1);
        divoom_hot_manage_update_local_ready(image_id, 0);
    }
    return image_flag;
}

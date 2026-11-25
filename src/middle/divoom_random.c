#include "divoom.h"


typedef struct _divoom_random_ctrl
{
    int no_use_num;     //未使用个数
    int capacity;       //当前位图可容纳的数量
    int max;            //随机数最大值
    int map_num;        //map_use_info 使用的个数
    int last_index;     //上一次选中的项，用于避免跨轮连续重复
    uint8   *map_use_info;   //use_map中uint32激活的个数
    uint32 *use_map;    //随机数据位图
}divoom_random_ctrl, *Pdivoom_random_ctrl;

void *divoom_random_create(int max)
{
    int map_num;
    Pdivoom_random_ctrl phandle;

    map_num = (max + 31)/32;
    phandle = divoom_system_malloc(sizeof(divoom_random_ctrl) + map_num  * sizeof(uint32) + map_num + 8);
    if(phandle == NULL)
    {
        return NULL;
    }
    phandle->capacity = max;
    phandle->max = max;
    phandle->no_use_num = max;
    phandle->map_num = map_num;
    phandle->last_index = -1;
    phandle->use_map = (uint32 *)&phandle[1];
    phandle->map_use_info = (uint8 *)&phandle->use_map[map_num];
    memset(phandle->use_map, 0xFF, map_num  * sizeof(uint32));
    memset(phandle->map_use_info, 32, map_num);
    if(max%32)
    {
        phandle->map_use_info[map_num - 1] = max%32;
    }
    if(max % 32)
    {
        phandle->use_map[map_num - 1] = (1U << (max % 32)) - 1U;
    }
    return phandle;
}

/* 以下更新接口由调用方串行化；1 表示本轮尚未选中。 */
static int divoom_random_is_unused(Pdivoom_random_ctrl phandle, int index)
{
    return (phandle->use_map[index / 32] & (1U << (index % 32))) != 0;
}

static void divoom_random_set_unused(Pdivoom_random_ctrl phandle, int index, int unused)
{
    int old = divoom_random_is_unused(phandle, index);
    if(old == unused)
    {
        return;
    }
    if(unused)
    {
        phandle->use_map[index / 32] |= 1U << (index % 32);
        phandle->map_use_info[index / 32]++;
        phandle->no_use_num++;
    }
    else
    {
        phandle->use_map[index / 32] &= ~(1U << (index % 32));
        phandle->map_use_info[index / 32]--;
        phandle->no_use_num--;
    }
}

/* 调整尾部范围并按需扩容；保留已有下标的本轮状态和句柄地址。 */
int divoom_random_set_max(void *handle, int max)
{
    Pdivoom_random_ctrl phandle = (Pdivoom_random_ctrl)handle;
    int i;
    if(phandle == NULL || max < 0)
    {
        return 0;
    }
    if(max > phandle->capacity)
    {
        int map_num = max / 32 + (max % 32 != 0);
        uint32 *use_map;
        uint8 *map_use_info;

        /* 先分配并复制，失败时原容量、位图和未选数量均保持不变。 */
        use_map = divoom_system_malloc((uint32)map_num * (sizeof(uint32) + sizeof(uint8)));
        if(use_map == NULL)
        {
            return 0;
        }
        map_use_info = (uint8 *)&use_map[map_num];
        memset(use_map, 0, (uint32)map_num * (sizeof(uint32) + sizeof(uint8)));
        memcpy(use_map, phandle->use_map, (uint32)phandle->map_num * sizeof(uint32));
        memcpy(map_use_info, phandle->map_use_info, (uint32)phandle->map_num * sizeof(uint8));
        /* 初始位图内嵌在句柄尾部；只有后续扩容的独立位图需要单独释放。 */
        if(phandle->use_map != (uint32 *)&phandle[1])
        {
            divoom_system_free(phandle->use_map);
        }
        phandle->use_map = use_map;
        phandle->map_use_info = map_use_info;
        phandle->map_num = map_num;
        phandle->capacity = max;
    }
    for(i = max; i < phandle->max; i++)
    {
        divoom_random_set_unused(phandle, i, 0);
    }
    for(i = phandle->max; i < max; i++)
    {
        divoom_random_set_unused(phandle, i, 1);
    }
    phandle->max = max;
    if(phandle->last_index >= max)
    {
        phandle->last_index = -1;
    }
    return 1;
}

/* 插入未选项，后续项的本轮状态随列表右移。 */
int divoom_random_insert(void *handle, int index)
{
    Pdivoom_random_ctrl phandle = (Pdivoom_random_ctrl)handle;
    int i;
    if(phandle == NULL || index < 0 || index > phandle->max ||
        phandle->max >= phandle->capacity)
    {
        return 0;
    }
    for(i = phandle->max; i > index; i--)
    {
        divoom_random_set_unused(phandle, i,
            divoom_random_is_unused(phandle, i - 1));
    }
    divoom_random_set_unused(phandle, index, 1);
    if(phandle->last_index >= index)
    {
        phandle->last_index++;
    }
    phandle->max++;
    return 1;
}

/* 删除指定项，后续项的本轮状态随列表左移。 */
int divoom_random_remove(void *handle, int index)
{
    Pdivoom_random_ctrl phandle = (Pdivoom_random_ctrl)handle;
    int i;
    if(phandle == NULL || index < 0 || index >= phandle->max)
    {
        return 0;
    }
    for(i = index; i + 1 < phandle->max; i++)
    {
        divoom_random_set_unused(phandle, i,
            divoom_random_is_unused(phandle, i + 1));
    }
    divoom_random_set_unused(phandle, phandle->max - 1, 0);
    if(phandle->last_index == index)
    {
        phandle->last_index = -1;
    }
    else if(phandle->last_index > index)
    {
        phandle->last_index--;
    }
    phandle->max--;
    return 1;
}

int divoom_random_get_no_use(void *handle)
{
    Pdivoom_random_ctrl phandle = (Pdivoom_random_ctrl)handle;

    if(phandle == NULL)
    {
        return 0;
    }
    return phandle->no_use_num;
}

int divoom_random_get(void *handle)
{
    int i, j;
    int rand_num;
    int excluded_index = -1;
    Pdivoom_random_ctrl phandle = (Pdivoom_random_ctrl)handle;

    if(phandle == NULL)
    {
        return 0;
    }
    if(phandle->max <= 0)
    {
        return -1;
    }
    if(phandle->no_use_num == 0)
    {
        for(i = 0; i < phandle->max; i++)
        {
            divoom_random_set_unused(phandle, i, 1);
        }
        /* 暂时排除上轮末项，选完首项后放回，仍保证每轮所有项各选一次。 */
        if(phandle->max > 1 && phandle->last_index >= 0 && phandle->last_index < phandle->max)
        {
            excluded_index = phandle->last_index;
            divoom_random_set_unused(phandle, excluded_index, 0);
        }
    }
    rand_num = rand()%phandle->no_use_num;
    for(i = 0; i < phandle->map_num; i ++)
    {
        if(rand_num < phandle->map_use_info[i])
        {
            for(j = 0; j < 32; j ++)
            {
                if(phandle->use_map[i] & (1U << j))
                {
                    rand_num --;
                    if(rand_num < 0)
                    {
                        phandle->use_map[i] &= ~(1U << j);
                        phandle->map_use_info[i] --;
                        phandle->no_use_num --;
                        if(excluded_index >= 0)
                        {
                            divoom_random_set_unused(phandle, excluded_index, 1);
                        }
                        phandle->last_index = i *32 + j;
                        return i *32 + j;
                    }
                }
            }
            break;
        }
        rand_num -= phandle->map_use_info[i];
    }
    if(excluded_index >= 0)
    {
        divoom_random_set_unused(phandle, excluded_index, 1);
    }
    return 0;
}

void divoom_random_destroy(void *handle)
{
    Pdivoom_random_ctrl phandle = (Pdivoom_random_ctrl)handle;
    if(phandle != NULL && phandle->use_map != (uint32 *)&phandle[1])
    {
        divoom_system_free(phandle->use_map);
    }
    divoom_system_free(handle);
}


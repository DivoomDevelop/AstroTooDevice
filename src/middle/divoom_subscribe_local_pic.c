#include "divoom.h"
#include "divoom_subscribe_local_pic.h"
#include "divoom_manage_image.h"

#include <stdio.h>
#include <string.h>
#if defined(_MSC_VER)
#ifndef strcasecmp
#define strcasecmp _stricmp
#endif
#else
#include <strings.h>
#endif

static int divoom_subscribe_local_pic_is_bin_name(const char *name, uint32 *image_id)
{
    const char *dot;

    if (name == NULL || image_id == NULL)
    {
        return 0;
    }
    dot = strrchr(name, '.');
    if (dot == NULL || strcasecmp(dot, ".bin") != 0)
    {
        return 0;
    }
    if (sscanf(name, "%u.", image_id) != 1)
    {
        return 0;
    }
    return 1;
}

static int divoom_subscribe_local_pic_count_bins(int model_id)
{
    char base_dir[256];
    DIR *dir;
    const divoom_port_dirent_t *ent;
    int count = 0;

    divoom_manage_image_download_get_path_name((uint32)model_id, 1, base_dir);
    dir = divoom_port_dir_open(base_dir);
    if (dir == NULL)
    {
        return 0;
    }
    while ((ent = divoom_port_dir_read(dir)) != NULL)
    {
        uint32 image_id;

        if (ent->is_dir)
        {
            continue;
        }
        if (divoom_subscribe_local_pic_is_bin_name(ent->name, &image_id))
        {
            count++;
        }
    }
    divoom_port_dir_close(dir);
    return count;
}

static void divoom_subscribe_local_pic_seed_if_empty(int model_id)
{
    static const int k_fallback_models[] = { 15, 22, 36, 29, 28, 17, 0 };
    char dst_dir[256];
    char src_dir[256];
    int i;

    if (divoom_subscribe_local_pic_count_bins(model_id) > 0)
    {
        return;
    }

    #if 0
    for (i = 0; k_fallback_models[i] != 0; i++)
    {
        if (divoom_subscribe_local_pic_count_bins(k_fallback_models[i]) > 0)
        {
            divoom_manage_image_download_get_path_name((uint32)model_id, 0, dst_dir);
            divoom_manage_image_download_get_path_name((uint32)k_fallback_models[i], 0, src_dir);
            divoom_app_com_mk_dir(dst_dir);
            /*
             * divoom_port_exec() does not expand shell wildcards. Passing
             * A source path ending in a wildcard makes its cp adapter stat a
             * literal '*' path and copy nothing, so use the filesystem port
             * directly.
             */
            if (divoom_port_copy_tree(src_dir, dst_dir) == 0)
            {
                divoom_print("subscribe local pic seed model %d from %d\n",
                    model_id, k_fallback_models[i]);
                return;
            }
            divoom_print("subscribe local pic seed failed model %d from %d\n",
                model_id, k_fallback_models[i]);
        }
    }
    #endif
}

int divoom_subscribe_local_pic_load(
    int model_id,
    divoom_image_base_info *list,
    int *num,
    int max_num,
    uint32 default_user_id)
{
    char base_dir[256];
    DIR *dir;
    const divoom_port_dirent_t *ent;
    int count = 0;

    if (list == NULL || num == NULL || max_num <= 0)
    {
        if (num)
        {
            *num = 0;
        }
        return 0;
    }

    divoom_subscribe_local_pic_seed_if_empty(model_id);
    divoom_manage_image_download_get_path_name((uint32)model_id, 0, base_dir);
    divoom_app_com_mk_dir(base_dir);

    dir = divoom_port_dir_open(base_dir);
    if (dir == NULL)
    {
        *num = 0;
        return 0;
    }

    while ((ent = divoom_port_dir_read(dir)) != NULL && count < max_num)
    {
        uint32 image_id;

        if (ent->is_dir)
        {
            continue;
        }
        if (!divoom_subscribe_local_pic_is_bin_name(ent->name, &image_id))
        {
            continue;
        }

        list[count].image_id = image_id;
        list[count].user_id = default_user_id;
        list[count].voice_id[0] = '\0';
        list[count].image_name[0] = '\0';
        count++;
    }
    divoom_port_dir_close(dir);

    *num = count;
    if (count > 0)
    {
        divoom_print("subscribe local pic model %d count=%d\n", model_id, count);
    }
    return count;
}

char *divoom_subscribe_local_pic_copy_image_name(const char *image_name)
{
    uint32 name_size;
    char *name_copy;

    if(image_name == NULL)
    {
        image_name = "";
    }
    name_size = (uint32)strlen(image_name) + 1;
    name_copy = divoom_system_malloc(name_size);
    if(name_copy != NULL)
    {
        memcpy(name_copy, image_name, name_size);
    }
    return name_copy;
}

int32 divoom_subscribe_local_pic_reserve_image_list(
    Pdivoom_image_hot_item *pimage_list,
    int32 image_num,
    int32 *pimage_capacity,
    int32 required_num)
{
    int32 max_capacity;
    int32 new_capacity;
    Pdivoom_image_hot_item new_list;

    if(pimage_list == NULL || pimage_capacity == NULL || image_num < 0 ||
        required_num <= 0)
    {
        return 0;
    }
    if(required_num <= *pimage_capacity)
    {
        return 1;
    }
    max_capacity = (int32)(
        U32_MAX / (uint32)sizeof(divoom_image_hot_item));
    if(required_num > max_capacity)
    {
        return 0;
    }
    new_capacity = *pimage_capacity;
    if(new_capacity <= 0)
    {
        new_capacity = 16;
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
    if(*pimage_list != NULL && image_num > 0)
    {
        memcpy(new_list, *pimage_list,
            (uint32)image_num * sizeof(divoom_image_hot_item));
    }
    if(*pimage_list != NULL)
    {
        divoom_system_free(*pimage_list);
    }
    *pimage_list = new_list;
    *pimage_capacity = new_capacity;
    return 1;
}

int32 divoom_subscribe_local_pic_append_image(
    Pdivoom_image_hot_item *pimage_list,
    int32 *pimage_num,
    int32 *pimage_capacity,
    uint32 image_size,
    uint32 image_id,
    const char *image_name,
    uint32 user_id)
{
    char *name_copy;

    if(pimage_num == NULL || *pimage_num < 0 ||
        *pimage_num == 0x7fffffff ||
        divoom_subscribe_local_pic_reserve_image_list(
            pimage_list, *pimage_num, pimage_capacity, *pimage_num + 1) == 0)
    {
        return 0;
    }
    name_copy = divoom_subscribe_local_pic_copy_image_name(image_name);
    if(name_copy == NULL)
    {
        return 0;
    }
    (*pimage_list)[*pimage_num].image_size = image_size;
    (*pimage_list)[*pimage_num].image_id = image_id;
    (*pimage_list)[*pimage_num].image_name = name_copy;
    (*pimage_list)[*pimage_num].user_id = user_id;
    (*pimage_num)++;
    return 1;
}

int32 divoom_subscribe_local_pic_replace_image_name(
    Pdivoom_image_hot_item pimage_item, const char *image_name)
{
    char *name_copy;

    if(pimage_item == NULL)
    {
        return 0;
    }
    name_copy = divoom_subscribe_local_pic_copy_image_name(image_name);
    if(name_copy == NULL)
    {
        return 0;
    }
    if(pimage_item->image_name != NULL)
    {
        divoom_system_free(pimage_item->image_name);
    }
    pimage_item->image_name = name_copy;
    return 1;
}

void divoom_subscribe_local_pic_free_image_list(
    Pdivoom_image_hot_item image_list, int32 image_num)
{
    int32 i;

    if(image_list == NULL)
    {
        return;
    }
    for(i = 0; i < image_num; i++)
    {
        if(image_list[i].image_name != NULL)
        {
            divoom_system_free(image_list[i].image_name);
        }
    }
    divoom_system_free(image_list);
}

void divoom_subscribe_local_pic_remove_first_image(
    Pdivoom_image_hot_item image_list, int32 *pimage_num,
    int32 remove_num)
{
    int32 i;
    int32 remain_num;

    if(image_list == NULL || pimage_num == NULL || *pimage_num <= 0 ||
        remove_num <= 0)
    {
        return;
    }
    if(remove_num > *pimage_num)
    {
        remove_num = *pimage_num;
    }
    for(i = 0; i < remove_num; i++)
    {
        if(image_list[i].image_name != NULL)
        {
            divoom_system_free(image_list[i].image_name);
        }
    }
    remain_num = *pimage_num - remove_num;
    if(remain_num > 0)
    {
        memmove(image_list, &image_list[remove_num],
            (uint32)remain_num * sizeof(divoom_image_hot_item));
    }
    memset(&image_list[remain_num], 0,
        (uint32)remove_num * sizeof(divoom_image_hot_item));
    *pimage_num = remain_num;
}

Pdivoom_image_hot_item divoom_subscribe_local_pic_load_dynamic(
    int model_id, int32 *pimage_num, int32 max_num,
    uint32 default_user_id)
{
    int32 i;
    int32 loaded_num = 0;
    int32 local_num;
    divoom_image_base_info *local_list;
    Pdivoom_image_hot_item image_list;

    if(pimage_num == NULL || max_num <= 0)
    {
        return NULL;
    }
    *pimage_num = 0;
    divoom_subscribe_local_pic_seed_if_empty(model_id);
    local_num = divoom_subscribe_local_pic_count_bins(model_id);
    if(local_num > max_num)
    {
        local_num = max_num;
    }
    if(local_num <= 0 || (uint32)local_num >
        U32_MAX / (uint32)sizeof(divoom_image_base_info))
    {
        return NULL;
    }
    local_list = divoom_system_malloc(
        (uint32)local_num * sizeof(divoom_image_base_info));
    if(local_list == NULL)
    {
        return NULL;
    }
    divoom_subscribe_local_pic_load(
        model_id, local_list, &loaded_num, local_num, default_user_id);
    if(loaded_num <= 0 || (uint32)loaded_num >
        U32_MAX / (uint32)sizeof(divoom_image_hot_item))
    {
        divoom_system_free(local_list);
        return NULL;
    }
    image_list = divoom_system_malloc(
        (uint32)loaded_num * sizeof(divoom_image_hot_item));
    if(image_list == NULL)
    {
        divoom_system_free(local_list);
        return NULL;
    }
    for(i = 0; i < loaded_num; i++)
    {
        image_list[i].image_size = 0;
        image_list[i].image_id = local_list[i].image_id;
        image_list[i].user_id = local_list[i].user_id;
        image_list[i].image_name =
            divoom_subscribe_local_pic_copy_image_name(
                local_list[i].image_name);
        if(image_list[i].image_name == NULL)
        {
            divoom_subscribe_local_pic_free_image_list(image_list, i);
            divoom_system_free(local_list);
            return NULL;
        }
    }
    divoom_system_free(local_list);
    *pimage_num = loaded_num;
    return image_list;
}

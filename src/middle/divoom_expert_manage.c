#include "divoom.h"
#include "divoom_subscribe_local_pic.h"


#define DIVOOM_EXPERT_MANAGE_MAX_FILE_NUM 300
#define DIVOOM_EXPERT_MANAGE_LOAD_FILE_TIME 600
#define DIVOOM_EXPERT_MANAGE_MAX_REQUEST_NUM 100
#define DIVOOM_EXPERT_MANAGE_ALL_LOAD_FILE_TIME 3600


typedef struct _divoom_expert_manage_user
{
    int user_id;
    int user_times;         //使用次数
    int read_pos;
    int upload_time;        //最后更新时间
    int last_request_time;
    int image_num;
    int image_capacity;
    void *prandom_handle;
    Pdivoom_image_hot_item image_list;  //所有作品按实际数量动态扩容
}divoom_expert_manage_user, *Pdivoom_expert_manage_user;


typedef struct _divoom_expert_manage_ctrl
{
    void *expert_queue;     //达人相关文件的队列
    void *lock_handle;
}divoom_expert_manage_ctrl, *Pdivoom_expert_manage_ctrl;


Pdivoom_expert_manage_ctrl pdivoom_expert_manage_handle = NULL;

static int32 divoom_expert_manage_add_unique_user_id(
    uint32 *user_list, int32 *puser_num, uint32 user_id)
{
    if(user_list == NULL || puser_num == NULL || user_id == 0)
    {
        return 0;
    }
    user_list[*puser_num] = user_id;
    (*puser_num)++;
    return 1;
}

static int divoom_expert_manage_compare_image_id(
    const void *item1, const void *item2)
{
    const divoom_image_hot_item *image1 =
        (const divoom_image_hot_item *)item1;
    const divoom_image_hot_item *image2 =
        (const divoom_image_hot_item *)item2;

    return image1->image_id < image2->image_id ? -1 :
        image1->image_id > image2->image_id;
}

static int32 divoom_expert_manage_append_image_sorted(
    Pdivoom_expert_manage_user pcur_item, uint32 image_id, uint32 user_id)
{
    int32 low;
    int32 high;
    int32 mid;
    int32 insert_pos;
    int32 old_num;
    divoom_image_hot_item new_item;

    if(pcur_item == NULL)
    {
        return 0;
    }
    old_num = pcur_item->image_num;
    if(divoom_subscribe_local_pic_append_image(
        &pcur_item->image_list, &pcur_item->image_num,
        &pcur_item->image_capacity, 0, image_id, "", user_id) == 0)
    {
        return 0;
    }
    low = 0;
    high = old_num;
    while(low < high)
    {
        mid = low + (high - low) / 2;
        if(pcur_item->image_list[mid].image_id < image_id)
        {
            low = mid + 1;
        }
        else
        {
            high = mid;
        }
    }
    insert_pos = low;
    if(insert_pos < old_num)
    {
        new_item = pcur_item->image_list[old_num];
        memmove(&pcur_item->image_list[insert_pos + 1],
            &pcur_item->image_list[insert_pos],
            (uint32)(old_num - insert_pos) * sizeof(divoom_image_hot_item));
        pcur_item->image_list[insert_pos] = new_item;
    }
    return 1;
}

int divoom_expert_manage_find_expert_compare_id(void *item1, void *item2)
{
    int *user_id = (int *)item1;
    Pdivoom_expert_manage_user pcache_item = (Pdivoom_expert_manage_user)item2;

    return *user_id != pcache_item->user_id;
    
}


/*********************************************************************
  Function:        divoom_expert_manage_delete_image_list
  Description:     删除一些作品
  Input:           mode
                    
  Return:          none
**********************************************************************/

void divoom_expert_manage_delete_image_list(int expert_id, uint32  *image_list, int image_num)
{
    for(int i = 0; i < image_num; i++)
    {
        divoom_database_manage_delete_subscribe_image_item(DIVOOM_EXPERT_IMAGE_TABLE_NAME, expert_id, image_list[i]);
    }
    divoom_manage_image_del(expert_id, image_list, image_num, 1);
}

/* 达人作品达到业务上限时淘汰最旧项，动态数组只负责按需分配内存。 */
static void divoom_expert_manage_remove_first_image(
    Pdivoom_expert_manage_user pcur_item, int32 remove_num)
{
    int32 i;
    uint32 image_id;

    if(pcur_item == NULL || remove_num <= 0)
    {
        return;
    }
    if(remove_num > pcur_item->image_num)
    {
        remove_num = pcur_item->image_num;
    }
    for(i = 0; i < remove_num; i++)
    {
        image_id = pcur_item->image_list[i].image_id;
        divoom_database_manage_delete_subscribe_image_item(
            DIVOOM_EXPERT_IMAGE_TABLE_NAME, pcur_item->user_id,
            (int)image_id);
        divoom_manage_image_del(pcur_item->user_id, &image_id, 1, 1);
    }
    divoom_subscribe_local_pic_remove_first_image(
        pcur_item->image_list, &pcur_item->image_num, remove_num);
}


/*********************************************************************
  Function:        divoom_expert_manage_check_expert_image_id
  Description:     检查这个文件是否在这个达人下面
  Input:           mode
                    
  Return:          none
**********************************************************************/

int divoom_expert_manage_check_expert_image_id(Pdivoom_expert_manage_user pcur_item, int  image_id)
{
    int32 low = 0;
    int32 high;
    int32 mid;
    uint32 target_id = (uint32)image_id;

    if(pcur_item == NULL || pcur_item->image_num == 0)
    {
        return 0;
    }
    high = pcur_item->image_num - 1;
    while(low <= high)
    {
        mid = low + (high - low) / 2;
        if(pcur_item->image_list[mid].image_id == target_id)
        {
            return 1;
        }
        if(pcur_item->image_list[mid].image_id < target_id)
        {
            low = mid + 1;
        }
        else
        {
            high = mid - 1;
        }
    }
    return 0;
}

/*********************************************************************
  Function:        divoom_expert_manage_update_list_image
  Description:     更新达人最新的图
  Input:           mode
                    
  Return:          none
**********************************************************************/

void *divoom_expert_manage_open_expert_handle(int expert_id)
{
    int32 remove_num;
    Pdivoom_expert_manage_user pcur_item;
    
    divoom_system_get_lock(pdivoom_expert_manage_handle->lock_handle);

    pcur_item = divoom_queue_get_cmp(pdivoom_expert_manage_handle->expert_queue, &expert_id, divoom_expert_manage_find_expert_compare_id);
    if(pcur_item)
    {
        pcur_item->user_times ++;
        divoom_system_free_lock(pdivoom_expert_manage_handle->lock_handle);
        return pcur_item;
    }
    pcur_item = divoom_system_malloc(sizeof(divoom_expert_manage_user));
    if (pcur_item == NULL)
    {
        divoom_system_free_lock(pdivoom_expert_manage_handle->lock_handle);
        return NULL;
    }
    pcur_item->user_id = expert_id;
    pcur_item->image_list =
        divoom_database_manage_get_subscribe_image_pic_list(
            DIVOOM_EXPERT_IMAGE_TABLE_NAME, expert_id,
            &pcur_item->image_num);
    pcur_item->image_capacity = pcur_item->image_num;
    if(pcur_item->image_num > DIVOOM_EXPERT_MANAGE_MAX_FILE_NUM)
    {
        remove_num = pcur_item->image_num -
            DIVOOM_EXPERT_MANAGE_MAX_FILE_NUM;
        divoom_expert_manage_remove_first_image(pcur_item, remove_num);
    }
    if(pcur_item->image_num == 0)
    {
        pcur_item->image_list = divoom_subscribe_local_pic_load_dynamic(
            expert_id,
            &pcur_item->image_num,
            DIVOOM_EXPERT_MANAGE_MAX_FILE_NUM,
            (uint32)expert_id);
        pcur_item->image_capacity = pcur_item->image_num;
    }
    if(pcur_item->image_num > 1)
    {
        qsort(pcur_item->image_list, (size_t)pcur_item->image_num,
            sizeof(divoom_image_hot_item),
            divoom_expert_manage_compare_image_id);
    }
    pcur_item->user_times = 1;
    pcur_item->last_request_time = 0;
    pcur_item->prandom_handle = divoom_random_create(pcur_item->image_num);
    pcur_item->upload_time = divoom_database_manage_get_subscribe_image_last_update_time(DIVOOM_EXPERT_IMAGE_TABLE_NAME, expert_id);
    divoom_queue_push(pdivoom_expert_manage_handle->expert_queue, pcur_item);
    divoom_system_free_lock(pdivoom_expert_manage_handle->lock_handle);
    return pcur_item;
}

/*********************************************************************
  Function:        divoom_expert_manage_delete_expert_info
  Description:     删除某个表盘的某个达人信息
  Input:           mode
                    
  Return:          none
**********************************************************************/

void divoom_expert_manage_delete_expert_info(int clock_id, int clock_pos, int parent_id, int expert_id)
{
    uint32 *pic_list;
    Pdivoom_expert_manage_user pcur_item;
    
    divoom_print("remove expert id: %d, %d,%d, %d\n", clock_id, clock_pos, parent_id, expert_id);

    pcur_item = divoom_expert_manage_open_expert_handle(expert_id);
    divoom_database_manage_delete_divoom_image_config_item(DIVOOM_EXPERT_IMAGE_CONFIG_TABLE_NAME, clock_id, clock_pos, parent_id, expert_id);
    if(divoom_database_manage_check_divoom_image_type_exist(DIVOOM_EXPERT_IMAGE_CONFIG_TABLE_NAME, expert_id) == 0)
    {
        //如果没有人使用，将清除数据
        divoom_print("delete expert id: %d\n", expert_id);
        pic_list = divoom_system_malloc(pcur_item->image_num * sizeof(uint32));
        for(int i = 0; i < pcur_item->image_num; i++)
        {
            pic_list[i] = pcur_item->image_list[i].image_id;
        }
        divoom_manage_image_del(expert_id, pic_list, pcur_item->image_num, 1);
        divoom_system_free(pic_list);
        divoom_database_manage_delete_subscribe_image_all(DIVOOM_EXPERT_IMAGE_TABLE_NAME, expert_id);
    }
    divoom_expert_manage_close_expert_handle(expert_id);
}

/*********************************************************************
  Function:        divoom_expert_manage_add_expert_info
  Description:     添加某个表盘的某个达人的信息
  Input:           mode
                    
  Return:          none
**********************************************************************/

void divoom_expert_manage_add_expert_info(int clock_id, int clock_pos, int parent_id, int expert_id)
{
    int exist_flag;
    
    divoom_print("add expert id: %d, %d,%d, %d\n", clock_id, clock_pos, parent_id, expert_id);
    exist_flag = divoom_database_manage_check_divoom_image_type_exist(DIVOOM_EXPERT_IMAGE_CONFIG_TABLE_NAME, expert_id);
    divoom_database_manage_add_divoom_image_config_item(DIVOOM_EXPERT_IMAGE_CONFIG_TABLE_NAME, clock_id, clock_pos, parent_id, expert_id);
    if(exist_flag == 0)
    {
        //如果不存在，将先请求一次数据
        divoom_devcice_expert_request_json pexpert_info = {0};

        divoom_print("add new expert id: %d\n", expert_id);
        pexpert_info.expert_num = 1;
        pexpert_info.file_num = DIVOOM_EXPERT_MANAGE_MAX_FILE_NUM;
        pexpert_info.upload_time[0] = 0;
        pexpert_info.expert_list[0] = expert_id;
        divoom_net_app_send_packet(DIVOOM_NET_COMM_GET_EXPERT_LIST, DIVOOM_SERVER_TOPIC, &pexpert_info);     
    }
}


/*********************************************************************
  Function:        divoom_expert_manage_update_list_image
  Description:     更新达人最新的图
  Input:           mode
                    
  Return:          none
**********************************************************************/

void divoom_expert_manage_close_expert_handle(int expert_id)
{
    Pdivoom_expert_manage_user pcur_item;
    
    divoom_system_get_lock(pdivoom_expert_manage_handle->lock_handle);

    pcur_item = divoom_queue_get_cmp(pdivoom_expert_manage_handle->expert_queue, &expert_id, divoom_expert_manage_find_expert_compare_id);
    if(pcur_item == NULL)
    {
        divoom_system_free_lock(pdivoom_expert_manage_handle->lock_handle);
        return;
    }
    pcur_item->user_times --;
    if(pcur_item->user_times)
    {
        divoom_system_free_lock(pdivoom_expert_manage_handle->lock_handle);
        return;
    }
    pcur_item = divoom_queue_pop_cmp(pdivoom_expert_manage_handle->expert_queue, &expert_id, divoom_expert_manage_find_expert_compare_id);
    divoom_subscribe_local_pic_free_image_list(
        pcur_item->image_list, pcur_item->image_num);
    divoom_random_destroy(pcur_item->prandom_handle);
    divoom_system_free(pcur_item);
    divoom_system_free_lock(pdivoom_expert_manage_handle->lock_handle);
}

/*********************************************************************
  Function:        divoom_expert_manage_update_last_image
  Description:     更新达人最新的图
  Input:           mode
                    
  Return:          none
**********************************************************************/

void divoom_expert_manage_update_last_image(Pdivoom_devcice_image_list pimage_list)
{
    int32 i;
    int32 group_start;
    int32 group_end;
    int32 download_num = 0;
    int32 user_num = 0;
    int show_transfer;
    divoom_manage_image_item_info pimage_item = {0};
    Pdivoom_manage_image_item_info pdownload_list = NULL;
    Pdivoom_manage_image_item_info pdownload_item;
    int32 *pdownload_model_list = NULL;
    Pdivoom_expert_manage_user expert_queue_info = NULL;
    
    if(pimage_list == NULL || pimage_list->num == 0)
    {
        return;
    }
    if((uint32)pimage_list->num <=
        U32_MAX / (uint32)sizeof(divoom_manage_image_item_info) &&
        (uint32)pimage_list->num <= U32_MAX / (uint32)sizeof(int32))
    {
        pdownload_list = divoom_system_malloc(
            (uint32)pimage_list->num *
                sizeof(divoom_manage_image_item_info));
        pdownload_model_list = divoom_system_malloc(
            (uint32)pimage_list->num * sizeof(int32));
        if(pdownload_list == NULL || pdownload_model_list == NULL)
        {
            if(pdownload_list != NULL)
            {
                divoom_system_free(pdownload_list);
            }
            if(pdownload_model_list != NULL)
            {
                divoom_system_free(pdownload_model_list);
            }
            pdownload_list = NULL;
            pdownload_model_list = NULL;
        }
        else
        {
            memset(pdownload_list, 0,
                (uint32)pimage_list->num *
                    sizeof(divoom_manage_image_item_info));
        }
    }
    show_transfer = divoom_subscribe_manage_transfer_response_is_active();
    if(show_transfer)
    {
        divoom_manage_image_transfer_batch_begin();
    }
    divoom_database_manage_txn_begin();
    for(i = 0; i < pimage_list->num; i ++)
    {
        expert_queue_info = divoom_expert_manage_open_expert_handle(pimage_list->pfile_list[i].user_id);
        if(expert_queue_info != NULL &&
            divoom_expert_manage_check_expert_image_id(
                expert_queue_info,
                pimage_list->pfile_list[i].image_id) == 0)
        {
            if(expert_queue_info->upload_time < pimage_list->pfile_list[i].utc_time)
            {
                expert_queue_info->upload_time = pimage_list->pfile_list[i].utc_time;
            }
            if(expert_queue_info->image_num >=
                DIVOOM_EXPERT_MANAGE_MAX_FILE_NUM)
            {
                divoom_expert_manage_remove_first_image(
                    expert_queue_info, 1);
            }
            if(divoom_expert_manage_append_image_sorted(
                expert_queue_info,
                (uint32)pimage_list->pfile_list[i].image_id,
                (uint32)pimage_list->pfile_list[i].user_id) == 0)
            {
                divoom_expert_manage_close_expert_handle(
                    pimage_list->pfile_list[i].user_id);
                continue;
            }
            
            if(pdownload_list != NULL)
            {
                pdownload_item = &pdownload_list[download_num];
                pdownload_model_list[download_num] =
                    expert_queue_info->user_id;
                download_num++;
            }
            else
            {
                pdownload_item = &pimage_item;
            }
            pdownload_item->image_id = pimage_list->pfile_list[i].image_id;
            strcpy(pdownload_item->file_addr,
                pimage_list->pfile_list[i].file_id);
			pdownload_item->utc_time = pimage_list->pfile_list[i].utc_time;
            pdownload_item->pcallback = NULL;
            if(pdownload_list == NULL)
            {
                if(show_transfer)
                {
                    divoom_manage_image_add_transfer(
                        expert_queue_info->user_id, pdownload_item, 1);
                }
                else
                {
                    divoom_manage_image_add(
                        expert_queue_info->user_id, pdownload_item, 1);
                }
            }
            divoom_database_manage_add_subscribe_image_item(
                DIVOOM_EXPERT_IMAGE_TABLE_NAME,
                expert_queue_info->user_id,
                pdownload_item->image_id,
                pdownload_item->utc_time,
                pdownload_item->file_addr,
                expert_queue_info->user_id,
                "");
        }
        if(expert_queue_info != NULL)
        {
            divoom_expert_manage_close_expert_handle(
                pimage_list->pfile_list[i].user_id);
        }
    }
    divoom_database_manage_txn_end();
    if(pdownload_list != NULL)
    {
        group_start = 0;
        while(group_start < download_num)
        {
            group_end = group_start + 1;
            while(group_end < download_num &&
                pdownload_model_list[group_end] ==
                    pdownload_model_list[group_start])
            {
                group_end++;
            }
            if(show_transfer)
            {
                divoom_manage_image_add_transfer(
                    pdownload_model_list[group_start],
                    &pdownload_list[group_start],
                    (uint32)(group_end - group_start));
            }
            else
            {
                divoom_manage_image_add(
                    pdownload_model_list[group_start],
                    &pdownload_list[group_start],
                    (uint32)(group_end - group_start));
            }
            group_start = group_end;
        }
        divoom_system_free(pdownload_list);
        divoom_system_free(pdownload_model_list);
    }
    if(show_transfer)
    {
        divoom_manage_image_transfer_batch_end();
    }

}


/*********************************************************************
  Function:        divoom_expert_manage_update_list_image
  Description:     更新达人最新的图
  Input:           mode
                    
  Return:          none
**********************************************************************/

void divoom_expert_manage_update_list_image(Pdivoom_devcice_expert_list pimage_list)
{
    int32 i;
    int32 j;
    int32 max_file_num = 0;
    int32 download_num;
    int32 user_num = 0;
    int show_transfer;
    divoom_manage_image_item_info pimage_item = {0};
    Pdivoom_manage_image_item_info pdownload_list = NULL;
    Pdivoom_manage_image_item_info pdownload_item;
    Pdivoom_expert_manage_user expert_queue_info = NULL;
    
    if(pimage_list == NULL || pimage_list->num == 0)
    {
        return;
    }
    for(i = 0; i < pimage_list->num; i++)
    {
        if(pimage_list->puser_list[i].num > max_file_num)
        {
            max_file_num = pimage_list->puser_list[i].num;
        }
    }
    if(max_file_num > 0 && (uint32)max_file_num <=
        U32_MAX / (uint32)sizeof(divoom_manage_image_item_info))
    {
        pdownload_list = divoom_system_malloc(
            (uint32)max_file_num * sizeof(divoom_manage_image_item_info));
        if(pdownload_list != NULL)
        {
            memset(pdownload_list, 0,
                (uint32)max_file_num *
                    sizeof(divoom_manage_image_item_info));
        }
    }
    show_transfer = divoom_subscribe_manage_transfer_response_is_active();
    if(show_transfer)
    {
        divoom_manage_image_transfer_batch_begin();
    }
    divoom_database_manage_txn_begin();
    for(i = 0; i < pimage_list->num; i ++)
    {
        download_num = 0;
        expert_queue_info = divoom_expert_manage_open_expert_handle(pimage_list->puser_list[i].user_id);
        if(expert_queue_info == NULL)
        {
            divoom_print("this is err: %d\n",pimage_list->puser_list[i].user_id);
            continue;
        }
        for(j = 0; j < pimage_list->puser_list[i].num; j++)
        {
            if(divoom_expert_manage_check_expert_image_id(expert_queue_info, pimage_list->puser_list[i].pfile_list[j].image_id) == 0)
            {
                if(expert_queue_info->upload_time < pimage_list->puser_list[i].pfile_list[j].utc_time)
                {
                    expert_queue_info->upload_time = pimage_list->puser_list[i].pfile_list[j].utc_time;
                }
                if(expert_queue_info->image_num >=
                    DIVOOM_EXPERT_MANAGE_MAX_FILE_NUM)
                {
                    divoom_expert_manage_remove_first_image(
                        expert_queue_info, 1);
                }
                if(divoom_expert_manage_append_image_sorted(
                    expert_queue_info,
                    (uint32)pimage_list->puser_list[i].pfile_list[j].image_id,
                    (uint32)pimage_list->puser_list[i].user_id) == 0)
                {
                    continue;
                }
                if(pdownload_list != NULL)
                {
                    pdownload_item = &pdownload_list[download_num++];
                }
                else
                {
                    pdownload_item = &pimage_item;
                }
                pdownload_item->image_id =
                    pimage_list->puser_list[i].pfile_list[j].image_id;
                strcpy(pdownload_item->file_addr,
                    pimage_list->puser_list[i].pfile_list[j].file_id);
				pdownload_item->utc_time =
                    pimage_list->puser_list[i].pfile_list[j].utc_time;
                pdownload_item->pcallback = NULL;
                if(pdownload_list == NULL)
                {
                    if(show_transfer)
                    {
                        divoom_manage_image_add_transfer(
                            expert_queue_info->user_id, pdownload_item, 1);
                    }
                    else
                    {
                        divoom_manage_image_add(
                            expert_queue_info->user_id, pdownload_item, 1);
                    }
                }
                divoom_database_manage_add_subscribe_image_item(
                    DIVOOM_EXPERT_IMAGE_TABLE_NAME,
                    expert_queue_info->user_id,
                    pdownload_item->image_id,
                    pdownload_item->utc_time,
                    pdownload_item->file_addr,
                    expert_queue_info->user_id,
                    "");
            }
        }
        if(pdownload_list != NULL && download_num > 0)
        {
            if(show_transfer)
            {
                divoom_manage_image_add_transfer(
                    expert_queue_info->user_id, pdownload_list,
                    (uint32)download_num);
            }
            else
            {
                divoom_manage_image_add(
                    expert_queue_info->user_id, pdownload_list,
                    (uint32)download_num);
            }
        }
        divoom_expert_manage_close_expert_handle(pimage_list->puser_list[i].user_id);
    }
    divoom_database_manage_txn_end();
    if(pdownload_list != NULL)
    {
        divoom_system_free(pdownload_list);
    }
    if(show_transfer)
    {
        divoom_manage_image_transfer_batch_end();
    }

}

void divoom_expert_manage_request_image_list(uint8 wait_flag);


/*********************************************************************
  Function:        divoom_expert_manage_get_disp_image_id
  Description:     得到要显示的那张图的ＩＤ
  Input:           image_info
                    
  Return:          none
**********************************************************************/

void divoom_expert_manage_get_disp_image_id(int expert_id, Pdivoom_play_gif_image_info image_info, int last_flag, char *image_name)
{
    Pdivoom_expert_manage_user pcur_item;
    
    divoom_system_get_lock(pdivoom_expert_manage_handle->lock_handle);
    pcur_item = divoom_queue_get_cmp(pdivoom_expert_manage_handle->expert_queue, &expert_id, divoom_expert_manage_find_expert_compare_id);
    #if 1
    if(pcur_item && pcur_item->last_request_time + DIVOOM_EXPERT_MANAGE_ALL_LOAD_FILE_TIME < time(0))
    {
        divoom_devcice_expert_request_json pexpert_info = {0};
        
        pexpert_info.expert_num = 1;
        pexpert_info.file_num = DIVOOM_EXPERT_MANAGE_MAX_FILE_NUM;
        pexpert_info.upload_time[0] = pcur_item->upload_time;
        pexpert_info.expert_list[0] = expert_id;
        pcur_item->last_request_time = time(0);
        divoom_net_app_send_packet(DIVOOM_NET_COMM_GET_EXPERT_LIST, DIVOOM_SERVER_TOPIC, &pexpert_info);        
    }
    #endif
    if(pcur_item == NULL || pcur_item->image_num == 0)
    {
        image_info->model_id =  DIVOOM_MANAGE_IMAGE_LOCAL_PIC_MODE;
        image_info->image_id =  DIVOOM_MANAGE_IMAGE_ID_BASE_LOCAL_START_GIF + DIVOOM_DISP_USER_DEFINE_TV_PIC;
        divoom_system_free_lock(pdivoom_expert_manage_handle->lock_handle);
        return;
    }

    image_info->model_id =  expert_id;
    if(!last_flag)
    {
        image_info->image_id =  pcur_item->image_list[pcur_item->image_num-1].image_id;
        strcpy(image_name, pcur_item->image_list[pcur_item->image_num-1].image_name);
    }
    else
    {
        int index = divoom_random_get(pcur_item->prandom_handle)%pcur_item->image_num;
        image_info->image_id =  pcur_item->image_list[index].image_id;
        strcpy(image_name, pcur_item->image_list[index].image_name);
    }
    divoom_system_free_lock(pdivoom_expert_manage_handle->lock_handle);
}

void divoom_expert_manage_reset(void)
{
    int *plist, list_num;
    
    divoom_database_manage_clear_all_divoom_image(DIVOOM_EXPERT_IMAGE_TABLE_NAME);
    plist = divoom_database_manage_get_divoom_image_image_type_list(DIVOOM_EXPERT_IMAGE_CONFIG_TABLE_NAME, &list_num);
    if(plist)
    {
        for(int i = 0; i < list_num; i++)
        {
            divoom_manage_image_remove(plist[i], 1);
        }
        divoom_database_manage_clear_all_divoom_image(DIVOOM_EXPERT_IMAGE_CONFIG_TABLE_NAME);
        divoom_system_free(plist);
    }
}

void divoom_expert_manage_init(void)
{
    if(pdivoom_expert_manage_handle)
    {
        return; 
    }

    pdivoom_expert_manage_handle = (Pdivoom_expert_manage_ctrl)divoom_system_malloc(sizeof(divoom_expert_manage_ctrl));
    if(pdivoom_expert_manage_handle == NULL)
    {
        return;
    }
	char *table_item = "image_id INT PRIMARY KEY     NOT NULL," \
		"image_type INTEGER DEFAULT 0," \
		"author_id INTEGER DEFAULT 0," \
		"image_name CHAR(200)," \
		"file_addr CHAR(64)," \
		"update_time INTEGER DEFAULT 0";

	divoom_database_manage_add_new_table(DIVOOM_EXPERT_IMAGE_TABLE_NAME, table_item);
    divoom_database_manage_add_table_index(DIVOOM_EXPERT_IMAGE_TABLE_NAME, "update_time");
    
	char *table_config_item = "id INTEGER PRIMARY KEY     AUTOINCREMENT," \
		"clock_id INTEGER DEFAULT 0," \
		"clock_pos INTEGER DEFAULT 0," \
		"parent_id INTEGER DEFAULT 0," \
		"image_type INTEGER DEFAULT 0," \
		"update_time INTEGER DEFAULT 0";

	divoom_database_manage_add_new_table(DIVOOM_EXPERT_IMAGE_CONFIG_TABLE_NAME, table_config_item);
    divoom_database_manage_add_table_index(DIVOOM_EXPERT_IMAGE_CONFIG_TABLE_NAME, "clock_id");
    divoom_database_manage_add_table_index(DIVOOM_EXPERT_IMAGE_CONFIG_TABLE_NAME, "clock_pos");
    divoom_database_manage_add_table_index(DIVOOM_EXPERT_IMAGE_CONFIG_TABLE_NAME, "parent_id");
    divoom_database_manage_add_table_index(DIVOOM_EXPERT_IMAGE_CONFIG_TABLE_NAME, "image_type");
    divoom_database_manage_add_table_index(DIVOOM_EXPERT_IMAGE_CONFIG_TABLE_NAME, "update_time");
    
    pdivoom_expert_manage_handle->expert_queue = divoom_queue_init(1000, __func__, __LINE__);
    pdivoom_expert_manage_handle->lock_handle = divoom_system_create_lock();
}


/*********************************************************************
  Function:        divoom_expert_manage_get_url_addr
  Description:     获取指定文件的网络地址
  Input:           model_id
                   image_id
                   url_addr
                    
  Return:          image_flag
**********************************************************************/

int divoom_expert_manage_get_url_addr(int model_id, uint32 image_id, char *url_addr)
{
    return divoom_database_manage_get_subscribe_image_pic_addr(DIVOOM_EXPERT_IMAGE_TABLE_NAME, image_id, url_addr);
}


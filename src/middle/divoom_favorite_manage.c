#include "divoom.h"
#include "divoom_subscribe_local_pic.h"


#define DIVOOM_FAVORITE_MANAGE_MAX_FILE_NUM 300
#define DIVOOM_FAVORITE_MANAGE_LOAD_FILE_TIME 3600
#define DIVOOM_FAVORITE_MANAGE_ALL_LOAD_FILE_TIME 3600


typedef struct _divoom_favorite_manage_ctrl
{
    int use_time;
    int update_time;
    int next_request_time;
    int image_num;
    int image_capacity;
    void *prandom_handle;
    Pdivoom_image_hot_item image_list;
}divoom_favorite_manage_ctrl, *Pdivoom_favorite_manage_ctrl;

void *pdivoom_favorite_manage_lock = NULL;
Pdivoom_favorite_manage_ctrl pdivoom_favorite_manage_handle = NULL;
static int g_divoom_favorite_manage_next_request_time = 0;

static int divoom_favorite_manage_compare_user_id(
    const void *item1, const void *item2)
{
    uint32 user_id1 = *(const uint32 *)item1;
    uint32 user_id2 = *(const uint32 *)item2;

    return user_id1 < user_id2 ? -1 : user_id1 > user_id2;
}

static int divoom_favorite_manage_compare_image_id(
    const void *item1, const void *item2)
{
    const divoom_image_hot_item *image1 =
        (const divoom_image_hot_item *)item1;
    const divoom_image_hot_item *image2 =
        (const divoom_image_hot_item *)item2;

    return image1->image_id < image2->image_id ? -1 :
        image1->image_id > image2->image_id;
}

static int32 divoom_favorite_manage_find_image_index(uint32 image_id)
{
    int32 low = 0;
    int32 high;
    int32 mid;

    if(pdivoom_favorite_manage_handle == NULL ||
        pdivoom_favorite_manage_handle->image_num <= 0)
    {
        return -1;
    }
    high = pdivoom_favorite_manage_handle->image_num - 1;
    while(low <= high)
    {
        mid = low + (high - low) / 2;
        if(pdivoom_favorite_manage_handle->image_list[mid].image_id == image_id)
        {
            return mid;
        }
        if(pdivoom_favorite_manage_handle->image_list[mid].image_id < image_id)
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

static int32 divoom_favorite_manage_append_image_sorted(
    uint32 image_id, uint32 user_id)
{
    int32 low;
    int32 high;
    int32 mid;
    int32 insert_pos;
    int32 old_num;
    divoom_image_hot_item new_item;

    if(pdivoom_favorite_manage_handle == NULL)
    {
        return 0;
    }
    old_num = pdivoom_favorite_manage_handle->image_num;
    if(divoom_subscribe_local_pic_append_image(
        &pdivoom_favorite_manage_handle->image_list,
        &pdivoom_favorite_manage_handle->image_num,
        &pdivoom_favorite_manage_handle->image_capacity,
        0, image_id, "", user_id) == 0)
    {
        return 0;
    }
    low = 0;
    high = old_num;
    while(low < high)
    {
        mid = low + (high - low) / 2;
        if(pdivoom_favorite_manage_handle->image_list[mid].image_id < image_id)
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
        new_item = pdivoom_favorite_manage_handle->image_list[old_num];
        memmove(&pdivoom_favorite_manage_handle->image_list[insert_pos + 1],
            &pdivoom_favorite_manage_handle->image_list[insert_pos],
            (uint32)(old_num - insert_pos) * sizeof(divoom_image_hot_item));
        pdivoom_favorite_manage_handle->image_list[insert_pos] = new_item;
    }
    return 1;
}


void divoom_favorite_manage_clear_all(void)
{
    divoom_system_get_lock(pdivoom_favorite_manage_lock);
    if(pdivoom_favorite_manage_handle != NULL)
    {
        divoom_subscribe_local_pic_free_image_list(
            pdivoom_favorite_manage_handle->image_list,
            pdivoom_favorite_manage_handle->image_num);
        pdivoom_favorite_manage_handle->image_list = NULL;
        pdivoom_favorite_manage_handle->image_num = 0;
        pdivoom_favorite_manage_handle->image_capacity = 0;
    }
    divoom_system_free_lock(pdivoom_favorite_manage_lock);
    divoom_manage_image_remove(DIVOOM_MANAGE_IMAGE_SUBSCRIBE_FAVORITE, 1);
    divoom_database_manage_clear_all_divoom_image(DIVOOM_FAVORITE_IMAGE_TABLE_NAME);
}


void divoom_favorite_manage_init(uint8 *mac_addr)
{

	char *table_item = "id INTEGER PRIMARY KEY  AUTOINCREMENT," \
		"image_type INTEGER DEFAULT 0," \
		"author_id INTEGER DEFAULT 0," \
		"image_name CHAR(200)," \
		"image_id INTEGER DEFAULT 0," \
		"update_time INTEGER DEFAULT 0," \
		"file_addr CHAR(64)" ;
    

	divoom_database_manage_add_new_table(DIVOOM_FAVORITE_IMAGE_TABLE_NAME, table_item);
    divoom_database_manage_add_table_index(DIVOOM_FAVORITE_IMAGE_TABLE_NAME, "image_type");
    divoom_database_manage_add_table_index(DIVOOM_FAVORITE_IMAGE_TABLE_NAME, "image_id");
    divoom_database_manage_add_table_index(DIVOOM_FAVORITE_IMAGE_TABLE_NAME, "update_time");
    pdivoom_favorite_manage_lock = divoom_system_create_lock();
}

static int divoom_favorite_manage_request_list_locked(int update_time)
{
    int now = (int)time(0);

    if(now < g_divoom_favorite_manage_next_request_time)
    {
        return 0;
    }
    g_divoom_favorite_manage_next_request_time = now +
        DIVOOM_FAVORITE_MANAGE_ALL_LOAD_FILE_TIME;
    if(pdivoom_favorite_manage_handle != NULL)
    {
        pdivoom_favorite_manage_handle->next_request_time =
            g_divoom_favorite_manage_next_request_time;
    }
    divoom_net_app_send_packet(DIVOOM_NET_COMM_GET_FAVORITE_LIST,
        DIVOOM_SERVER_TOPIC, &update_time);
    return 1;
}

int divoom_favorite_manage_request_list(int update_time)
{
    int request_flag;

    divoom_system_get_lock(pdivoom_favorite_manage_lock);
    request_flag = divoom_favorite_manage_request_list_locked(update_time);
    divoom_system_free_lock(pdivoom_favorite_manage_lock);
    return request_flag;
}

static int divoom_favorite_manage_open_internal(int request_list)
{
    int32 i;
    int32 remove_num;

    divoom_system_get_lock(pdivoom_favorite_manage_lock);
    if(pdivoom_favorite_manage_handle)
    {
        pdivoom_favorite_manage_handle->use_time ++;
        divoom_system_free_lock(pdivoom_favorite_manage_lock);
        return 1;
    }
    
    pdivoom_favorite_manage_handle = divoom_system_malloc(sizeof(divoom_favorite_manage_ctrl));
    if(pdivoom_favorite_manage_handle == NULL)
    {
        divoom_system_free_lock(pdivoom_favorite_manage_lock);
        return 0;
    }
    pdivoom_favorite_manage_handle->image_list =
        divoom_database_manage_get_subscribe_image_pic_list(
            DIVOOM_FAVORITE_IMAGE_TABLE_NAME, 0,
            &pdivoom_favorite_manage_handle->image_num);
    pdivoom_favorite_manage_handle->image_capacity =
        pdivoom_favorite_manage_handle->image_num;
    if(pdivoom_favorite_manage_handle->image_num >
        DIVOOM_FAVORITE_MANAGE_MAX_FILE_NUM)
    {
        remove_num = pdivoom_favorite_manage_handle->image_num -
            DIVOOM_FAVORITE_MANAGE_MAX_FILE_NUM;
        for(i = 0; i < remove_num; i++)
        {
            divoom_database_manage_delete_subscribe_image_item(
                DIVOOM_FAVORITE_IMAGE_TABLE_NAME, 0,
                (int)pdivoom_favorite_manage_handle->image_list[i].image_id);
        }
        divoom_subscribe_local_pic_remove_first_image(
            pdivoom_favorite_manage_handle->image_list,
            &pdivoom_favorite_manage_handle->image_num, remove_num);
    }
    if(pdivoom_favorite_manage_handle->image_num == 0)
    {
        pdivoom_favorite_manage_handle->image_list =
            divoom_subscribe_local_pic_load_dynamic(
            DIVOOM_MANAGE_IMAGE_SUBSCRIBE_FAVORITE,
            &pdivoom_favorite_manage_handle->image_num,
            DIVOOM_FAVORITE_MANAGE_MAX_FILE_NUM,
            0);
        pdivoom_favorite_manage_handle->image_capacity =
            pdivoom_favorite_manage_handle->image_num;
    }
    if(pdivoom_favorite_manage_handle->image_num > 1)
    {
        qsort(pdivoom_favorite_manage_handle->image_list,
            (size_t)pdivoom_favorite_manage_handle->image_num,
            sizeof(divoom_image_hot_item),
            divoom_favorite_manage_compare_image_id);
    }
    pdivoom_favorite_manage_handle->update_time = divoom_database_manage_get_subscribe_image_last_update_time(DIVOOM_FAVORITE_IMAGE_TABLE_NAME, 0);
    pdivoom_favorite_manage_handle->next_request_time =
        g_divoom_favorite_manage_next_request_time;
    pdivoom_favorite_manage_handle->prandom_handle = divoom_random_create(pdivoom_favorite_manage_handle->image_num);
    pdivoom_favorite_manage_handle->use_time = 1;
    if(request_list)
    {
        divoom_favorite_manage_request_list_locked(
            pdivoom_favorite_manage_handle->update_time);
    }
    divoom_system_free_lock(pdivoom_favorite_manage_lock);
    return 1;
}

void divoom_favorite_manage_open(void)
{
    divoom_favorite_manage_open_internal(1);
}

void divoom_favorite_manage_close(void)
{
    divoom_system_get_lock(pdivoom_favorite_manage_lock);
    if(pdivoom_favorite_manage_handle == NULL)
    {
        divoom_system_free_lock(pdivoom_favorite_manage_lock);
        return;
    }
    pdivoom_favorite_manage_handle->use_time --;
    if(pdivoom_favorite_manage_handle->use_time == 0)
    {
        divoom_subscribe_local_pic_free_image_list(
            pdivoom_favorite_manage_handle->image_list,
            pdivoom_favorite_manage_handle->image_num);
        divoom_random_destroy(pdivoom_favorite_manage_handle->prandom_handle);
        divoom_system_free(pdivoom_favorite_manage_handle);
        pdivoom_favorite_manage_handle = NULL;
        divoom_system_free_lock(pdivoom_favorite_manage_lock);
        return;
    }
    divoom_system_free_lock(pdivoom_favorite_manage_lock);
}

/*********************************************************************
  Function:        divoom_album_manage_check_pic_id_in_album
  Description:     确认资源是否在队列中
  Input:           
  Return:          返回图片ＩＤ
**********************************************************************/

int divoom_favorite_manage_check_pic_id_in_queue(int image_id)
{
    if(pdivoom_favorite_manage_handle == NULL)
    {
        return 1;
    }
    return divoom_favorite_manage_find_image_index((uint32)image_id) >= 0;
}



/*********************************************************************
  Function:        divoom_favorite_manage_get_disp_image_id
  Description:     得到一个将要播放的图像
  Input:           none
  Return:          图像ＩＤ
**********************************************************************/

int divoom_favorite_manage_get_disp_image_id(uint32 *user_id, char *image_name)
{
    int index;
    
    if(pdivoom_favorite_manage_handle == NULL || (pdivoom_favorite_manage_handle->image_num == 0 && pdivoom_favorite_manage_handle->next_request_time))
    {
        return 0;
    }
    #if 1
    if(pdivoom_favorite_manage_handle->next_request_time < time(0) || pdivoom_favorite_manage_handle->image_num == 0)
    {
        divoom_favorite_manage_request_list(
            pdivoom_favorite_manage_handle->update_time);
    }
    #endif
    if(pdivoom_favorite_manage_handle->image_num <= 0)
        return 0;
    /* The list can grow after an initially empty collection was opened. */
    if(pdivoom_favorite_manage_handle->prandom_handle == NULL)
        pdivoom_favorite_manage_handle->prandom_handle = divoom_random_create(pdivoom_favorite_manage_handle->image_num);
    if(!divoom_random_set_max(pdivoom_favorite_manage_handle->prandom_handle,
        pdivoom_favorite_manage_handle->image_num))
        return 0;
    index = divoom_random_get(pdivoom_favorite_manage_handle->prandom_handle);
    if(index < 0 || index >= pdivoom_favorite_manage_handle->image_num)
        return 0;
    *user_id = pdivoom_favorite_manage_handle->image_list[index].user_id;
    strcpy(image_name ,pdivoom_favorite_manage_handle->image_list[index].image_name);
    return pdivoom_favorite_manage_handle->image_list[index].image_id;
}

/*********************************************************************
  Function:        divoom_favorite_manage_delete_item
  Description:     删除某个元素
  Input:           pimage_item
                   
                    
  Return:          none
**********************************************************************/

void divoom_favorite_manage_delete_item(uint32 image_id)
{
    int32 image_index;

    divoom_manage_image_del(DIVOOM_MANAGE_IMAGE_SUBSCRIBE_FAVORITE, &image_id, 1, 1);
    divoom_database_manage_delete_subscribe_image_item(DIVOOM_FAVORITE_IMAGE_TABLE_NAME, 0,  image_id);
    if(pdivoom_favorite_manage_handle)
    {
        image_index = divoom_favorite_manage_find_image_index(image_id);
        if(image_index >= 0)
        {
            pdivoom_favorite_manage_handle->image_num --;
            if(pdivoom_favorite_manage_handle->image_list[image_index].image_name != NULL)
            {
                divoom_system_free(
                    pdivoom_favorite_manage_handle->image_list[image_index].image_name);
            }
            if(image_index != pdivoom_favorite_manage_handle->image_num)
            {
                memmove(&pdivoom_favorite_manage_handle->image_list[image_index],
                    &pdivoom_favorite_manage_handle->image_list[image_index + 1],
                    (uint32)(pdivoom_favorite_manage_handle->image_num - image_index) *
                        sizeof(divoom_image_hot_item));
            }
        }
    }
}


/*********************************************************************
  Function:        divoom_favorite_manage_update_item
  Description:     新增加收藏列表
  Input:           pimage_item
                   
                    
  Return:          none
**********************************************************************/

static int32 divoom_favorite_manage_update_item_internal(
    Pdivoom_devcice_image_file pimage_item, int32 add_flag,
    Pdivoom_manage_image_item_info pdownload_item,
    int32 queue_download, int show_transfer)
{
    divoom_manage_image_item_info pcur_item = {0};
    

    if(pimage_item == NULL)
    {
        return 0;
    }
    
    if(add_flag)
    {
        //添加
        if(divoom_favorite_manage_check_pic_id_in_queue(pimage_item->image_id))
        {
            return 0;
        }
        if(pdivoom_favorite_manage_handle)
        {
            if(pdivoom_favorite_manage_handle->image_num >=
                DIVOOM_FAVORITE_MANAGE_MAX_FILE_NUM)
            {
                divoom_manage_image_del(
                    DIVOOM_MANAGE_IMAGE_SUBSCRIBE_FAVORITE,
                    &pdivoom_favorite_manage_handle->image_list[0].image_id,
                    1, 1);
                divoom_database_manage_delete_subscribe_image_item(
                    DIVOOM_FAVORITE_IMAGE_TABLE_NAME, 0,
                    (int)pdivoom_favorite_manage_handle->image_list[0].image_id);
                divoom_subscribe_local_pic_remove_first_image(
                    pdivoom_favorite_manage_handle->image_list,
                    &pdivoom_favorite_manage_handle->image_num, 1);
            }
            if(divoom_favorite_manage_append_image_sorted(
                (uint32)pimage_item->image_id,
                (uint32)pimage_item->user_id) == 0)
            {
                return 0;
            }
        }
        if(pdownload_item == NULL)
        {
            pdownload_item = &pcur_item;
        }
        pdownload_item->image_id = pimage_item->image_id;
        pdownload_item->utc_time = pimage_item->utc_time;
        strcpy(pdownload_item->file_addr,pimage_item->file_id);
        pdownload_item->pcallback = NULL;
        if(queue_download)
        {
            if(show_transfer)
            {
                divoom_manage_image_add_transfer(
                    DIVOOM_MANAGE_IMAGE_SUBSCRIBE_FAVORITE,
                    pdownload_item, 1);
            }
            else
            {
                divoom_manage_image_add(
                    DIVOOM_MANAGE_IMAGE_SUBSCRIBE_FAVORITE,
                    pdownload_item, 1);
            }
        }
        divoom_database_manage_add_subscribe_image_item(
            DIVOOM_FAVORITE_IMAGE_TABLE_NAME, 0,
            pimage_item->image_id, pimage_item->utc_time,
            pimage_item->file_id, pimage_item->user_id, "");
        return 1;
    }
    else
    {
        //删除
        divoom_favorite_manage_delete_item(pimage_item->image_id);
    }
    return 0;
}

void divoom_favorite_manage_update_item(Pdivoom_devcice_image_file pimage_item)
{
    int32 add_flag;
    int show_transfer;
    uint32 user_id;

    add_flag = pimage_item != NULL && pimage_item->size != 0;
    show_transfer = add_flag &&
        divoom_subscribe_manage_transfer_response_is_active();
    if(show_transfer)
    {
        divoom_manage_image_transfer_batch_begin();
    }
    divoom_database_manage_txn_begin();
    divoom_favorite_manage_update_item_internal(
        pimage_item, add_flag, NULL, 1, show_transfer);
    divoom_database_manage_txn_end();
    if(show_transfer)
    {
        divoom_manage_image_transfer_batch_end();
    }
}


/*********************************************************************
  Function:        divoom_favorite_manage_update_list
  Description:     新增加收藏列表
  Input:           pimage_list
                   
                    
  Return:          none
**********************************************************************/

void divoom_favorite_manage_update_list(Pdivoom_devcice_image_list pimage_list)
{
    int i;
    int32 download_num = 0;
    int32 user_num;
    int show_transfer;
    Pdivoom_manage_image_item_info pdownload_list = NULL;
    Pdivoom_manage_image_item_info pdownload_item;
    Pdivoom_devcice_image_file psrc_item;
    
    if(pimage_list == NULL)
    {
        return;
    }
    if(pdivoom_favorite_manage_handle == NULL &&
        !divoom_subscribe_manage_transfer_response_is_active())
    {
        return;
    }
    /* 表盘切换期间旧句柄可能已释放；本次回包只重建数据上下文，不再发请求。 */
    if(!divoom_favorite_manage_open_internal(0))
    {
        divoom_subscribe_manage_transfer_response_defer();
        return;
    }
    {
        if(pimage_list->num > 0 && (uint32)pimage_list->num <=
            U32_MAX / (uint32)sizeof(divoom_manage_image_item_info))
        {
            pdownload_list = divoom_system_malloc(
                (uint32)pimage_list->num *
                    sizeof(divoom_manage_image_item_info));
            if(pdownload_list != NULL)
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
        divoom_print("del image num: %d\n", pimage_list->unlike_num);
        for(i = 0; i < pimage_list->unlike_num; i ++)
        {
            divoom_favorite_manage_delete_item(pimage_list->unlike_list[i]);        
        }
        
        for(i = 0; i < pimage_list->num; i ++)
        {
            psrc_item = &pimage_list->pfile_list[i];
            if(psrc_item->utc_time > pdivoom_favorite_manage_handle->update_time)
            {
                pdivoom_favorite_manage_handle->update_time = psrc_item->utc_time;
            }
            pdownload_item = pdownload_list != NULL ?
                &pdownload_list[download_num] : NULL;
            if(divoom_favorite_manage_update_item_internal(
                psrc_item, 1, pdownload_item,
                pdownload_list == NULL, show_transfer))
            {
                download_num++;
            }
        }
        divoom_database_manage_txn_end();
        if(pdownload_list != NULL && download_num > 0)
        {
            if(show_transfer)
            {
                divoom_manage_image_add_transfer(
                    DIVOOM_MANAGE_IMAGE_SUBSCRIBE_FAVORITE,
                    pdownload_list, (uint32)download_num);
            }
            else
            {
                divoom_manage_image_add(
                    DIVOOM_MANAGE_IMAGE_SUBSCRIBE_FAVORITE,
                    pdownload_list, (uint32)download_num);
            }
        }
        if(pdownload_list != NULL)
        {
            divoom_system_free(pdownload_list);
        }
        if(show_transfer)
        {
            divoom_manage_image_transfer_batch_end();
        }
        divoom_favorite_manage_close();
    }
}

/*********************************************************************
  Function:        divoom_favorite_manage_get_url_addr
  Description:     获取指定文件的网络地址
  Input:           model_id
                   image_id
                   url_addr
                    
  Return:          image_flag
**********************************************************************/

int divoom_favorite_manage_get_url_addr(int model_id, uint32 image_id, char *url_addr)
{
    return divoom_database_manage_get_subscribe_image_pic_addr(DIVOOM_FAVORITE_IMAGE_TABLE_NAME, image_id, url_addr);
}

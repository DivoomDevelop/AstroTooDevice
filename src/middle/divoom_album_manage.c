#include "divoom.h"
#include "divoom_subscribe_local_pic.h"

#define DIVOOM_ALBUM_MANAGE_MAX_FILE_NUM 300
#define DIVOOM_ALBUM_MANAGE_LOAD_FILE_TIME (6*3600)


typedef struct _divoom_album_manage_item
{
    int     use_time;
    int     next_request_time;  //下一次请求的时间
    int     album_id;   //当前正在使用的，用来更新数据使用
    int     update_time;
    int     pic_num;
    int     pic_capacity;
    int     read_pos;
    void    *prandom_handle;
    Pdivoom_image_hot_item pic_list;
}divoom_album_manage_item, *Pdivoom_album_manage_item;


typedef struct _divoom_album_manage_ctrl
{
    void    *album_queue;         
}divoom_album_manage_ctrl, *Pdivoom_album_manage_ctrl;

Pdivoom_album_manage_ctrl pdivoom_album_manage_handle = NULL;

int divoom_album_manage_find_file_compare_album_id(void *item1, void *item2)
{
    int *album_id = (int *)item1;
    Pdivoom_album_manage_item pcache_item = (Pdivoom_album_manage_item)item2;

    return *album_id != pcache_item->album_id;
    
}

static int divoom_album_manage_compare_image_id(
    const void *item1, const void *item2)
{
    const divoom_image_hot_item *image1 =
        (const divoom_image_hot_item *)item1;
    const divoom_image_hot_item *image2 =
        (const divoom_image_hot_item *)item2;

    return image1->image_id < image2->image_id ? -1 :
        image1->image_id > image2->image_id;
}


static int32 divoom_album_manage_append_image_sorted(
    Pdivoom_album_manage_item palbum_item, uint32 image_id, uint32 user_id)
{
    int32 low;
    int32 high;
    int32 mid;
    int32 insert_pos;
    int32 old_num;
    divoom_image_hot_item new_item;

    if(palbum_item == NULL)
    {
        return 0;
    }
    old_num = palbum_item->pic_num;
    if(divoom_subscribe_local_pic_append_image(
        &palbum_item->pic_list, &palbum_item->pic_num,
        &palbum_item->pic_capacity, 0, image_id, "", user_id) == 0)
    {
        return 0;
    }
    low = 0;
    high = old_num;
    while(low < high)
    {
        mid = low + (high - low) / 2;
        if(palbum_item->pic_list[mid].image_id < image_id)
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
        new_item = palbum_item->pic_list[old_num];
        memmove(&palbum_item->pic_list[insert_pos + 1],
            &palbum_item->pic_list[insert_pos],
            (uint32)(old_num - insert_pos) * sizeof(divoom_image_hot_item));
        palbum_item->pic_list[insert_pos] = new_item;
    }
    return 1;
}

/*********************************************************************
  Function:        divoom_album_manage_open_album_id
  Description:     打开专辑资源
  Input:           
  Return:          
**********************************************************************/


void *divoom_album_manage_open_album_id(int album_id)
{
    int32 i;
    int32 remove_num;
    Pdivoom_album_manage_item      palbum_item;
    
    palbum_item = divoom_queue_get_cmp(pdivoom_album_manage_handle->album_queue, &album_id, divoom_album_manage_find_file_compare_album_id);
    if(palbum_item)
    {
        palbum_item->use_time ++;
        return palbum_item;
    }
    divoom_print("open album id: %d\n", album_id);
    palbum_item = divoom_system_malloc(sizeof(divoom_album_manage_item));
    if (palbum_item == NULL)
    {
        return NULL;
    }
    palbum_item->album_id = album_id;
    palbum_item->pic_list =
        divoom_database_manage_get_subscribe_image_pic_list(
            DIVOOM_ALBUM_MANAGE_TABLE_NAME, album_id,
            &palbum_item->pic_num);
    palbum_item->pic_capacity = palbum_item->pic_num;
    if(palbum_item->pic_num > DIVOOM_ALBUM_MANAGE_MAX_FILE_NUM)
    {
        remove_num = palbum_item->pic_num -
            DIVOOM_ALBUM_MANAGE_MAX_FILE_NUM;
        for(i = 0; i < remove_num; i++)
        {
            divoom_database_manage_delete_subscribe_image_item(
                DIVOOM_ALBUM_MANAGE_TABLE_NAME, album_id,
                (int)palbum_item->pic_list[i].image_id);
        }
        divoom_subscribe_local_pic_remove_first_image(
            palbum_item->pic_list, &palbum_item->pic_num, remove_num);
    }
    if(palbum_item->pic_num == 0)
    {
        palbum_item->pic_list = divoom_subscribe_local_pic_load_dynamic(
            DIVOOM_MANAGE_IMAGE_ALBUM_ID_LIST_PIC,
            &palbum_item->pic_num,
            DIVOOM_ALBUM_MANAGE_MAX_FILE_NUM,
            0);
        palbum_item->pic_capacity = palbum_item->pic_num;
    }
    if(palbum_item->pic_num > 1)
    {
        qsort(palbum_item->pic_list, (size_t)palbum_item->pic_num,
            sizeof(divoom_image_hot_item),
            divoom_album_manage_compare_image_id);
    }
    palbum_item->prandom_handle = divoom_random_create(palbum_item->pic_num);
    divoom_queue_push(pdivoom_album_manage_handle->album_queue, palbum_item);    
    palbum_item->use_time = 1;
    return palbum_item;

}

/*********************************************************************
  Function:        divoom_album_manage_close_album_id
  Description:     关闭专辑资源
  Input:           
  Return:          
**********************************************************************/

void divoom_album_manage_close_album_id(int album_id)
{
    Pdivoom_album_manage_item      palbum_item;

    palbum_item = divoom_queue_get_cmp(pdivoom_album_manage_handle->album_queue, &album_id, divoom_album_manage_find_file_compare_album_id);
    if(palbum_item)
    {
        palbum_item->use_time --;
        if(palbum_item->use_time == 0)
        {
            palbum_item = divoom_queue_pop_cmp(pdivoom_album_manage_handle->album_queue, &album_id, divoom_album_manage_find_file_compare_album_id);
            divoom_print("close album id: %d\n", album_id);
            divoom_subscribe_local_pic_free_image_list(
                palbum_item->pic_list, palbum_item->pic_num);
            divoom_random_destroy(palbum_item->prandom_handle);
            divoom_system_free(palbum_item);
        }
    }

}



void divoom_album_manage_clear_album_id(int album_id)
{
    Pdivoom_album_manage_item      palbum_item;
    palbum_item = divoom_album_manage_open_album_id(album_id);
    if(palbum_item)
    {
        if(palbum_item->pic_num)
        {
            uint32 *image_id_list;

            image_id_list = divoom_system_malloc(sizeof(uint32)*palbum_item->pic_num);
            for(int i = 0; i < palbum_item->pic_num; i++)
            {
                image_id_list[i] = palbum_item->pic_list[i].image_id;
            }
            divoom_manage_image_del(DIVOOM_MANAGE_IMAGE_ALBUM_ID_LIST_PIC , image_id_list, palbum_item->pic_num, 0);
            divoom_system_free(image_id_list);
        }
        divoom_album_manage_close_album_id(album_id);
    }
    divoom_print("we will delete album id; %d\n",album_id);
    divoom_database_manage_delete_subscribe_image_all(DIVOOM_ALBUM_MANAGE_TABLE_NAME, album_id);
}



/*********************************************************************
  Function:        divoom_album_manage_change_album_id
  Description:     某个表盘切换专辑，如果这个专辑没有被使用，将清理相关资源
  Input:           clock_id
                   clock_pos
                   parent_id
                   cur_album_id
                   new_album_id
                    
  Return:           none
**********************************************************************/

void divoom_album_manage_change_album_id(int clock_id, int clock_pos, int parent_id, int cur_album_id,int new_album_id)
{
    if(cur_album_id && cur_album_id != new_album_id)
    {
        divoom_print("we will remove album id; %d\n",cur_album_id);
        divoom_database_manage_delete_divoom_image_config_item(DIVOOM_ALBUM_MANAGE_CONFIG_TABLE_NAME, clock_id, clock_pos, parent_id, cur_album_id);
        if(divoom_database_manage_check_divoom_image_type_exist(DIVOOM_ALBUM_MANAGE_CONFIG_TABLE_NAME, cur_album_id) == 0)
        {
            //如果没有表盘使用这个专辑，将清空相关数据
            divoom_album_manage_clear_album_id(cur_album_id);
        }
    }
    if(new_album_id == 0)
    {
        return;
    }
    if(divoom_database_manage_check_divoom_image_type_exist(DIVOOM_ALBUM_MANAGE_CONFIG_TABLE_NAME, new_album_id) == 0)
    {
        //如果第一次使用将请求列表
        divoom_print("we will new album id; %d\n",cur_album_id);
        divoom_net_app_send_packet(DIVOOM_NET_COMM_GET_ALBUM_LIST, DIVOOM_SERVER_TOPIC, &new_album_id); 
    }
    divoom_print("we will add album id; %d\n",cur_album_id);
    divoom_database_manage_add_divoom_image_config_item(DIVOOM_ALBUM_MANAGE_CONFIG_TABLE_NAME, clock_id, clock_pos, parent_id, new_album_id);
}

/*********************************************************************
  Function:        divoom_album_manage_reset
  Description:     复位模块，全部删除相关数据
  Input:           none
                    
  Return:           none
**********************************************************************/

void divoom_album_manage_reset(void)
{
    divoom_manage_image_remove(DIVOOM_MANAGE_IMAGE_ALBUM_ID_LIST_PIC, 1);
    divoom_database_manage_clear_all_divoom_image(DIVOOM_FAVORITE_IMAGE_TABLE_NAME);
}


/*********************************************************************
  Function:        divoom_album_manage_init
  Description:     初始化模块，将相关的文件从spi中load出来
  Input:           file_path  SD路径 
                    
  Return:           none
**********************************************************************/

void divoom_album_manage_init(void)
{
    if(pdivoom_album_manage_handle)
    {
        return;
    }

    pdivoom_album_manage_handle = divoom_system_malloc(sizeof(divoom_album_manage_ctrl));
    if(pdivoom_album_manage_handle == NULL)
    {
        return;
    }
    memset(pdivoom_album_manage_handle, 0, sizeof(divoom_album_manage_ctrl));
    pdivoom_album_manage_handle->album_queue = divoom_queue_init(1000, __func__, __LINE__);
    
	char *table_item = "id INTEGER PRIMARY KEY  AUTOINCREMENT," \
		"image_type INTEGER DEFAULT 0," \
		"image_id INTEGER DEFAULT 0," \
		"author_id INTEGER DEFAULT 0," \
		"image_name CHAR(200)," \
		"update_time INTEGER DEFAULT 0," \
		"file_addr CHAR(64)" ;

	divoom_database_manage_add_new_table(DIVOOM_ALBUM_MANAGE_TABLE_NAME, table_item);
    
    divoom_database_manage_add_table_index(DIVOOM_ALBUM_MANAGE_TABLE_NAME, "image_type");
    divoom_database_manage_add_table_index(DIVOOM_ALBUM_MANAGE_TABLE_NAME, "image_id");
    divoom_database_manage_add_table_index(DIVOOM_ALBUM_MANAGE_TABLE_NAME, "update_time");
    
	char *table_config_item = "id INTEGER PRIMARY KEY     AUTOINCREMENT," \
		"clock_id INTEGER DEFAULT 0," \
		"clock_pos INTEGER DEFAULT 0," \
		"parent_id INTEGER DEFAULT 0," \
		"image_type INTEGER DEFAULT 0," \
		"update_time INTEGER DEFAULT 0";

	divoom_database_manage_add_new_table(DIVOOM_ALBUM_MANAGE_CONFIG_TABLE_NAME, table_config_item);
    divoom_database_manage_add_table_index(DIVOOM_ALBUM_MANAGE_CONFIG_TABLE_NAME, "clock_id");
    divoom_database_manage_add_table_index(DIVOOM_ALBUM_MANAGE_CONFIG_TABLE_NAME, "clock_pos");
    divoom_database_manage_add_table_index(DIVOOM_ALBUM_MANAGE_CONFIG_TABLE_NAME, "parent_id");
    divoom_database_manage_add_table_index(DIVOOM_ALBUM_MANAGE_CONFIG_TABLE_NAME, "image_type");
    divoom_database_manage_add_table_index(DIVOOM_ALBUM_MANAGE_CONFIG_TABLE_NAME, "update_time");
}



/*********************************************************************
  Function:        divoom_album_manage_get_random_pic_id
  Description:     得到某类型图片的ＩＤ
  Input:           
  Return:          返回图片ＩＤ
**********************************************************************/

uint32 divoom_album_manage_get_random_pic_id(int album_id, int *user_id, char *image_name)
{
    int index;
    Pdivoom_album_manage_item      palbum_item;
    
    if(pdivoom_album_manage_handle)
    {
        
        palbum_item = divoom_queue_get_cmp(pdivoom_album_manage_handle->album_queue, &album_id, divoom_album_manage_find_file_compare_album_id);
        if (palbum_item == NULL)
        {
            palbum_item = divoom_album_manage_open_album_id(album_id);
        }
        if(palbum_item == NULL || palbum_item->pic_num == 0)
        {
            return 0;
        }
        #if 1
        if(palbum_item->next_request_time < time(0))
        {
            //六个小时重新请求一次数据
            palbum_item->next_request_time = time(0) + DIVOOM_ALBUM_MANAGE_LOAD_FILE_TIME;
            divoom_net_app_send_packet(DIVOOM_NET_COMM_GET_ALBUM_LIST, DIVOOM_SERVER_TOPIC, &album_id); 
        }
        #endif
        /* An album may receive its first pictures after it was opened empty. */
        if(palbum_item->prandom_handle == NULL)
            palbum_item->prandom_handle = divoom_random_create(palbum_item->pic_num);
        if(!divoom_random_set_max(palbum_item->prandom_handle, palbum_item->pic_num))
            return 0;
        index = divoom_random_get(palbum_item->prandom_handle);
        if(index < 0 || index >= palbum_item->pic_num)
            return 0;
        *user_id = palbum_item->pic_list[index].user_id;
        strcpy(image_name, palbum_item->pic_list[index].image_name);
        return palbum_item->pic_list[index].image_id;
    }
    return 0;
}

/*********************************************************************
  Function:        divoom_album_manage_check_pic_id_in_album
  Description:     确认资源是否在队列中
  Input:           
  Return:          返回图片ＩＤ
**********************************************************************/

int divoom_album_manage_check_pic_id_in_album(Pdivoom_album_manage_item palbum_item, int image_id)
{
    int32 low;
    int32 high;
    int32 mid;
    uint32 target_id;

    if(palbum_item == NULL || palbum_item->pic_num == 0)
    {
        return 0;
    }
    target_id = (uint32)image_id;
    low = 0;
    high = palbum_item->pic_num - 1;
    while(low <= high)
    {
        mid = low + (high - low) / 2;
        if(palbum_item->pic_list[mid].image_id == target_id)
        {
            return 1;
        }
        if(palbum_item->pic_list[mid].image_id < target_id)
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
  Function:        divoom_album_manage_update_file_list
  Description:     更新图片列表
  Input:           
                    
  Return:          none
**********************************************************************/

void divoom_album_manage_update_file_list(Pdivoom_devcice_image_list pimage_list)
{
    int32 i;
    int32 user_num;
    int32 download_num = 0;
    int show_transfer;
    divoom_manage_image_item_info pcur_item = {0};
    Pdivoom_manage_image_item_info pdownload_list = NULL;
    Pdivoom_manage_image_item_info pdownload_item;
    Pdivoom_album_manage_item      palbum_item;

    if(pimage_list == NULL || pimage_list->pfile_list == NULL ||
        pimage_list->num <= 0)
    {
        return;
    }
    palbum_item = divoom_album_manage_open_album_id(pimage_list->album_id);
    if(palbum_item == NULL)
    {
        return;
    }
    if((uint32)pimage_list->num <=
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
    for(i = 0; i < pimage_list->num; i ++)
    {
        if(divoom_album_manage_check_pic_id_in_album(palbum_item, pimage_list->pfile_list[i].image_id))
        {
            continue;
        }
        if(palbum_item->pic_num >= DIVOOM_ALBUM_MANAGE_MAX_FILE_NUM)
        {
            divoom_manage_image_del(DIVOOM_MANAGE_IMAGE_ALBUM_ID_LIST_PIC,
                &palbum_item->pic_list[0].image_id, 1, 1);
            divoom_database_manage_delete_subscribe_image_item(
                DIVOOM_ALBUM_MANAGE_TABLE_NAME, pimage_list->album_id,
                (int)palbum_item->pic_list[0].image_id);
            divoom_subscribe_local_pic_remove_first_image(
                palbum_item->pic_list, &palbum_item->pic_num, 1);
        }
        if(divoom_album_manage_append_image_sorted(palbum_item,
            (uint32)pimage_list->pfile_list[i].image_id,
            (uint32)pimage_list->pfile_list[i].user_id) == 0)
        {
            divoom_print("album image append failed image_id=%d\n",
                pimage_list->pfile_list[i].image_id);
            continue;
        }
        if(pdownload_list != NULL)
        {
            pdownload_item = &pdownload_list[download_num++];
        }
        else
        {
            pdownload_item = &pcur_item;
        }
        pdownload_item->image_id = pimage_list->pfile_list[i].image_id;
        strcpy(pdownload_item->file_addr,pimage_list->pfile_list[i].file_id);
        pdownload_item->pcallback = NULL;
        if(pdownload_list == NULL)
        {
            if(show_transfer)
            {
                divoom_manage_image_add_transfer(
                    DIVOOM_MANAGE_IMAGE_ALBUM_ID_LIST_PIC,
                    pdownload_item, 1);
            }
            else
            {
                divoom_manage_image_add(
                    DIVOOM_MANAGE_IMAGE_ALBUM_ID_LIST_PIC,
                    pdownload_item, 1);
            }
        }
        divoom_database_manage_add_subscribe_image_item(
            DIVOOM_ALBUM_MANAGE_TABLE_NAME, pimage_list->album_id,
            pimage_list->pfile_list[i].image_id,
            pimage_list->pfile_list[i].utc_time,
            pimage_list->pfile_list[i].file_id,
            pimage_list->pfile_list[i].user_id, "");
        
    }
    divoom_database_manage_txn_end();
    if(pdownload_list != NULL && download_num > 0)
    {
        if(show_transfer)
        {
            divoom_manage_image_add_transfer(
                DIVOOM_MANAGE_IMAGE_ALBUM_ID_LIST_PIC,
                pdownload_list, (uint32)download_num);
        }
        else
        {
            divoom_manage_image_add(
                DIVOOM_MANAGE_IMAGE_ALBUM_ID_LIST_PIC,
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
    divoom_album_manage_close_album_id(pimage_list->album_id);
    
}

/*********************************************************************
  Function:        divoom_album_manage_get_url_addr
  Description:     获取指定文件的网络地址
  Input:           model_id
                   image_id
                   url_addr
                    
  Return:          image_flag
**********************************************************************/

int divoom_album_manage_get_url_addr(int model_id, uint32 image_id, char *url_addr)
{
    return divoom_database_manage_get_subscribe_image_pic_addr(DIVOOM_ALBUM_MANAGE_TABLE_NAME, image_id, url_addr);
}

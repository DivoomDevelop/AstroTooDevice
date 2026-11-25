#include "divoom.h"
#include "divoom_subscribe_local_pic.h"

#define DIVOOM_USERIMAGELIST_MANAGE_MAX_FILE_NUM 300
#define DIVOOM_USERIMAGELIST_MANAGE_LOAD_FILE_TIME (3600)


typedef struct _divoom_userimagelist_manage_item
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
}divoom_userimagelist_manage_item, *Pdivoom_userimagelist_manage_item;


typedef struct _divoom_userimagelist_manage_ctrl
{
    int     tmp_album_id;
    int     change_flag;
    void    *lock_handle;
    void    *album_queue;         
}divoom_userimagelist_manage_ctrl, *Pdivoom_userimagelist_manage_ctrl;

Pdivoom_userimagelist_manage_ctrl pdivoom_userimagelist_manage_handle = NULL;

static void divoom_userimagelist_manage_free_album_item(void *item)
{
    Pdivoom_userimagelist_manage_item palbum_item =
        (Pdivoom_userimagelist_manage_item)item;

    if(palbum_item == NULL)
    {
        return;
    }
    divoom_subscribe_local_pic_free_image_list(
        palbum_item->pic_list, palbum_item->pic_num);
    divoom_random_destroy(palbum_item->prandom_handle);
    divoom_system_free(palbum_item);
}


/*********************************************************************
  Function:        divoom_userimagelist_manage_set_temp_album_id
  Description:     返回临时播放列表ＩＤ
  Input:           
  Return:          
**********************************************************************/


int divoom_userimagelist_manage_set_temp_album_id(int playlist_id)
{
    if(pdivoom_userimagelist_manage_handle == NULL)
    {
        divoom_userimagelist_manage_init();
    }
    if(playlist_id > 0 && playlist_id != pdivoom_userimagelist_manage_handle->tmp_album_id)
    {
        divoom_userimagelist_manage_change_album_id(0, 0, 0, pdivoom_userimagelist_manage_handle->tmp_album_id, playlist_id);
        pdivoom_userimagelist_manage_handle->tmp_album_id = playlist_id;
        pdivoom_userimagelist_manage_handle->change_flag = 1;
    }
    return pdivoom_userimagelist_manage_handle->tmp_album_id;
}

int divoom_userimagelist_manage_find_file_compare_album_id(void *item1, void *item2)
{
    int *album_id = (int *)item1;
    Pdivoom_userimagelist_manage_item pcache_item = (Pdivoom_userimagelist_manage_item)item2;

    return *album_id != pcache_item->album_id;
    
}

static int divoom_userimagelist_manage_compare_image_id(
    const void *item1, const void *item2)
{
    const divoom_image_hot_item *image1 =
        (const divoom_image_hot_item *)item1;
    const divoom_image_hot_item *image2 =
        (const divoom_image_hot_item *)item2;

    return image1->image_id < image2->image_id ? -1 :
        image1->image_id > image2->image_id;
}

static int32 divoom_userimagelist_manage_find_image_index(
    Pdivoom_userimagelist_manage_item palbum_item, uint32 image_id)
{
    int32 low = 0;
    int32 high;
    int32 mid;

    if(palbum_item == NULL || palbum_item->pic_num <= 0)
    {
        return -1;
    }
    high = palbum_item->pic_num - 1;
    while(low <= high)
    {
        mid = low + (high - low) / 2;
        if(palbum_item->pic_list[mid].image_id == image_id)
        {
            return mid;
        }
        if(palbum_item->pic_list[mid].image_id < image_id)
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

static int32 divoom_userimagelist_manage_append_image_sorted(
    Pdivoom_userimagelist_manage_item palbum_item, uint32 image_id,
    uint32 user_id)
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
    if(!divoom_random_insert(palbum_item->prandom_handle, insert_pos))
    {
        return 0;
    }
    if(divoom_subscribe_local_pic_append_image(
        &palbum_item->pic_list, &palbum_item->pic_num,
        &palbum_item->pic_capacity, 0, image_id, "", user_id) == 0)
    {
        divoom_random_remove(palbum_item->prandom_handle, insert_pos);
        return 0;
    }
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
  Function:        divoom_userimagelist_manage_open_album_id
  Description:     打开专辑资源
  Input:           
  Return:          
**********************************************************************/


void *divoom_userimagelist_manage_open_album_id_inner(int album_id, int update_flag)
{
    int32 i;
    int32 remove_num;
    Pdivoom_userimagelist_manage_item      palbum_item;

    
    palbum_item = divoom_queue_get_cmp(pdivoom_userimagelist_manage_handle->album_queue, &album_id, divoom_userimagelist_manage_find_file_compare_album_id);
    if(palbum_item)
    {
        palbum_item->use_time ++;
        return palbum_item;
    }
    divoom_print("open album id: %d\n", album_id);
    palbum_item = divoom_system_malloc(sizeof(divoom_userimagelist_manage_item));
    if (palbum_item == NULL)
    {
        return NULL;
    }
    palbum_item->album_id = album_id;
    
    palbum_item->update_time = divoom_database_manage_get_divoom_image_update_tiem(DIVOOM_USERIMAGELIST_CONFIGTABLE_NAME, album_id);
    
    palbum_item->pic_list =
        divoom_database_manage_get_subscribe_image_pic_list_by_updatetime(
            DIVOOM_USERIMAGELIST_TABLE_NAME, album_id,
            &palbum_item->pic_num);
    palbum_item->pic_capacity = palbum_item->pic_num;
    if(palbum_item->pic_num > DIVOOM_USERIMAGELIST_MANAGE_MAX_FILE_NUM)
    {
        remove_num = palbum_item->pic_num -
            DIVOOM_USERIMAGELIST_MANAGE_MAX_FILE_NUM;
        for(i = 0; i < remove_num; i++)
        {
            divoom_database_manage_delete_subscribe_image_item(
                DIVOOM_USERIMAGELIST_TABLE_NAME, album_id,
                (int)palbum_item->pic_list[i].image_id);
        }
        divoom_subscribe_local_pic_remove_first_image(
            palbum_item->pic_list, &palbum_item->pic_num, remove_num);
    }
    if (palbum_item->pic_num == 0)
    {
        palbum_item->pic_list = divoom_subscribe_local_pic_load_dynamic(
            DIVOOM_MANAGE_IMAGE_USER_IMAGE_LIST_PIC,
            &palbum_item->pic_num,
            DIVOOM_USERIMAGELIST_MANAGE_MAX_FILE_NUM,
            0);
        palbum_item->pic_capacity = palbum_item->pic_num;
    }
    if(palbum_item->pic_num > 1)
    {
        qsort(palbum_item->pic_list, (size_t)palbum_item->pic_num,
            sizeof(divoom_image_hot_item),
            divoom_userimagelist_manage_compare_image_id);
    }
    /* Reserve the bounded album capacity so edits can move shuffle state without allocation. */
    palbum_item->prandom_handle = divoom_random_create(
        DIVOOM_USERIMAGELIST_MANAGE_MAX_FILE_NUM);
    if(palbum_item->prandom_handle == NULL ||
        !divoom_random_set_max(palbum_item->prandom_handle, palbum_item->pic_num))
    {
        divoom_userimagelist_manage_free_album_item(palbum_item);
        return NULL;
    }
    palbum_item->use_time = 1;
    if(!divoom_queue_push(pdivoom_userimagelist_manage_handle->album_queue, palbum_item))
    {
        divoom_userimagelist_manage_free_album_item(palbum_item);
        return NULL;
    }
    
    #if 1
    divoom_device_playlist_image_list_item playlist_info ={0};
    playlist_info.file_size = 256;
    playlist_info.playlist_id = album_id;
    playlist_info.update_time = palbum_item->update_time;
    if(update_flag)
    {
        divoom_net_app_send_packet(DIVOOM_NET_COMM_PLAY_LIST_GET_IMAGE_LIST, DIVOOM_SERVER_TOPIC, &playlist_info); 
    }
    #endif
    return palbum_item;

}

/*********************************************************************
  Function:        divoom_userimagelist_manage_try_open_album_id
  Description:     如果没有打开，将直接打开专辑资源
  Input:           
  Return:          
**********************************************************************/


void *divoom_userimagelist_manage_try_open_album_id(int album_id)
{
    Pdivoom_userimagelist_manage_item      palbum_item;
    divoom_system_get_lock(pdivoom_userimagelist_manage_handle->lock_handle);
    
    palbum_item = divoom_queue_get_cmp(pdivoom_userimagelist_manage_handle->album_queue, &album_id, divoom_userimagelist_manage_find_file_compare_album_id);
    if(palbum_item == NULL)
    {
        palbum_item = divoom_userimagelist_manage_open_album_id_inner(album_id, 1);
    }
    divoom_system_free_lock(pdivoom_userimagelist_manage_handle->lock_handle);
    return palbum_item;
}

/*********************************************************************
  Function:        divoom_userimagelist_manage_open_album_id
  Description:     打开专辑资源
  Input:           
  Return:          
**********************************************************************/


void *divoom_userimagelist_manage_open_album_id(int album_id)
{
    void *handle;
    divoom_system_get_lock(pdivoom_userimagelist_manage_handle->lock_handle);
    handle = divoom_userimagelist_manage_open_album_id_inner(album_id, 1);
    divoom_system_free_lock(pdivoom_userimagelist_manage_handle->lock_handle);
    return handle;
}

/*********************************************************************
  Function:        divoom_userimagelist_manage_close_album_id
  Description:     关闭专辑资源
  Input:           
  Return:          
**********************************************************************/

void divoom_userimagelist_manage_close_album_id_inner(int album_id)
{
    Pdivoom_userimagelist_manage_item      palbum_item;
    
    palbum_item = divoom_queue_get_cmp(pdivoom_userimagelist_manage_handle->album_queue, &album_id, divoom_userimagelist_manage_find_file_compare_album_id);
    if(palbum_item)
    {
        palbum_item->use_time --;
        if(palbum_item->use_time == 0)
        {
            palbum_item = divoom_queue_pop_cmp(pdivoom_userimagelist_manage_handle->album_queue, &album_id, divoom_userimagelist_manage_find_file_compare_album_id);
            divoom_print("close album id: %d\n", album_id);
            divoom_userimagelist_manage_free_album_item(palbum_item);
        }
    }

}


/*********************************************************************
  Function:        divoom_userimagelist_manage_close_album_id
  Description:     关闭专辑资源
  Input:           
  Return:          
**********************************************************************/

void divoom_userimagelist_manage_close_album_id(int album_id)
{
    divoom_system_get_lock(pdivoom_userimagelist_manage_handle->lock_handle);
    divoom_userimagelist_manage_close_album_id_inner(album_id);
    divoom_system_free_lock(pdivoom_userimagelist_manage_handle->lock_handle);
}


void divoom_userimagelist_manage_clear_album_id(int album_id)
{
    Pdivoom_userimagelist_manage_item      palbum_item;
    
    palbum_item = divoom_userimagelist_manage_open_album_id_inner(album_id, 0);
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
            divoom_manage_image_del(DIVOOM_MANAGE_IMAGE_USER_IMAGE_LIST_PIC , image_id_list, palbum_item->pic_num, 0);
            divoom_system_free(image_id_list);
        }
        divoom_userimagelist_manage_close_album_id_inner(album_id);
    }
    divoom_print("we will delete album id; %d\n",album_id);
    divoom_database_manage_delete_subscribe_image_all(DIVOOM_USERIMAGELIST_TABLE_NAME, album_id);
}



/*********************************************************************
  Function:        divoom_userimagelist_manage_change_album_id
  Description:     某个表盘切换专辑，如果这个专辑没有被使用，将清理相关资源
  Input:           clock_id
                   clock_pos
                   parent_id
                   cur_album_id
                   new_album_id
                    
  Return:           none
**********************************************************************/

void divoom_userimagelist_manage_change_album_id(int clock_id, int clock_pos, int parent_id, int cur_album_id,int new_album_id)
{
    if(cur_album_id == 0 && new_album_id == 0)
    {
        divoom_print("pass the change!\n");
        return;
    }
    divoom_system_get_lock(pdivoom_userimagelist_manage_handle->lock_handle);
    divoom_print("we will change : %d, %d, %d, %d, %d\n",clock_id,  clock_pos,  parent_id,  cur_album_id, new_album_id);
    if(cur_album_id && cur_album_id != new_album_id)
    {
        divoom_print("we will remove album id; %d\n",cur_album_id);
        
        Pdivoom_userimagelist_manage_item      palbum_item;
        
        palbum_item = divoom_queue_get_cmp(pdivoom_userimagelist_manage_handle->album_queue, &cur_album_id, divoom_userimagelist_manage_find_file_compare_album_id);
        if(palbum_item)
        {
            divoom_userimagelist_manage_close_album_id_inner(cur_album_id);
        }
        divoom_database_manage_delete_divoom_image_config_item(DIVOOM_USERIMAGELIST_CONFIGTABLE_NAME, clock_id, clock_pos, parent_id, cur_album_id);
        if(divoom_database_manage_check_divoom_image_type_exist(DIVOOM_USERIMAGELIST_CONFIGTABLE_NAME, cur_album_id) == 0)
        {
            //如果没有表盘使用这个专辑，将清空相关数据
            divoom_userimagelist_manage_clear_album_id(cur_album_id);
        }
    }
    if(new_album_id == 0)
    {
        divoom_system_free_lock(pdivoom_userimagelist_manage_handle->lock_handle);
        return;
    }
    if(divoom_database_manage_check_divoom_image_type_exist(DIVOOM_USERIMAGELIST_CONFIGTABLE_NAME, new_album_id) == 0)
    {
        //如果第一次使用将请求列表
        divoom_print("we will new album id; %d\n",new_album_id);
        divoom_device_playlist_image_list_item playlist_info ={0};

        playlist_info.file_size = 256;
        playlist_info.playlist_id = new_album_id;
        playlist_info.update_time = 0;
        divoom_net_app_send_packet(DIVOOM_NET_COMM_PLAY_LIST_GET_IMAGE_LIST, DIVOOM_SERVER_TOPIC, &playlist_info); 
    }
    divoom_print("we will add album id; %d\n",new_album_id);
    divoom_database_manage_add_divoom_image_config_item(DIVOOM_USERIMAGELIST_CONFIGTABLE_NAME, clock_id, clock_pos, parent_id, new_album_id);
    divoom_system_free_lock(pdivoom_userimagelist_manage_handle->lock_handle);
}

/*********************************************************************
  Function:        divoom_userimagelist_manage_change_album_id
  Description:     某个表盘切换专辑，如果这个专辑没有被使用，将清理相关资源
  Input:           clock_id
                   clock_pos
                   parent_id
                   cur_album_id
                   new_album_id
                    
  Return:           none
**********************************************************************/

void divoom_userimagelist_manage_check_album_id(int clock_id, int clock_pos, int parent_id, int cur_album_id)
{
    Pdivoom_userimagelist_manage_item      palbum_item;
    if(cur_album_id == 0 )
    {
        divoom_print("pass the change!\n");
        return;
    }
    divoom_system_get_lock(pdivoom_userimagelist_manage_handle->lock_handle);
    
    if(divoom_database_manage_check_divoom_image_type_exist(DIVOOM_USERIMAGELIST_CONFIGTABLE_NAME, cur_album_id) == 0)
    {
        divoom_device_playlist_image_list_item playlist_info ={0};
        divoom_database_manage_add_divoom_image_config_item(DIVOOM_USERIMAGELIST_CONFIGTABLE_NAME, clock_id, clock_pos, parent_id, cur_album_id);

        playlist_info.file_size = 256;
        playlist_info.playlist_id = cur_album_id;
        playlist_info.update_time = 0;
        divoom_net_app_send_packet(DIVOOM_NET_COMM_PLAY_LIST_GET_IMAGE_LIST, DIVOOM_SERVER_TOPIC, &playlist_info); 
    }
    divoom_system_free_lock(pdivoom_userimagelist_manage_handle->lock_handle);
}

/*********************************************************************
  Function:        divoom_userimagelist_manage_reset
  Description:     复位模块，全部删除相关数据
  Input:           none
                    
  Return:           none
**********************************************************************/

void divoom_userimagelist_manage_reset(void)
{
    if(pdivoom_userimagelist_manage_handle == NULL)
    {
        return;
    }
    divoom_system_get_lock(
        pdivoom_userimagelist_manage_handle->lock_handle);
    divoom_manage_image_remove(DIVOOM_MANAGE_IMAGE_USER_IMAGE_LIST_PIC, 1);
    divoom_database_manage_clear_all_divoom_image(DIVOOM_USERIMAGELIST_TABLE_NAME);
    divoom_database_manage_clear_all_divoom_image(DIVOOM_USERIMAGELIST_CONFIGTABLE_NAME);
    divoom_queue_pop_all(pdivoom_userimagelist_manage_handle->album_queue,
        divoom_userimagelist_manage_free_album_item);
    divoom_system_free_lock(
        pdivoom_userimagelist_manage_handle->lock_handle);
}


/*********************************************************************
  Function:        divoom_userimagelist_manage_init
  Description:     初始化模块，将相关的文件从spi中load出来
  Input:           file_path  SD路径 
                    
  Return:           none
**********************************************************************/

void divoom_userimagelist_manage_init(void)
{
    if(pdivoom_userimagelist_manage_handle)
    {
        return;
    }

    pdivoom_userimagelist_manage_handle = divoom_system_malloc(sizeof(divoom_userimagelist_manage_ctrl));
    if(pdivoom_userimagelist_manage_handle == NULL)
    {
        return;
    }
    memset(pdivoom_userimagelist_manage_handle, 0, sizeof(divoom_userimagelist_manage_ctrl));
    pdivoom_userimagelist_manage_handle->album_queue = divoom_queue_init(1000, __func__, __LINE__);
    
	char *table_item = "id INTEGER PRIMARY KEY  AUTOINCREMENT," \
		"image_type INTEGER DEFAULT 0," \
		"image_id INTEGER DEFAULT 0," \
		"author_id INTEGER DEFAULT 0," \
		"image_name CHAR(200)," \
		"update_time INTEGER DEFAULT 0," \
		"file_addr CHAR(64)" ;

	divoom_database_manage_add_new_table(DIVOOM_USERIMAGELIST_TABLE_NAME, table_item);
    
    divoom_database_manage_add_table_index(DIVOOM_USERIMAGELIST_TABLE_NAME, "image_type");
    divoom_database_manage_add_table_index(DIVOOM_USERIMAGELIST_TABLE_NAME, "image_id");
    divoom_database_manage_add_table_index(DIVOOM_USERIMAGELIST_TABLE_NAME, "update_time");
    
	char *table_config_item = "id INTEGER PRIMARY KEY     AUTOINCREMENT," \
		"clock_id INTEGER DEFAULT 0," \
		"clock_pos INTEGER DEFAULT 0," \
		"parent_id INTEGER DEFAULT 0," \
		"image_type INTEGER DEFAULT 0," \
		"update_time INTEGER DEFAULT 0";

	divoom_database_manage_add_new_table(DIVOOM_USERIMAGELIST_CONFIGTABLE_NAME, table_config_item);
    divoom_database_manage_add_table_index(DIVOOM_USERIMAGELIST_CONFIGTABLE_NAME, "clock_id");
    divoom_database_manage_add_table_index(DIVOOM_USERIMAGELIST_CONFIGTABLE_NAME, "clock_pos");
    divoom_database_manage_add_table_index(DIVOOM_USERIMAGELIST_CONFIGTABLE_NAME, "parent_id");
    divoom_database_manage_add_table_index(DIVOOM_USERIMAGELIST_CONFIGTABLE_NAME, "image_type");
    divoom_database_manage_add_table_index(DIVOOM_USERIMAGELIST_CONFIGTABLE_NAME, "update_time");
    pdivoom_userimagelist_manage_handle->tmp_album_id = divoom_database_manage_get_divoom_image_type_id(DIVOOM_USERIMAGELIST_CONFIGTABLE_NAME, 0, 0, 0);
    pdivoom_userimagelist_manage_handle->lock_handle = divoom_system_create_lock();
}


void divoom_userimagelist_manage_get_play_image_id(void *user_data, T_U8 id, Pdivoom_play_gif_image_info image_info)
{
    int index;
    Pdivoom_userimagelist_manage_item      palbum_item;

    if (image_info == NULL)
    {
        return;
    }
    if(pdivoom_userimagelist_manage_handle)
    {
        divoom_system_get_lock(pdivoom_userimagelist_manage_handle->lock_handle);
        int album_id = pdivoom_userimagelist_manage_handle->tmp_album_id;
        
        if(pdivoom_userimagelist_manage_handle->change_flag)
        {
            pdivoom_userimagelist_manage_handle->change_flag = 0;
            image_info->image_id = -1;
            divoom_system_free_lock(pdivoom_userimagelist_manage_handle->lock_handle);
            return ;
        }
        
        
        palbum_item = divoom_queue_get_cmp(pdivoom_userimagelist_manage_handle->album_queue, &album_id, divoom_userimagelist_manage_find_file_compare_album_id);
        if(palbum_item == NULL || palbum_item->pic_num == 0)
        {
            divoom_system_free_lock(pdivoom_userimagelist_manage_handle->lock_handle);
            return ;
        }
        #if 1
        if(palbum_item->next_request_time < time(0))
        {
            divoom_device_playlist_image_list_item playlist_info ={0};
            //六个小时重新请求一次数据
            palbum_item->next_request_time = time(0) + DIVOOM_USERIMAGELIST_MANAGE_LOAD_FILE_TIME;
            
            playlist_info.file_size = 256;
            playlist_info.playlist_id = album_id;
            playlist_info.update_time = palbum_item->update_time;
            divoom_net_app_send_packet(DIVOOM_NET_COMM_PLAY_LIST_GET_IMAGE_LIST, DIVOOM_SERVER_TOPIC, &playlist_info); 
            
        }
        #endif
        index = divoom_random_get(palbum_item->prandom_handle);
        if(index < 0 || index >= palbum_item->pic_num)
        {
            divoom_system_free_lock(pdivoom_userimagelist_manage_handle->lock_handle);
            return;
        }
        image_info->model_id = DIVOOM_MANAGE_IMAGE_USER_IMAGE_LIST_PIC;
        image_info->user_id = palbum_item->pic_list[index].user_id;
        strcpy(image_info->image_name, palbum_item->pic_list[index].image_name);
        image_info->image_id = palbum_item->pic_list[index].image_id;
        divoom_system_free_lock(pdivoom_userimagelist_manage_handle->lock_handle);
    }
    else
    {
        image_info->model_id = DIVOOM_MANAGE_IMAGE_LOCAL_PIC_MODE;
        image_info->image_id = DIVOOM_DISP_USER_DEFINE_TV_PIC;
    }
    
    
}

int divoom_userimagelist_manage_get_play_change_flag(void *user_data)
{
    int ret = 0;
    Pdivoom_userimagelist_manage_item      palbum_item;
    

    if(pdivoom_userimagelist_manage_handle)
    {
        divoom_system_get_lock(pdivoom_userimagelist_manage_handle->lock_handle);

        ret = pdivoom_userimagelist_manage_handle->change_flag;
        divoom_system_free_lock(pdivoom_userimagelist_manage_handle->lock_handle);
    }
    return ret;
}


/*********************************************************************
  Function:        divoom_userimagelist_manage_get_random_pic_id
  Description:     得到某类型图片的ＩＤ
  Input:           
  Return:          返回图片ＩＤ
**********************************************************************/

int divoom_userimagelist_manage_get_random_pic_id(int album_id, int *user_id, char *image_name)
{
    int index;
    int image_id;
    Pdivoom_userimagelist_manage_item      palbum_item;
    
    if(pdivoom_userimagelist_manage_handle)
    {
        divoom_system_get_lock(pdivoom_userimagelist_manage_handle->lock_handle);
        if(pdivoom_userimagelist_manage_handle->change_flag)
        {
            pdivoom_userimagelist_manage_handle->change_flag = 0;
            divoom_system_free_lock(pdivoom_userimagelist_manage_handle->lock_handle);
            return -1;
        }
        palbum_item = divoom_queue_get_cmp(pdivoom_userimagelist_manage_handle->album_queue, &album_id, divoom_userimagelist_manage_find_file_compare_album_id);
        if(palbum_item == NULL)
        {
            //如果没有打开，就再打开一次
            palbum_item = divoom_userimagelist_manage_open_album_id_inner(album_id, 1);
        }
        if(palbum_item == NULL || palbum_item->pic_num == 0)
        {
            divoom_system_free_lock(pdivoom_userimagelist_manage_handle->lock_handle);
            return 0;
        }
        #if 1
        if(palbum_item->next_request_time < time(0))
        {
            divoom_device_playlist_image_list_item playlist_info ={0};
            //六个小时重新请求一次数据
            palbum_item->next_request_time = time(0) + DIVOOM_USERIMAGELIST_MANAGE_LOAD_FILE_TIME;
            
            playlist_info.file_size = 256;
            playlist_info.playlist_id = album_id;
            playlist_info.update_time = palbum_item->update_time;
            divoom_net_app_send_packet(DIVOOM_NET_COMM_PLAY_LIST_GET_IMAGE_LIST, DIVOOM_SERVER_TOPIC, &playlist_info); 
            
        }
        #endif
        index = divoom_random_get(palbum_item->prandom_handle);
        if(index < 0 || index >= palbum_item->pic_num)
        {
            divoom_system_free_lock(pdivoom_userimagelist_manage_handle->lock_handle);
            return 0;
        }
        *user_id = palbum_item->pic_list[index].user_id;
        strcpy(image_name, palbum_item->pic_list[index].image_name);
        image_id = palbum_item->pic_list[index].image_id;
        divoom_system_free_lock(pdivoom_userimagelist_manage_handle->lock_handle);
        return image_id;
    }
    
    return 0;
}

/*********************************************************************
  Function:        divoom_userimagelist_manage_check_pic_id_in_album
  Description:     确认资源是否在队列中
  Input:           
  Return:          返回图片ＩＤ
**********************************************************************/

int divoom_userimagelist_manage_check_pic_id_in_album(Pdivoom_userimagelist_manage_item palbum_item, int image_id)
{
    return divoom_userimagelist_manage_find_image_index(
        palbum_item, (uint32)image_id) >= 0;
}


/*********************************************************************
  Function:        divoom_userimagelist_manage_update_file_list
  Description:     更新图片列表
  Input:           
                    
  Return:          none
**********************************************************************/

void divoom_userimagelist_manage_update_file_list(Pdivoom_devcice_image_list pimage_list)
{
    int32 i;
    int32 j;
    int32 user_num;
    int32 download_num = 0;
    int show_transfer;
    int request_next_page;
    divoom_manage_image_item_info pcur_item = {0};
    Pdivoom_manage_image_item_info pdownload_list = NULL;
    Pdivoom_manage_image_item_info pdownload_item;
    Pdivoom_userimagelist_manage_item      palbum_item;

    if(pdivoom_userimagelist_manage_handle == NULL || pimage_list == NULL ||
        (pimage_list->num > 0 && pimage_list->pfile_list == NULL))
    {
        return;
    }
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
    request_next_page = pimage_list->num + pimage_list->unlike_num > 0;
    divoom_system_get_lock(pdivoom_userimagelist_manage_handle->lock_handle);
    palbum_item = divoom_userimagelist_manage_open_album_id_inner(pimage_list->album_id, 0);
    if(palbum_item == NULL)
    {
        divoom_system_free_lock(pdivoom_userimagelist_manage_handle->lock_handle);
        if(pdownload_list != NULL)
        {
            divoom_system_free(pdownload_list);
        }
        return;
    }
    if(show_transfer)
    {
        divoom_manage_image_transfer_batch_begin();
    }
    divoom_database_manage_txn_begin();
    for(i = 0; i < pimage_list->unlike_num; i++)
    {
        divoom_database_manage_delete_subscribe_image_item(DIVOOM_USERIMAGELIST_TABLE_NAME, pimage_list->album_id,  pimage_list->unlike_list[i]);   
        j = divoom_userimagelist_manage_find_image_index(
            palbum_item, pimage_list->unlike_list[i]);
        if(j >= 0)
        {
            divoom_random_remove(palbum_item->prandom_handle, j);
            if(palbum_item->pic_list[j].image_name != NULL)
            {
                divoom_system_free(palbum_item->pic_list[j].image_name);
            }
            if(palbum_item->pic_num != j + 1)
            {
                memmove(&palbum_item->pic_list[j],
                    &palbum_item->pic_list[j + 1],
                    (uint32)(palbum_item->pic_num - (j + 1)) *
                        sizeof(divoom_image_hot_item));
            }
            palbum_item->pic_num --;
        }
    }
    for(i = 0; i < pimage_list->num; i ++)
    {
        if(divoom_userimagelist_manage_check_pic_id_in_album(palbum_item, pimage_list->pfile_list[i].image_id))
        {
            continue;
        }
        if(palbum_item->pic_num >=
            DIVOOM_USERIMAGELIST_MANAGE_MAX_FILE_NUM)
        {
            divoom_manage_image_del(
                DIVOOM_MANAGE_IMAGE_USER_IMAGE_LIST_PIC,
                &palbum_item->pic_list[0].image_id, 1, 1);
            divoom_database_manage_delete_subscribe_image_item(
                DIVOOM_USERIMAGELIST_TABLE_NAME, pimage_list->album_id,
                (int)palbum_item->pic_list[0].image_id);
            divoom_random_remove(palbum_item->prandom_handle, 0);
            divoom_subscribe_local_pic_remove_first_image(
                palbum_item->pic_list, &palbum_item->pic_num, 1);
        }
        if(divoom_userimagelist_manage_append_image_sorted(
            palbum_item,
            (uint32)pimage_list->pfile_list[i].image_id,
            (uint32)pimage_list->pfile_list[i].user_id) == 0)
        {
            divoom_print("user image list append failed image_id=%d\n",
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
        pdownload_item->utc_time = pimage_list->pfile_list[i].utc_time;
        strcpy(pdownload_item->file_addr,pimage_list->pfile_list[i].file_id);
        pdownload_item->pcallback = NULL;
        if(pdownload_list == NULL)
        {
            if(show_transfer)
            {
                divoom_manage_image_add_transfer(
                    DIVOOM_MANAGE_IMAGE_USER_IMAGE_LIST_PIC,
                    pdownload_item, 1);
            }
            else
            {
                divoom_manage_image_add(
                    DIVOOM_MANAGE_IMAGE_USER_IMAGE_LIST_PIC,
                    pdownload_item, 1);
            }
        }
        
        divoom_database_manage_add_subscribe_image_item(
            DIVOOM_USERIMAGELIST_TABLE_NAME,
            pimage_list->album_id,
            pimage_list->pfile_list[i].image_id,
            pimage_list->pfile_list[i].utc_time,
            pimage_list->pfile_list[i].file_id,
            pimage_list->pfile_list[i].user_id,
            "");
        
    }
    if(request_next_page)
    {
        divoom_database_manage_update_divoom_image_config_update_time(DIVOOM_USERIMAGELIST_CONFIGTABLE_NAME, pimage_list->album_id, pimage_list->last_update_time);
        palbum_item->update_time = pimage_list->last_update_time;
    }
    divoom_database_manage_txn_end();
    divoom_userimagelist_manage_close_album_id_inner(pimage_list->album_id);
    divoom_system_free_lock(pdivoom_userimagelist_manage_handle->lock_handle);
    if(pdownload_list != NULL && download_num > 0)
    {
        if(show_transfer)
        {
            divoom_manage_image_add_transfer(
                DIVOOM_MANAGE_IMAGE_USER_IMAGE_LIST_PIC,
                pdownload_list, (uint32)download_num);
        }
        else
        {
            divoom_manage_image_add(
                DIVOOM_MANAGE_IMAGE_USER_IMAGE_LIST_PIC,
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
    if(request_next_page)
    {
        divoom_device_playlist_image_list_item playlist_info ={0};

        playlist_info.file_size = 256;
        playlist_info.playlist_id = pimage_list->album_id;
        playlist_info.update_time = pimage_list->last_update_time;
        divoom_net_app_send_packet(DIVOOM_NET_COMM_PLAY_LIST_GET_IMAGE_LIST,
            DIVOOM_SERVER_TOPIC, &playlist_info);
    }
}


/*********************************************************************
  Function:        divoom_userimagelist_manage_get_url_addr
  Description:     获取指定文件的网络地址
  Input:           model_id
                   image_id
                   url_addr
                    
  Return:          image_flag
**********************************************************************/

int divoom_userimagelist_manage_get_url_addr(int model_id, uint32 image_id, char *url_addr)
{
    return divoom_database_manage_get_subscribe_image_pic_addr(DIVOOM_USERIMAGELIST_TABLE_NAME, image_id, url_addr);
}


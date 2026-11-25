#include "divoom.h"
#include "divoom_subscribe_local_pic.h"

#define DIVOOM_USER_DEFINE_VOICE_ID_LEN 64

typedef struct _divoom_user_define_ctrl_item
{
    int update_flag;
    int use_time;
    int clock_id;
    int clock_pos;
    int read_pos;
    int parent_id;
    int update_time;    //更新时间
    int play_time;      //播放时间
    int show_time;      //是否在作品播放时显示时间
    int cache_num;
    int image_num;
    Pdivoom_image_hot_item image_list;
    /* voice_id 不属于公共四字段图片元数据，按相同下标单独动态保存。 */
    char *voice_list;
}divoom_user_define_ctrl_item, *Pdivoom_user_define_ctrl_item;


typedef struct _divoom_user_define_ctrl_ctrl
{
    int update_time;    //更新时间
    int show_time;      //播放总开关
    void *image_queue;
}divoom_user_define_ctrl_ctrl, *Pdivoom_user_define_ctrl_ctrl;


Pdivoom_user_define_ctrl_ctrl pdivoom_user_define_ctrl_handle = NULL;

/* 图片数组与语音数组必须同时扩容成功，避免两个并行下标失配。 */
static int32 divoom_user_define_ctrl_reserve_image_list(
    Pdivoom_user_define_ctrl_item pcur_item, int32 required_num)
{
    int32 max_capacity;
    int32 new_capacity;
    char *new_voice_list;
    Pdivoom_image_hot_item new_image_list;

    if(pcur_item == NULL || required_num <= 0)
    {
        return 0;
    }
    if(required_num <= pcur_item->cache_num)
    {
        return 1;
    }
    max_capacity = (int32)(
        U32_MAX / (uint32)sizeof(divoom_image_hot_item));
    if((uint32)max_capacity > U32_MAX / DIVOOM_USER_DEFINE_VOICE_ID_LEN)
    {
        max_capacity = (int32)(
            U32_MAX / DIVOOM_USER_DEFINE_VOICE_ID_LEN);
    }
    if(required_num > max_capacity)
    {
        return 0;
    }
    new_capacity = pcur_item->cache_num;
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
    new_image_list = divoom_system_malloc(
        (uint32)new_capacity * sizeof(divoom_image_hot_item));
    new_voice_list = divoom_system_malloc(
        (uint32)new_capacity * DIVOOM_USER_DEFINE_VOICE_ID_LEN);
    if(new_image_list == NULL || new_voice_list == NULL)
    {
        if(new_image_list != NULL)
        {
            divoom_system_free(new_image_list);
        }
        if(new_voice_list != NULL)
        {
            divoom_system_free(new_voice_list);
        }
        return 0;
    }
    if(pcur_item->image_num > 0)
    {
        memcpy(new_image_list, pcur_item->image_list,
            (uint32)pcur_item->image_num * sizeof(divoom_image_hot_item));
        memcpy(new_voice_list, pcur_item->voice_list,
            (uint32)pcur_item->image_num * DIVOOM_USER_DEFINE_VOICE_ID_LEN);
    }
    if(pcur_item->image_list != NULL)
    {
        divoom_system_free(pcur_item->image_list);
    }
    if(pcur_item->voice_list != NULL)
    {
        divoom_system_free(pcur_item->voice_list);
    }
    pcur_item->image_list = new_image_list;
    pcur_item->voice_list = new_voice_list;
    pcur_item->cache_num = new_capacity;
    return 1;
}

static int32 divoom_user_define_ctrl_add_image_id(
    Pdivoom_user_define_ctrl_item pcur_item, int id, int user_id,
    const char *voice_id)
{
    char *name_copy;

    if(pcur_item == NULL ||
        divoom_user_define_ctrl_reserve_image_list(
            pcur_item, pcur_item->image_num + 1) == 0)
    {
        return 0;
    }
    name_copy = divoom_subscribe_local_pic_copy_image_name("");
    if(name_copy == NULL)
    {
        return 0;
    }
    /* 非 HOT 模块不使用 image_size，公共结构中的该字段固定为 0。 */
    pcur_item->image_list[pcur_item->image_num].image_size = 0;
    pcur_item->image_list[pcur_item->image_num].image_id = (uint32)id;
    pcur_item->image_list[pcur_item->image_num].user_id = (uint32)user_id;
    pcur_item->image_list[pcur_item->image_num].image_name = name_copy;
    memset(&pcur_item->voice_list[
        pcur_item->image_num * DIVOOM_USER_DEFINE_VOICE_ID_LEN],
        0, DIVOOM_USER_DEFINE_VOICE_ID_LEN);
    if(voice_id != NULL)
    {
        strncpy(&pcur_item->voice_list[
            pcur_item->image_num * DIVOOM_USER_DEFINE_VOICE_ID_LEN],
            voice_id, DIVOOM_USER_DEFINE_VOICE_ID_LEN - 1);
    }
    pcur_item->image_num++;
    return 1;
}

static void divoom_user_define_ctrl_update_cached_item(
    Pdivoom_user_define_ctrl_item pcur_item, int id, int user_id,
    const char *voice_id)
{
    int32 i;

    if(pcur_item == NULL)
    {
        return;
    }
    for(i = 0; i < pcur_item->image_num; i++)
    {
        if(pcur_item->image_list[i].image_id == (uint32)id)
        {
            pcur_item->image_list[i].user_id = (uint32)user_id;
            divoom_subscribe_local_pic_replace_image_name(
                &pcur_item->image_list[i], "");
            memset(&pcur_item->voice_list[
                i * DIVOOM_USER_DEFINE_VOICE_ID_LEN],
                0, DIVOOM_USER_DEFINE_VOICE_ID_LEN);
            if(voice_id != NULL)
            {
                strncpy(&pcur_item->voice_list[
                    i * DIVOOM_USER_DEFINE_VOICE_ID_LEN],
                    voice_id, DIVOOM_USER_DEFINE_VOICE_ID_LEN - 1);
            }
            return;
        }
    }
}

/*********************************************************************
  Function:        divoom_user_define_ctrl_init
  Description:     模块初始化
  Input:           none
                    
  Return:           none
**********************************************************************/

void divoom_user_define_ctrl_init(uint8 *mac_addr)
{
    if(pdivoom_user_define_ctrl_handle)
    {
        return;
    }
    pdivoom_user_define_ctrl_handle = divoom_system_malloc(sizeof(divoom_user_define_ctrl_ctrl));
    if(pdivoom_user_define_ctrl_handle == NULL)
    {
        return;
    }
    pdivoom_user_define_ctrl_handle->image_queue = divoom_queue_init(1000, __func__, __LINE__);

	char *table_item = "id INTEGER PRIMARY KEY     NOT NULL," \
		"parent_id INTEGER DEFAULT 0," \
		"pos INTEGER DEFAULT 0," \
		"clock_id INTEGER DEFAULT 0," \
		"author_id INTEGER DEFAULT 0," \
		"image_name CHAR(200)," \
		"voice_addr CHAR(64)," \
		"file_addr CHAR(64)" ;

	divoom_database_manage_add_new_table(DIVOOM_USER_DEFINE_TABLE_NAME, table_item);
    
    
    divoom_database_manage_check_column_in_table(DIVOOM_USER_DEFINE_TABLE_NAME, "voice_addr", "CHAR(64)");
    divoom_database_manage_add_table_index(DIVOOM_USER_DEFINE_TABLE_NAME, "parent_id");
    divoom_database_manage_add_table_index(DIVOOM_USER_DEFINE_TABLE_NAME, "pos");
    divoom_database_manage_add_table_index(DIVOOM_USER_DEFINE_TABLE_NAME, "clock_id");
    
	char *config_table_item = "id INTEGER PRIMARY KEY  AUTOINCREMENT," \
		"parent_id INTEGER KEY DEFAULT 0," \
		"pos INTEGER KEY DEFAULT 0," \
		"clock_id INTEGER KEY DEFAULT 0," \
		"play_time INTEGER DEFAULT 0," \
		"show_time INTEGER DEFAULT 0," \
		"update_time INTEGER KEY DEFAULT 0";

	divoom_database_manage_add_new_table(DIVOOM_USER_DEFINE_CONFIG_TABLE_NAME, config_table_item);
    
    /* 旧数据库没有 show_time 列，启动时补列以兼容存量数据库。 */
    divoom_database_manage_check_column_in_table(DIVOOM_USER_DEFINE_CONFIG_TABLE_NAME, "show_time", "INTEGER DEFAULT 0");
    divoom_database_manage_add_table_index(DIVOOM_USER_DEFINE_CONFIG_TABLE_NAME, "parent_id");
    divoom_database_manage_add_table_index(DIVOOM_USER_DEFINE_CONFIG_TABLE_NAME, "pos");
    divoom_database_manage_add_table_index(DIVOOM_USER_DEFINE_CONFIG_TABLE_NAME, "clock_id");
    divoom_database_manage_add_table_index(DIVOOM_USER_DEFINE_CONFIG_TABLE_NAME, "update_time");

    pdivoom_user_define_ctrl_handle->update_time =  divoom_database_manage_get_user_define_last_update_time();
    pdivoom_user_define_ctrl_handle->show_time = 0;
}

int divoom_user_define_ctrl_compare_item(void *item1, void *item2)
{
    Pdivoom_user_define_ctrl_item pcur_item = (Pdivoom_user_define_ctrl_item)item2;
    Pdivoom_clock_id_info clock_info = (Pdivoom_clock_id_info)item1;

    if(pcur_item->clock_id == clock_info->clock_id &&
        pcur_item->clock_pos == clock_info->clock_pos &&
        pcur_item->parent_id == clock_info->parent_id)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

/*********************************************************************
  Function:        divoom_user_define_ctrl_init
  Description:     模块初始化
  Input:           none
                    
  Return:           none
**********************************************************************/

void *divoom_user_define_ctrl_open(Pdivoom_clock_id_info clock_info)
{
    Pdivoom_user_define_ctrl_item pcur_item;

    if(pdivoom_user_define_ctrl_handle == NULL || clock_info == NULL)
    {
        return NULL;
    }
    pcur_item = divoom_queue_get_cmp(pdivoom_user_define_ctrl_handle->image_queue, clock_info, divoom_user_define_ctrl_compare_item);
    if(pcur_item)
    {        
        pcur_item->use_time ++;
        return pcur_item;
    }
    
    pcur_item = divoom_system_malloc(sizeof(divoom_user_define_ctrl_item));
    if(pcur_item == NULL)
    {
        return NULL;
    }
    pcur_item->clock_id = clock_info->clock_id;
    pcur_item->clock_pos = clock_info->clock_pos;
    pcur_item->parent_id = clock_info->parent_id;
    pcur_item->image_list = divoom_database_manage_get_user_define_pic_list(
        clock_info->clock_id, clock_info->clock_pos, clock_info->parent_id,
        &pcur_item->image_num, &pcur_item->voice_list);
    pcur_item->cache_num = pcur_item->image_num;
    pcur_item->use_time  = 1;
    pcur_item->play_time = divoom_database_manage_get_user_define_play_time(clock_info->clock_id, clock_info->clock_pos, clock_info->parent_id, &pcur_item->show_time);
    divoom_queue_push(pdivoom_user_define_ctrl_handle->image_queue, pcur_item);
    divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_GET_CUSTOM_PLAY_TIME, DIVOOM_SERVER_TOPIC, clock_info);
    pdivoom_user_define_ctrl_handle->show_time = pcur_item->show_time;
    return pcur_item;
}

/*********************************************************************
  Function:        divoom_user_define_ctrl_close
  Description:     模块释放相关资源
  Input:           none
                    
  Return:           none
**********************************************************************/

void divoom_user_define_ctrl_close(Pdivoom_clock_id_info clock_info)
{
    Pdivoom_user_define_ctrl_item pcur_item;

    pcur_item = divoom_queue_get_cmp(pdivoom_user_define_ctrl_handle->image_queue, clock_info, divoom_user_define_ctrl_compare_item);
    if(pcur_item)
    {
        
        pcur_item->use_time --;
        if(pcur_item->use_time == 0)
        {
            
            pcur_item = divoom_queue_pop_cmp(pdivoom_user_define_ctrl_handle->image_queue, clock_info, divoom_user_define_ctrl_compare_item);
            divoom_subscribe_local_pic_free_image_list(
                pcur_item->image_list, pcur_item->image_num);
            if(pcur_item->voice_list != NULL)
            {
                divoom_system_free(pcur_item->voice_list);
            }
            divoom_system_free(pcur_item);
            pdivoom_user_define_ctrl_handle->show_time = 0;
        }
    }
}

/*********************************************************************
  Function:        divoom_user_define_ctrl_check_id
  Description:     检查这个ID是否存在
  Input:           pcur_item   
                   id
                    
  Return:           none
**********************************************************************/

int divoom_user_define_ctrl_check_id(Pdivoom_user_define_ctrl_item pcur_item, int id)
{
    if(pcur_item == NULL)
    {
        return divoom_database_manage_get_user_define_pic_addr(id, NULL);
    }
    else
    {
        for(int i = 0; i < pcur_item->image_num; i++)
        {
            if(pcur_item->image_list[i].image_id == id)
            {
                return 1;
            }
        }
        return 0;
    }
}

/*********************************************************************
  Function:        divoom_user_define_ctrl_add_image_id
  Description:     向缓冲中增加一个ID
  Input:           pcur_item   
                   id
                    
  Return:           none
**********************************************************************/

/*********************************************************************
  Function:        divoom_user_define_ctrl_del_image_id
  Description:     向缓冲中删除一个ID
  Input:           pcur_item   
                   id
                    
  Return:           none
**********************************************************************/

void divoom_user_define_ctrl_del_image_id(Pdivoom_user_define_ctrl_item pcur_item, int id)
{
    if(pcur_item)
    {
        for(int i = 0; i < pcur_item->image_num; i++)
        {
            if(pcur_item->image_list[i].image_id == id)
            {
                if(pcur_item->image_list[i].image_name != NULL)
                {
                    divoom_system_free(pcur_item->image_list[i].image_name);
                }
                if(pcur_item->image_num != i+1)
                {
                    memmove(&pcur_item->image_list[i],
                        &pcur_item->image_list[i + 1],
                        (uint32)(pcur_item->image_num - (i + 1)) *
                            sizeof(divoom_image_hot_item));
                    memmove(&pcur_item->voice_list[
                        i * DIVOOM_USER_DEFINE_VOICE_ID_LEN],
                        &pcur_item->voice_list[
                            (i + 1) * DIVOOM_USER_DEFINE_VOICE_ID_LEN],
                        (uint32)(pcur_item->image_num - (i + 1)) *
                            DIVOOM_USER_DEFINE_VOICE_ID_LEN);
                }
                pcur_item->image_num --;
                return;
            }
        }
    }
}




/*********************************************************************
  Function:        divoom_user_define_ctrl_update
  Description:     模块更新
  Input:           plist   网络数据结构
                    
  Return:           none
**********************************************************************/

void divoom_user_define_ctrl_update(Pdivoom_devcice_user_define_respone plist)
{
    uint32 image_id;
    int32 j;
    int32 user_num;
    divoom_manage_image_item_info pimage_item = {0};
    divoom_clock_id_info clock_info;
    Pdivoom_user_define_ctrl_item pcur_item  = NULL;

    if(pdivoom_user_define_ctrl_handle == NULL || plist == NULL)
    {
        return;
    }


    //改变所有存在元素的状态
    for(j = 0; j < plist->file_num; j ++)
    {
        image_id = plist->pfile_list[j].image_id + DIVOOM_MANAGE_IMAGE_ID_BASE_USER_DEFINE;
        if(plist->pfile_list[j].utc_time > pdivoom_user_define_ctrl_handle->update_time)
        {
            pdivoom_user_define_ctrl_handle->update_time = plist->pfile_list[j].utc_time;
        }
        if(pcur_item == NULL || (pcur_item->clock_id != plist->pfile_list[j].clock_id || pcur_item->clock_pos != plist->pfile_list[j].clock_pos || pcur_item->parent_id != plist->pfile_list[j].parent_id))
        {
            clock_info.clock_id = plist->pfile_list[j].clock_id;
            clock_info.clock_pos = plist->pfile_list[j].clock_pos;
            clock_info.parent_id = plist->pfile_list[j].parent_id;
            pcur_item = divoom_queue_get_cmp(pdivoom_user_define_ctrl_handle->image_queue, &clock_info, divoom_user_define_ctrl_compare_item);
        }
        if(plist->pfile_list[j].type == 0)
        {
            //增加或修改
            if(divoom_user_define_ctrl_check_id(pcur_item, plist->pfile_list[j].image_id))
            {
                //修改s
                divoom_manage_image_del_direct(DIVOOM_MANAGE_IMAGE_USER_DEFINE_ONE, &image_id, 1);
                divoom_manage_image_del_direct(DIVOOM_MANAGE_IMAGE_USER_DEFINE_VOICE_FILE, &image_id, 1);
                divoom_database_manage_update_user_define_pic_addr(plist->pfile_list[j].clock_id, plist->pfile_list[j].clock_pos, 
                    plist->pfile_list[j].parent_id, plist->pfile_list[j].image_id,
                    plist->pfile_list[j].file_id,
                    plist->pfile_list[j].utc_time,
                    plist->pfile_list[j].author_id, "",
                    plist->pfile_list[j].voice_id);
                divoom_user_define_ctrl_update_cached_item(
                    pcur_item, plist->pfile_list[j].image_id,
                    plist->pfile_list[j].author_id,
                    plist->pfile_list[j].voice_id);
            }
            else
            {
                //增加
                divoom_database_manage_insert_user_define_pic_item(plist->pfile_list[j].clock_id, plist->pfile_list[j].clock_pos, plist->pfile_list[j].parent_id, 
                plist->pfile_list[j].image_id,
                plist->pfile_list[j].file_id,
                plist->pfile_list[j].utc_time,
                plist->pfile_list[j].author_id, "",
                plist->pfile_list[j].voice_id);
                if(pcur_item)
                {
                    divoom_user_define_ctrl_add_image_id(
                        pcur_item, plist->pfile_list[j].image_id,
                        plist->pfile_list[j].author_id,
                        plist->pfile_list[j].voice_id);
                }
            }
            pimage_item.image_id = image_id;
            pimage_item.utc_time = plist->pfile_list[j].utc_time;
            strcpy(pimage_item.file_addr, plist->pfile_list[j].file_id);
            pimage_item.pcallback = divoom_app_com_finishcallback;
            divoom_manage_image_add(DIVOOM_MANAGE_IMAGE_USER_DEFINE_ONE, &pimage_item, 1);
            if(strlen(plist->pfile_list[j].voice_id))
            {
                pimage_item.image_id = image_id;
                pimage_item.utc_time = plist->pfile_list[j].utc_time;
                strcpy(pimage_item.file_addr, plist->pfile_list[j].voice_id);
                pimage_item.pcallback = divoom_app_com_finishcallback;
                divoom_manage_image_add(DIVOOM_MANAGE_IMAGE_USER_DEFINE_VOICE_FILE, &pimage_item, 1);
            }
            
        }
        else
        {
            //删除
            divoom_manage_image_del_direct(DIVOOM_MANAGE_IMAGE_USER_DEFINE_ONE, &image_id, 1);
            divoom_manage_image_del_direct(DIVOOM_MANAGE_IMAGE_USER_DEFINE_VOICE_FILE, &image_id, 1);
            divoom_user_define_ctrl_del_image_id(
                pcur_item, plist->pfile_list[j].image_id);
            divoom_database_manage_delete_user_define_item(plist->pfile_list[j].clock_id, plist->pfile_list[j].clock_pos, plist->pfile_list[j].parent_id, plist->pfile_list[j].image_id, plist->pfile_list[j].utc_time);
        }
            
    }
    if(plist->file_num)
    {
        divoom_user_define_ctrl_reload();
    }
}


/*********************************************************************
  Function:        divoom_user_define_ctrl_set_item
  Description:     删除某个子项
  Input:           pdel_item   网络数据结构
                    
  Return:           none
**********************************************************************/

int divoom_user_define_ctrl_del_item(Pdivoom_devcice_user_define_item pdel_item)
{    
    int image_id;
    divoom_clock_id_info clock_info;
    Pdivoom_user_define_ctrl_item pcur_item  = NULL;

	if(NULL == pdivoom_user_define_ctrl_handle)
		return 1;

    image_id = pdel_item->image_id + DIVOOM_MANAGE_IMAGE_ID_BASE_USER_DEFINE;
    
    clock_info.clock_id = pdel_item->clock_id;
    clock_info.clock_pos = pdel_item->clock_pos;
    clock_info.parent_id = pdel_item->parent_id;
    pcur_item = divoom_queue_get_cmp(pdivoom_user_define_ctrl_handle->image_queue, &clock_info, divoom_user_define_ctrl_compare_item);
    
    divoom_manage_image_del_direct(DIVOOM_MANAGE_IMAGE_USER_DEFINE_ONE , &image_id, 1);
    divoom_manage_image_del_direct(DIVOOM_MANAGE_IMAGE_USER_DEFINE_VOICE_FILE , &image_id, 1);
    divoom_user_define_ctrl_del_image_id(pcur_item, pdel_item->image_id);
    divoom_database_manage_delete_user_define_item(pdel_item->clock_id, pdel_item->clock_pos, pdel_item->parent_id, pdel_item->image_id, pdel_item->utc_time);
    return 1;
}


/*********************************************************************
  Function:        divoom_user_define_ctrl_reset
  Description:     清空整个自定义
  Input:           pnew_item   网络数据结构
                    
  Return:           none
**********************************************************************/

void divoom_user_define_ctrl_reset(void)
{
    divoom_manage_image_remove(DIVOOM_MANAGE_IMAGE_USER_DEFINE_ONE, 1);
    divoom_manage_image_remove(DIVOOM_MANAGE_IMAGE_USER_DEFINE_VOICE_FILE, 1);
    divoom_database_manage_clear_all_divoom_image(DIVOOM_USER_DEFINE_TABLE_NAME);
}

/*********************************************************************
  Function:        divoom_user_define_ctrl_clean_item
  Description:     删除整个自定义
  Input:           pnew_item   网络数据结构
                    
  Return:           none
**********************************************************************/

void divoom_user_define_ctrl_clean_item(Pdivoom_devcice_user_define_item pclean_item)
{
    int is_open = 1;
    divoom_clock_id_info clock_info;
    Pdivoom_user_define_ctrl_item pcur_item  = NULL;

	if(NULL == pdivoom_user_define_ctrl_handle)
		return ;

    
    clock_info.clock_id = pclean_item->clock_id;
    clock_info.clock_pos = pclean_item->clock_pos;
    clock_info.parent_id = pclean_item->parent_id;
    pcur_item = divoom_queue_get_cmp(pdivoom_user_define_ctrl_handle->image_queue, &clock_info, divoom_user_define_ctrl_compare_item);
    if(pcur_item == NULL)
    {
        pcur_item = divoom_user_define_ctrl_open(&clock_info);
        is_open = 0;
    }
    if(pcur_item)
    {
        uint32 *image_id_list;

        image_id_list = divoom_system_malloc(sizeof(uint32) *pcur_item->image_num);
        for(int i = 0; i < pcur_item->image_num; i++)
        {
            image_id_list[i] = pcur_item->image_list[i].image_id;
        }
        divoom_manage_image_del(DIVOOM_MANAGE_IMAGE_USER_DEFINE_ONE , image_id_list, pcur_item->image_num, 0);
        divoom_manage_image_del(DIVOOM_MANAGE_IMAGE_USER_DEFINE_VOICE_FILE , image_id_list, pcur_item->image_num, 0);
        divoom_system_free(image_id_list);
        if(is_open == 0)
        {
            divoom_user_define_ctrl_close(&clock_info);
        }
        else
        {
            int32 i;

            for(i = 0; i < pcur_item->image_num; i++)
            {
                if(pcur_item->image_list[i].image_name != NULL)
                {
                    divoom_system_free(pcur_item->image_list[i].image_name);
                    pcur_item->image_list[i].image_name = NULL;
                }
            }
            pcur_item->image_num = 0;
            pcur_item->read_pos = 0;
        }
    }
    
    divoom_database_manage_empty_user_define_item(pclean_item->clock_id, pclean_item->clock_pos, pclean_item->parent_id, pclean_item->utc_time);
}

/*********************************************************************
  Function:        divoom_user_define_ctrl_set_item
  Description:     更新某个子项
  Input:           pnew_item   网络数据结构
                    
  Return:           none
**********************************************************************/

void divoom_user_define_ctrl_set_item(Pdivoom_devcice_user_define_item pnew_item)
{
    int image_id;
    divoom_clock_id_info clock_info;
    Pdivoom_user_define_ctrl_item pcur_item  = NULL;
    uint32 user_id;
    divoom_manage_image_item_info pimage_item = {0};

	if(NULL == pdivoom_user_define_ctrl_handle)
		return ;

    image_id = pnew_item->image_id + DIVOOM_MANAGE_IMAGE_ID_BASE_USER_DEFINE;
    
    clock_info.clock_id = pnew_item->clock_id;
    clock_info.clock_pos = pnew_item->clock_pos;
    clock_info.parent_id = pnew_item->parent_id;
    pcur_item = divoom_queue_get_cmp(pdivoom_user_define_ctrl_handle->image_queue, &clock_info, divoom_user_define_ctrl_compare_item);
    
    
    if(divoom_user_define_ctrl_check_id(pcur_item, pnew_item->image_id))
    {
        //修改s
        if(pcur_item)
        {
            pcur_item->update_flag = 1;
        }
        divoom_manage_image_del_direct(DIVOOM_MANAGE_IMAGE_USER_DEFINE_ONE, &image_id, 1);
        divoom_manage_image_del_direct(DIVOOM_MANAGE_IMAGE_USER_DEFINE_VOICE_FILE, &image_id, 1);
        divoom_database_manage_update_user_define_pic_addr(pnew_item->clock_id, pnew_item->clock_pos, 
            pnew_item->parent_id, pnew_item->image_id,
            pnew_item->file_id, pnew_item->utc_time,
            pnew_item->author_id, "", pnew_item->voice_id);
        divoom_user_define_ctrl_update_cached_item(
            pcur_item, pnew_item->image_id,
            pnew_item->author_id, pnew_item->voice_id);
        
    }
    else
    {
        //增加
        divoom_database_manage_insert_user_define_pic_item(pnew_item->clock_id, pnew_item->clock_pos, pnew_item->parent_id, 
        pnew_item->image_id, pnew_item->file_id,
        pnew_item->utc_time, pnew_item->author_id, "", pnew_item->voice_id);
        if(pcur_item)
        {
            divoom_user_define_ctrl_add_image_id(
                pcur_item, pnew_item->image_id,
                pnew_item->author_id, pnew_item->voice_id);
        }
    }
    pimage_item.image_id = image_id;
    pimage_item.utc_time = pnew_item->utc_time;
    strcpy(pimage_item.file_addr, pnew_item->file_id);
    pimage_item.pcallback = divoom_app_com_finishcallback;
    divoom_manage_image_add(DIVOOM_MANAGE_IMAGE_USER_DEFINE_ONE, &pimage_item, 1);
    if(strlen(pnew_item->voice_id))
    {
        strcpy(pimage_item.file_addr, pnew_item->voice_id);
        divoom_manage_image_add(DIVOOM_MANAGE_IMAGE_USER_DEFINE_VOICE_FILE, &pimage_item, 1);
    }
}

/*********************************************************************
  Function:        divoom_user_define_ctrl_check_exist_id
  Description:     检查这个文件是否存在
  Input:           image_id   
                    
  Return:           none
**********************************************************************/

int divoom_user_define_ctrl_check_exist_id(int image_id)
{
    return divoom_database_manage_get_user_define_pic_addr(image_id, NULL);
}

/*********************************************************************
  Function:        divoom_user_define_ctrl_get_url_addr
  Description:     获取指定文件的网络地址
  Input:           model_id
                   image_id
                   url_addr
                    
  Return:          image_flag
**********************************************************************/

int divoom_user_define_ctrl_get_url_addr(int model_id, int image_id, char *url_addr)
{
    return divoom_database_manage_get_user_define_pic_addr(image_id, url_addr);
}


/*********************************************************************
  Function:        divoom_user_define_ctrl_get_next_id
  Description:     得到将要播放的文件ＩＤ
  Input:           type   第几个自定义
                   cur_id 正在播放的，可以为０
                    
  Return:           none
**********************************************************************/

int divoom_user_define_ctrl_get_next_id(Pdivoom_clock_id_info clock_info, int *user_id, char *image_name, int *voice_flag)
{
    int image_id = 0;
    Pdivoom_user_define_ctrl_item pcur_item;

    pcur_item = divoom_queue_get_cmp(pdivoom_user_define_ctrl_handle->image_queue, clock_info, divoom_user_define_ctrl_compare_item);
    if(pcur_item)
    {
        if(pcur_item->image_num == 0 || pcur_item->image_num == 1 && pcur_item->update_flag)
        {
            pcur_item->update_flag = 0;
            return 0;
        }
        pcur_item->update_flag = 0;
        if(pcur_item->read_pos >= pcur_item->image_num)
        {
            pcur_item->read_pos = 0;
        }
        image_id = pcur_item->image_list[pcur_item->read_pos].image_id;
        *user_id = pcur_item->image_list[pcur_item->read_pos].user_id;
        strcpy(image_name,  pcur_item->image_list[pcur_item->read_pos].image_name);
        if(pcur_item->voice_list != NULL &&
            strlen(&pcur_item->voice_list[
                pcur_item->read_pos * DIVOOM_USER_DEFINE_VOICE_ID_LEN]))
        {
            *voice_flag = 1;
        }
        else
        {
            *voice_flag = 0;
        }
        pcur_item->read_pos ++;
    }
    return image_id;
}


/*********************************************************************
  Function:        divoom_user_define_ctrl_update_play_time
  Description:     更新播放的播放时间
  Input:           clock_info   
                   play_time
                    
  Return:           none
**********************************************************************/

void divoom_user_define_ctrl_update_play_time(Pdivoom_clock_id_info clock_info, int play_time, int show_time)
{
    Pdivoom_user_define_ctrl_item pcur_item;

    pcur_item = divoom_queue_get_cmp(pdivoom_user_define_ctrl_handle->image_queue, clock_info, divoom_user_define_ctrl_compare_item);
    if(pcur_item)
    {
        pcur_item->play_time = play_time;
        pcur_item->show_time = show_time;
        pdivoom_user_define_ctrl_handle->show_time = show_time;
    }
    divoom_database_manage_update_user_define_play_time(clock_info->clock_id, clock_info->clock_pos, clock_info->parent_id, play_time, show_time);
    return ;
}


/*********************************************************************
  Function:        divoom_user_define_ctrl_get_play_time
  Description:     得到将要播放的播放时间
  Input:           clock_info   
                    
  Return:           none
**********************************************************************/

int divoom_user_define_ctrl_get_play_time(Pdivoom_clock_id_info clock_info)
{
    int play_time = 0;
    Pdivoom_user_define_ctrl_item pcur_item;

    pcur_item = divoom_queue_get_cmp(pdivoom_user_define_ctrl_handle->image_queue, clock_info, divoom_user_define_ctrl_compare_item);
    if(pcur_item)
    {
        play_time = pcur_item->play_time;
    }
    return play_time;
}

/*********************************************************************
  Function:        divoom_user_define_ctrl_get_voice_flag
  Description:     得到自定义是否可以播放声音
  Input:           
                    
  Return:          true or flase
**********************************************************************/

int divoom_user_define_ctrl_get_voice_flag(void)
{
    return 0;
}


/*********************************************************************
  Function:        divoom_user_define_ctrl_get_show_time_flag
  Description:     得到自定义是否显示时间
  Input:           
                    
  Return:          true or flase
**********************************************************************/

int divoom_user_define_ctrl_get_show_time_flag(void)
{
    return pdivoom_user_define_ctrl_handle->show_time;
}

/*********************************************************************
  Function:        divoom_user_define_ctrl_reload
  Description:     当网络正常了，要重新加载一下自定义
  Input:           type   第几个自定义
                   cur_id 正在播放的，可以为０
                    
  Return:           none
**********************************************************************/

void divoom_user_define_ctrl_reload(void)
{
    divoom_devcice_user_define_request info;

    info.type = 255;
    info.update_time = pdivoom_user_define_ctrl_handle->update_time;
    
    divoom_net_app_send_packet(DIVOOM_NET_COMM_GET_USER_DEFINE_LIST, DIVOOM_SERVER_TOPIC,(void *)&info);
}


#include "divoom.h"

#if DIVOOM_PIXEL_EQ_ENABLE
typedef struct _divoom_eq_image_manage_change_item
{
    int mode;
    divoom_clock_id_info clock_info;
}divoom_eq_image_manage_change_item, *Pdivoom_eq_image_manage_change_item;


typedef struct _divoom_eq_image_manage_item
{
    int image_num;
    int use_time;
    int mode;
    divoom_clock_id_info clock_info;
    Pdivoom_eq_image_manage_image_item pimage_list;
}divoom_eq_image_manage_item, *Pdivoom_eq_image_manage_item;


typedef struct _divoom_eq_image_manage_ctrl
{
    int  update_time;
    void *change_eq_clock_queue;
    void *eq_queue;
}divoom_eq_image_manage_ctrl, *Pdivoom_eq_image_manage_ctrl;


Pdivoom_eq_image_manage_ctrl pdivoom_eq_image_manage_handle = NULL;

typedef struct _divoom_eq_image_manage_download_pending
{
    divoom_clock_id_info clock_info;
    int mode;
    int valid;
}divoom_eq_image_manage_download_pending;

static divoom_eq_image_manage_download_pending s_eq_download_pending;

int divoom_eq_image_manage_compare_change_item(void *item1, void *item2);

static int divoom_eq_image_manage_clock_match(Pdivoom_clock_id_info a, Pdivoom_clock_id_info b)
{
    return a->clock_id == b->clock_id &&
        a->clock_pos == b->clock_pos &&
        a->parent_id == b->parent_id;
}

static void divoom_eq_image_manage_push_eq_change(Pdivoom_clock_id_info pclock_info, int mode)
{
    Pdivoom_eq_image_manage_change_item pchange_item;

    pchange_item = divoom_queue_pop_cmp(pdivoom_eq_image_manage_handle->change_eq_clock_queue, pclock_info, divoom_eq_image_manage_compare_change_item);
    if(pchange_item == NULL)
    {
        if(divoom_queue_is_full(pdivoom_eq_image_manage_handle->change_eq_clock_queue))
        {
            pchange_item = divoom_queue_pop(pdivoom_eq_image_manage_handle->change_eq_clock_queue);
        }
        else
        {
            pchange_item = divoom_system_malloc(sizeof(divoom_eq_image_manage_change_item));
        }
        memcpy(&pchange_item->clock_info, pclock_info, sizeof(divoom_clock_id_info));
        pchange_item->clock_info.data_clock_pos = 0;
        pchange_item->clock_info.data_clock_id = 0;
        pchange_item->clock_info.data_parent_id = 0;
    }
    pchange_item->mode = mode;
    divoom_queue_push(pdivoom_eq_image_manage_handle->change_eq_clock_queue, pchange_item);
}

static void divoom_eq_image_manage_apply_eq_downloaded(Pdivoom_clock_id_info pclock_info, int mode)
{
    Pdivoom_eq_image_manage_item pcur_item;

    divoom_eq_image_manage_push_eq_change(pclock_info, mode);
    pcur_item = divoom_eq_image_manage_open_handle(pclock_info);
    if(pcur_item)
    {
        pcur_item->mode = mode;
        divoom_eq_image_manage_close_handle(pclock_info);
    }
    divoom_database_manage_update_eq_mode_index(DIVOOM_EQ_CONFIG_TABLE_NAME, pclock_info->clock_id, pclock_info->clock_pos, pclock_info->parent_id, mode);
}

void divoom_eq_image_manage_download_finishcallback(int ret)
{
    if(ret <= 0 || s_eq_download_pending.valid == 0)
    {
        return;
    }
    divoom_print("eq download done, switch mode=%d clock=%d\n", s_eq_download_pending.mode, s_eq_download_pending.clock_info.clock_id);
    divoom_eq_image_manage_apply_eq_downloaded(&s_eq_download_pending.clock_info, s_eq_download_pending.mode);
    s_eq_download_pending.valid = 0;
}

int  divoom_eq_image_manage_get_update_time(void)
{
    return pdivoom_eq_image_manage_handle->update_time;
}

void divoom_eq_image_manage_init(void)
{
	char *table_item = "id INTEGER PRIMARY KEY     AUTOINCREMENT," \
		"clock_id INTEGER DEFAULT 0," \
		"clock_pos INTEGER DEFAULT 0," \
		"parent_id INTEGER DEFAULT 0," \
		"update_time INTEGER DEFAULT 0," \
		"author_id INTEGER DEFAULT 0," \
		"image_name CHAR(200)," \
		"file_addr CHAR(64)";

	divoom_database_manage_add_new_table(DIVOOM_EQ_IMAGE_TABLE_NAME, table_item);
    divoom_database_manage_add_table_index(DIVOOM_EQ_IMAGE_TABLE_NAME, "clock_id");
    divoom_database_manage_add_table_index(DIVOOM_EQ_IMAGE_TABLE_NAME, "clock_pos");
    divoom_database_manage_add_table_index(DIVOOM_EQ_IMAGE_TABLE_NAME, "parent_id");
    divoom_database_manage_add_table_index(DIVOOM_EQ_IMAGE_TABLE_NAME, "update_time");
    
	char *config_table_item = "id INTEGER PRIMARY KEY     AUTOINCREMENT," \
		"clock_id INTEGER DEFAULT 0," \
		"clock_pos INTEGER DEFAULT 0," \
		"parent_id INTEGER DEFAULT 0," \
		"mode INTEGER DEFAULT 0";

	divoom_database_manage_add_new_table(DIVOOM_EQ_CONFIG_TABLE_NAME, config_table_item);
    divoom_database_manage_add_table_index(DIVOOM_EQ_CONFIG_TABLE_NAME, "clock_id");
    divoom_database_manage_add_table_index(DIVOOM_EQ_CONFIG_TABLE_NAME, "clock_pos");
    divoom_database_manage_add_table_index(DIVOOM_EQ_CONFIG_TABLE_NAME, "parent_id");
    
    pdivoom_eq_image_manage_handle = divoom_system_malloc(sizeof(divoom_eq_image_manage_ctrl));
    pdivoom_eq_image_manage_handle->eq_queue = divoom_queue_init(100, __func__, __LINE__);
    pdivoom_eq_image_manage_handle->change_eq_clock_queue = divoom_queue_init(10, __func__, __LINE__);
    pdivoom_eq_image_manage_handle->update_time = divoom_database_manage_get_subscribe_config_update_time(DIVOOM_EQ_IMAGE_TABLE_NAME);
}

int divoom_eq_image_manage_compare_clock(void *item1, void *item2)
{
    Pdivoom_clock_id_info pclock_info = (Pdivoom_clock_id_info)item1;
    Pdivoom_eq_image_manage_item pcur_item = (Pdivoom_eq_image_manage_item)item2;

    return !divoom_eq_image_manage_clock_match(pclock_info, &pcur_item->clock_info);
}

int divoom_eq_image_manage_get_image_id(Pdivoom_clock_id_info pclock_info, int  index, int *user_id, char *image_name)
{
    int image_id;
    Pdivoom_eq_image_manage_item pcur_item;

    pcur_item = divoom_eq_image_manage_open_handle(pclock_info);
    if(pcur_item == NULL)
    {
        divoom_print("this is err!\n");
        return  0;
    }
	if (pcur_item->image_num == 0)
	{
    	divoom_eq_image_manage_close_handle(pclock_info);
		return	0;
	}

    if(index >= pcur_item->image_num)
    {
        divoom_print(" index is overload(%d, %d) and this is err!\n", index ,pcur_item->image_num);
        index = 0;
    }
    image_id = pcur_item->pimage_list[index].image_id;
    *user_id = pcur_item->pimage_list[index].user_id;
    strcpy(image_name, pcur_item->pimage_list[index].image_name);
    divoom_eq_image_manage_close_handle(pclock_info);
    return image_id;
}

void divoom_eq_image_manage_delete_image_id(Pdivoom_clock_id_info pclock_info, int  index)
{
    Pdivoom_eq_image_manage_item pcur_item;

    pcur_item = divoom_eq_image_manage_open_handle(pclock_info);
    if(pcur_item == NULL)
    {
        divoom_print("this is err!\n");
        return;
    }
    if(index >= pcur_item->image_num)
    {
        divoom_print(" index is overload(%d, %d) and this is err!\n", index ,pcur_item->image_num);
        index = 0;
        divoom_eq_image_manage_close_handle(pclock_info);
        return;
    }
    divoom_database_manage_delete_eq_image_item(DIVOOM_EQ_IMAGE_TABLE_NAME, pcur_item->pimage_list[index].image_id);
    divoom_manage_image_del(DIVOOM_MANAGE_IMAGE_LIGHT_MUSIC_MODE, &pcur_item->pimage_list[index].image_id, 1, 1);
    pcur_item->image_num --;
    if(pcur_item->image_num != index)
    {
        memmove(&pcur_item->pimage_list[index], &pcur_item->pimage_list[index + 1], (pcur_item->image_num - index) * sizeof(divoom_eq_image_manage_image_item));
    }
    divoom_eq_image_manage_close_handle(pclock_info);
}

void divoom_eq_image_manage_clean_item(Pdivoom_clock_id_info pclock_info)
{
    Pdivoom_eq_image_manage_item pcur_item;

    pcur_item = divoom_eq_image_manage_open_handle(pclock_info);
    if(pcur_item == NULL)
    {
        divoom_print("this is err!\n");
        return;
    }
    for(int i = 0; i < pcur_item->image_num; i++)
    {
        divoom_database_manage_delete_eq_image_item(DIVOOM_EQ_IMAGE_TABLE_NAME, pcur_item->pimage_list[i].image_id);
        divoom_manage_image_del(DIVOOM_MANAGE_IMAGE_LIGHT_MUSIC_MODE, &pcur_item->pimage_list[i].image_id, 1, 1);
    }
    divoom_eq_image_manage_close_handle(pclock_info);
}


void divoom_eq_image_manage_add_image_id(Pdivoom_clock_id_info pclock_info, int update_time,char *file_id, int user_id, char *image_name)
{
    int record_id;
    int new_mode;
    Pdivoom_eq_image_manage_item pcur_item;
    divoom_manage_image_item_info image_item;
    Pdivoom_eq_image_manage_image_item pnew_image_list, pcur_image_list;

    pcur_item = divoom_eq_image_manage_open_handle(pclock_info);
    if(pcur_item == NULL)
    {
        divoom_print("this is err!\n");
        return;
    }
    record_id = divoom_database_manage_add_eq_image_item(DIVOOM_EQ_IMAGE_TABLE_NAME, pclock_info->clock_id, pclock_info->clock_pos, pclock_info->parent_id, update_time, file_id,user_id, image_name);
    pnew_image_list = divoom_system_malloc(sizeof(divoom_eq_image_manage_image_item) *(pcur_item->image_num + 1));
    memcpy(pnew_image_list, pcur_item->pimage_list, sizeof(divoom_eq_image_manage_image_item) *pcur_item->image_num);
    pnew_image_list[pcur_item->image_num].image_id = record_id;
    pnew_image_list[pcur_item->image_num].user_id = user_id;
    strcpy(pnew_image_list[pcur_item->image_num].image_name, image_name);
    strcpy(pnew_image_list[pcur_item->image_num].file_id, file_id);
    divoom_system_free(pcur_item->pimage_list);
    pcur_item->pimage_list = pnew_image_list;
    pcur_item->image_num ++;
    new_mode = pcur_item->image_num + DIVOOM_DISP_MUSIC_END_MODE - 1;
    pcur_item->mode = new_mode;
    divoom_database_manage_update_eq_mode_index(DIVOOM_EQ_CONFIG_TABLE_NAME, pclock_info->clock_id, pclock_info->clock_pos, pclock_info->parent_id, new_mode);
    image_item.image_id = record_id;
    strcpy(image_item.file_addr,file_id);
    image_item.pcallback = divoom_eq_image_manage_download_finishcallback;
    divoom_manage_image_add(DIVOOM_MANAGE_IMAGE_LIGHT_MUSIC_MODE, &image_item, 1);
    memcpy(&s_eq_download_pending.clock_info, pclock_info, sizeof(divoom_clock_id_info));
    s_eq_download_pending.clock_info.data_clock_pos = 0;
    s_eq_download_pending.clock_info.data_clock_id = 0;
    s_eq_download_pending.clock_info.data_parent_id = 0;
    s_eq_download_pending.mode = new_mode;
    s_eq_download_pending.valid = 1;
    divoom_eq_image_manage_close_handle(pclock_info);
    if(divoom_manage_image_check_exist(DIVOOM_MANAGE_IMAGE_LIGHT_MUSIC_MODE, record_id))
    {
        divoom_eq_image_manage_download_finishcallback(1);
    }
}

void divoom_eq_image_manage_update_image_list(void *data)
{
    int i,j,k, image_num, eq_index;
    uint8 small_flag[20], add_flag[12];
    Pdivoom_eq_image_manage_item pcur_item;
    divoom_clock_id_info clock_info;
    divoom_manage_image_item_info image_item;
    Pdivoom_channel_eq_list_info peq_list = (Pdivoom_channel_eq_list_info)data;

    for(i = 0; i < peq_list->num; i++)
    {
        clock_info.clock_id = peq_list->eq_list[i].clock_id;
        clock_info.clock_pos = peq_list->eq_list[i].clock_pos;
        clock_info.parent_id = peq_list->eq_list[i].parent_id;
        pcur_item = divoom_eq_image_manage_open_handle(&clock_info);
        if(pcur_item == NULL)
        {
            divoom_print("this is err!\n");
            continue;
        }
        memset(small_flag , 0, sizeof(small_flag));
        memset(add_flag , 0, sizeof(add_flag));
        for(j = 0; j < peq_list->eq_list[i].eq_num; j++)
        {
            for(k = 0; k < pcur_item->image_num; k ++)
            {
                if(strcmp(peq_list->eq_list[i].file_addr[j], pcur_item->pimage_list[k].file_id) == 0)
                {
                    small_flag[k] = 1;
                    break;
                }
            }
            if(k >= pcur_item->image_num)
            {
                //这个是新增加的
                add_flag[j] = 1;
            }            
        }
        image_num = pcur_item->image_num;
        eq_index = pcur_item->mode;
        pcur_item->mode = peq_list->eq_list[i].eq_index;
        divoom_eq_image_manage_close_handle(&clock_info);
        if(eq_index != peq_list->eq_list[i].eq_index)
        {
            divoom_database_manage_update_eq_mode_index(DIVOOM_EQ_CONFIG_TABLE_NAME, clock_info.clock_id, clock_info.clock_pos, clock_info.parent_id, eq_index);
        }
        for(k = image_num - 1; k >= 0; k --)
        {
            if(small_flag[k] == 0)
            {
                divoom_eq_image_manage_delete_image_id(&clock_info, k);
            }
        }
        
        for(j = 0; j < peq_list->eq_list[i].eq_num; j++)
        {
            if(add_flag[j])
            {
                divoom_eq_image_manage_add_image_id(&clock_info, peq_list->eq_list[i].update_time,peq_list->eq_list[i].file_addr[j],peq_list->eq_list[i].author_id[j],peq_list->eq_list[i].image_name[j]);
            }
        }
        
    }

    
}

int divoom_eq_image_manage_compare_change_item(void *item1, void *item2)
{
    Pdivoom_clock_id_info pclock_info = (Pdivoom_clock_id_info)item1;
    Pdivoom_eq_image_manage_change_item pcur_item = (Pdivoom_eq_image_manage_change_item)item2;

    return !divoom_eq_image_manage_clock_match(pclock_info, &pcur_item->clock_info);
}

void divoom_eq_image_manage_update_eq_mode(Pdivoom_clock_id_info pclock_info, int mode)
{
    Pdivoom_eq_image_manage_item pcur_item;

    pcur_item = divoom_eq_image_manage_open_handle(pclock_info);
    if(pcur_item)
    {
        if(pcur_item->mode != mode || mode == DIVOOM_DISP_MUSIC_TMP_INDEX)
        {
            divoom_eq_image_manage_push_eq_change(pclock_info, mode);
            pcur_item->mode = mode;
        }
        divoom_eq_image_manage_close_handle(pclock_info);
    }
    divoom_database_manage_update_eq_mode_index(DIVOOM_EQ_CONFIG_TABLE_NAME, pclock_info->clock_id, pclock_info->clock_pos, pclock_info->parent_id, mode);
}

int divoom_eq_image_manage_get_change_eq_mode(Pdivoom_clock_id_info pclock_info)
{    
    int eq_mode = -1;
    Pdivoom_eq_image_manage_change_item pchange_item;
    
    pchange_item = divoom_queue_pop_cmp(pdivoom_eq_image_manage_handle->change_eq_clock_queue, pclock_info, divoom_eq_image_manage_compare_change_item);
    if(pchange_item)
    {
        eq_mode = pchange_item->mode;
        divoom_system_free(pchange_item);
    }
    return eq_mode;

}

int divoom_eq_image_manage_get_eq_mode(Pdivoom_clock_id_info pclock_info)
{
    int mode = 0;
    Pdivoom_eq_image_manage_item pcur_item;

    pcur_item = divoom_eq_image_manage_open_handle(pclock_info);
    if(pcur_item)
    {
        mode = pcur_item->mode;
        divoom_eq_image_manage_close_handle(pclock_info);
    }
    return mode;

}

void *divoom_eq_image_manage_open_handle(Pdivoom_clock_id_info pclock_info)
{
    Pdivoom_eq_image_manage_item pcur_item;

    pcur_item = divoom_queue_get_cmp(pdivoom_eq_image_manage_handle->eq_queue, pclock_info, divoom_eq_image_manage_compare_clock);
    if(pcur_item)
    {
        pcur_item->use_time ++;
        return  pcur_item;
    }
    pcur_item = divoom_system_malloc(sizeof(divoom_eq_image_manage_item));    
    pcur_item->pimage_list = divoom_database_manage_get_eq_image_list(DIVOOM_EQ_IMAGE_TABLE_NAME, pclock_info->clock_id, pclock_info->clock_pos, pclock_info->parent_id, &pcur_item->image_num);
    pcur_item->use_time = 1;    
    pcur_item->mode = divoom_database_manage_get_eq_mode_index(DIVOOM_EQ_CONFIG_TABLE_NAME, pclock_info->clock_id, pclock_info->clock_pos, pclock_info->parent_id);        
    memcpy(&pcur_item->clock_info, pclock_info, sizeof(divoom_clock_id_info));
    
    divoom_queue_push(pdivoom_eq_image_manage_handle->eq_queue, pcur_item);
    return pcur_item;
}

void divoom_eq_image_manage_close_handle(Pdivoom_clock_id_info pclock_info)
{
    Pdivoom_eq_image_manage_item pcur_item;

    pcur_item = divoom_queue_get_cmp(pdivoom_eq_image_manage_handle->eq_queue, pclock_info, divoom_eq_image_manage_compare_clock);
    if(pcur_item == NULL)
    {
        return;
    }
    pcur_item->use_time --;
    if(pcur_item->use_time == 0)
    {
        pcur_item = divoom_queue_pop_cmp(pdivoom_eq_image_manage_handle->eq_queue, pclock_info, divoom_eq_image_manage_compare_clock);
        if(pcur_item)
        {
            divoom_system_free(pcur_item->pimage_list);
            divoom_system_free(pcur_item);
        }
        return ;
    }
    
}


/*********************************************************************
  Function:        divoom_eq_image_manage_get_url_addr
  Description:     获取指定文件的网络地址
  Input:           model_id
                   image_id
                   url_addr
                    
  Return:          image_flag
**********************************************************************/

int divoom_eq_image_manage_get_url_addr(int model_id, int image_id, char *url_addr)
{

    divoom_database_manage_get_eq_image_addr(DIVOOM_EQ_IMAGE_TABLE_NAME, image_id, url_addr);

    return 1;
}
#else
void *divoom_eq_image_manage_open_handle(Pdivoom_clock_id_info pclock_info) { (void)pclock_info; return NULL; }
void divoom_eq_image_manage_close_handle(Pdivoom_clock_id_info pclock_info) { (void)pclock_info; }
void divoom_eq_image_manage_update_image_list(void *data) { (void)data; }
void divoom_eq_image_manage_add_image_id(Pdivoom_clock_id_info pclock_info, int update_time, char *file_id, int user_id, char *image_name) { (void)pclock_info; (void)update_time; (void)file_id; (void)user_id; (void)image_name; }
void divoom_eq_image_manage_clean_item(Pdivoom_clock_id_info pclock_info) { (void)pclock_info; }
void divoom_eq_image_manage_delete_image_id(Pdivoom_clock_id_info pclock_info, int index) { (void)pclock_info; (void)index; }
int divoom_eq_image_manage_get_image_id(Pdivoom_clock_id_info pclock_info, int index, int *user_id, char *image_name) { (void)pclock_info; (void)index; (void)user_id; (void)image_name; return 0; }
void divoom_eq_image_manage_init(void) {}
void divoom_eq_image_manage_update_eq_mode(Pdivoom_clock_id_info pclock_info, int mode) { (void)pclock_info; (void)mode; }
int divoom_eq_image_manage_get_eq_mode(Pdivoom_clock_id_info pclock_info) { (void)pclock_info; return 0; }
int divoom_eq_image_manage_get_url_addr(int model_id, int image_id, char *url_addr) { (void)model_id; (void)image_id; if(url_addr) url_addr[0] = 0; return 0; }
int divoom_eq_image_manage_get_change_eq_mode(Pdivoom_clock_id_info pclock_info) { (void)pclock_info; return 0; }
int divoom_eq_image_manage_get_update_time(void) { return 0; }
#endif


#include <sys/stat.h>
#include "divoom.h"


typedef struct _divoom_custom_photo_item
{
    int    use_time;
    int    total_num;
    int    photo_num;
    int    record_offset;
    int    pause_flag;
    int    read_pos;
    int    delete_flag;
    int    order_flag;
    int    last_pic_id;
    int    reverse_flag;
    divoom_clock_id_info clock_info;
    uint32 *photo_list;
    void *prand_handle;
}divoom_custom_photo_item, *Pdivoom_custom_photo_item;

typedef struct _divoom_custom_photo_pos_info
{
    int pause_flag;
    int pic_id;
    divoom_clock_id_info clock_info;
}divoom_custom_photo_pos_info, *Pdivoom_custom_photo_pos_info;

typedef struct _divoom_custom_photo_ctrl
{
    divoom_clock_id_info  read_clock_info;
    int  last_id;
    void *lock_handle;
    void *photo_queue;
    void *photo_pos_queue;
}divoom_custom_photo_ctrl, *Pdivoom_custom_photo_ctrl;

void divoom_custom_photo_delete_all_pic(void *handle);

Pdivoom_custom_photo_ctrl pdivoom_custom_photo_handle = NULL;

void divoom_custom_photo_init(void)
{
	char *table_item = "id INTEGER PRIMARY KEY     NOT NULL," \
		"parent_id INTEGER DEFAULT 0," \
		"pos INTEGER DEFAULT 0," \
		"clock_id INTEGER DEFAULT 0," \
        "save_time BIGINT DEFAULT 0," \
        "photo_time BIGINT DEFAULT 0, " \
		"file_name CHAR(200)";

	divoom_database_manage_add_new_table(DIVOOM_CUSTOM_PHOTO_TABLE_NAME, table_item);
    divoom_database_manage_add_table_index(DIVOOM_CUSTOM_PHOTO_TABLE_NAME, "parent_id");
    divoom_database_manage_add_table_index(DIVOOM_CUSTOM_PHOTO_TABLE_NAME, "pos");
    divoom_database_manage_add_table_index(DIVOOM_CUSTOM_PHOTO_TABLE_NAME, "clock_id");
    divoom_database_manage_add_table_index(DIVOOM_CUSTOM_PHOTO_TABLE_NAME, "save_time");
    divoom_database_manage_add_table_index(DIVOOM_CUSTOM_PHOTO_TABLE_NAME, "photo_time");
    if(divoom_database_manage_check_count_err("SELECT id FROM custom_photo WHERE photo_time = 2147483647 OR photo_time = 0 LIMIT 1"))
    {
        //处理数据库出错
        divoom_print("do err!\n");
        divoom_database_manage_run_sql_command("UPDATE custom_photo SET photo_time = id WHERE photo_time = 2147483647");
        divoom_database_manage_run_sql_command("UPDATE custom_photo SET photo_time = id WHERE photo_time = 0");
    }
    if(divoom_database_manage_check_count_err("SELECT id FROM custom_photo WHERE save_time = 2147483647 OR save_time = 0 LIMIT 1"))
    {
        //处理数据库出错
        divoom_print("do err!\n");
        divoom_database_manage_run_sql_command("UPDATE custom_photo SET save_time = id WHERE save_time = 2147483647");
        divoom_database_manage_run_sql_command("UPDATE custom_photo SET save_time = id WHERE save_time = 0");
    }
    pdivoom_custom_photo_handle = divoom_system_malloc(sizeof(divoom_custom_photo_ctrl));
    pdivoom_custom_photo_handle->photo_queue = divoom_queue_init(100, __func__, __LINE__);
    pdivoom_custom_photo_handle->photo_pos_queue = divoom_queue_init(50, __func__, __LINE__);
    pdivoom_custom_photo_handle->lock_handle = divoom_system_create_lock();
}

int divoom_custom_photo_handle_compare(void *item1, void *item2)
{
    Pdivoom_clock_id_info clock_info = (Pdivoom_clock_id_info)item1;
    Pdivoom_custom_photo_item phandle = (Pdivoom_custom_photo_item)item2;


    return memcmp(clock_info, &phandle->clock_info, sizeof(divoom_clock_id_info));
}

void divoom_custom_photo_clear_all(Pdivoom_clock_id_info clock_info)
{
    Pdivoom_custom_photo_item  phandle;

    divoom_system_get_lock(pdivoom_custom_photo_handle->lock_handle);
    phandle = divoom_queue_pop_cmp(pdivoom_custom_photo_handle->photo_queue, clock_info, divoom_custom_photo_handle_compare);
    if(phandle)
    {
        phandle->delete_flag = 1;
        phandle->photo_num = 0;
        divoom_custom_photo_delete_all_pic(phandle);
        divoom_queue_push(pdivoom_custom_photo_handle->photo_queue, phandle);
    }
    else 
    {
        phandle = divoom_custom_photo_open(clock_info, 0, 0, NULL);
        if(phandle)
        {
            divoom_custom_photo_delete_all_pic(phandle);
            divoom_custom_photo_close(phandle);
        }
    }
    divoom_database_manage_delete_custom_photo_all(clock_info);
    divoom_system_free_lock(pdivoom_custom_photo_handle->lock_handle);
}

int divoom_custom_photo_add_item(Pdivoom_clock_id_info clock_info, double save_time, int photo_time, char *file_name)
{
    int record_id, out_w, out_h;
    Pdivoom_custom_photo_item  phandle;

    divoom_system_get_lock(pdivoom_custom_photo_handle->lock_handle);
    record_id = divoom_database_manage_insert_custom_photo_item(clock_info, save_time, photo_time, file_name);
    phandle = divoom_queue_pop_cmp(pdivoom_custom_photo_handle->photo_queue, clock_info, divoom_custom_photo_handle_compare);
    if(phandle)
    {
        uint32 *photo_list = divoom_system_malloc((phandle->photo_num + 1) *sizeof(uint32));

        if(phandle->photo_num == phandle->total_num)
        {
            photo_list[0] = record_id;
            memcpy(&photo_list[1], phandle->photo_list, phandle->photo_num *sizeof(uint32));
            divoom_system_free(phandle->photo_list);
            phandle->photo_list = photo_list;
            phandle->photo_num ++;
        }
        phandle->total_num ++;
        phandle->last_pic_id = record_id;
        divoom_queue_push(pdivoom_custom_photo_handle->photo_queue, phandle);
    }
    divoom_system_free_lock(pdivoom_custom_photo_handle->lock_handle);
    //divoom_pic_phone_manage_create_picture_thumb(file_name, &out_w, &out_h, NULL);
    return record_id;
}


void divoom_custom_photo_delete_item(Pdivoom_clock_id_info clock_info, int id)
{
    Pdivoom_custom_photo_item  phandle;
    char file_name[256], thumb_path[256];

    divoom_system_get_lock(pdivoom_custom_photo_handle->lock_handle);

    phandle = divoom_queue_pop_cmp(pdivoom_custom_photo_handle->photo_queue, clock_info, divoom_custom_photo_handle_compare);
    if(phandle)
    {
        for(int i = 0; i < phandle->photo_num; i++)
        {
            if(phandle->photo_list[i] == id)
            {
                if(phandle->photo_num != i+1)
                {
                    memmove(&phandle->photo_list[i], &phandle->photo_list[i + 1], (phandle->photo_num - (i + 1))*sizeof(uint32));
                }
                phandle->photo_num --;
                if(phandle->prand_handle)
                {
                    divoom_random_destroy(phandle->prand_handle);
                    phandle->prand_handle = divoom_random_create(phandle->photo_num);
                }
                break;
            }
        }
        phandle->total_num --;
        divoom_queue_push(pdivoom_custom_photo_handle->photo_queue, phandle);
    }
    divoom_custom_photo_get_pic_name_from_id(id, file_name);
    divoom_pic_phone_manage_cache_delete_pic(file_name);   
    divoom_database_manage_delete_custom_photo_item(id);
    divoom_system_free_lock(pdivoom_custom_photo_handle->lock_handle);
}

int divoom_custom_photo_handle_get_pos_compare(void *item1, void *item2)
{
    Pdivoom_clock_id_info clock_info = (Pdivoom_clock_id_info)item1;
    Pdivoom_custom_photo_pos_info phandle = (Pdivoom_custom_photo_pos_info)item2;

    return memcmp(clock_info, &phandle->clock_info, sizeof(divoom_clock_id_info));
}

void divoom_custom_photo_update_pos_info(Pdivoom_clock_id_info clock_info, int pic_id)
{
    Pdivoom_custom_photo_pos_info phandle;
    
    phandle = divoom_queue_pop_cmp(pdivoom_custom_photo_handle->photo_pos_queue, clock_info, divoom_custom_photo_handle_get_pos_compare);
    if(phandle)
    {
        phandle->pic_id = pic_id;
    }
    else
    {
        if(divoom_queue_is_full(pdivoom_custom_photo_handle->photo_pos_queue))
        {
            phandle = divoom_queue_pop(pdivoom_custom_photo_handle->photo_pos_queue);
        }
        else
        {
            phandle = divoom_system_malloc(sizeof(divoom_custom_photo_pos_info));
        }
        memcpy(&phandle->clock_info, clock_info, sizeof(divoom_clock_id_info));
        phandle->pic_id = pic_id;
    }
    divoom_queue_push(pdivoom_custom_photo_handle->photo_pos_queue, phandle);
}

void divoom_custom_photo_set_play_one(Pdivoom_clock_id_info clock_info, int pic_id)
{
    Pdivoom_custom_photo_pos_info phandle;
    
    phandle = divoom_queue_pop_cmp(pdivoom_custom_photo_handle->photo_pos_queue, clock_info, divoom_custom_photo_handle_get_pos_compare);
    if(phandle)
    {
        phandle->pic_id = pic_id;
        phandle->pause_flag = 1;
    }
    else
    {
        if(divoom_queue_is_full(pdivoom_custom_photo_handle->photo_pos_queue))
        {
            phandle = divoom_queue_pop(pdivoom_custom_photo_handle->photo_pos_queue);
        }
        else
        {
            phandle = divoom_system_malloc(sizeof(divoom_custom_photo_pos_info));
        }
        memcpy(&phandle->clock_info, clock_info, sizeof(divoom_clock_id_info));
        phandle->pic_id = pic_id;
        phandle->pause_flag = 1;
    }
    divoom_queue_push(pdivoom_custom_photo_handle->photo_pos_queue, phandle);
}

int divoom_custom_photo_get_pos_info(Pdivoom_clock_id_info clock_info, int *pause_flag)
{
    Pdivoom_custom_photo_pos_info phandle;
    
    phandle = divoom_queue_get_cmp(pdivoom_custom_photo_handle->photo_pos_queue, clock_info, divoom_custom_photo_handle_get_pos_compare);
    if(phandle)
    {
        char file_name[128];
        
        divoom_database_manage_get_custom_photo_name(phandle->pic_id, file_name);
        if(divoom_app_com_check_file_exist(file_name))
        {
            if(pause_flag)
            {
                *pause_flag = phandle->pause_flag;
            }
            phandle->pause_flag = 0;
            return phandle->pic_id;
        }
        else
        {
            phandle->pause_flag = 0;
            phandle->pic_id = 0;
            return 0;
        }
    }
    else
    {
        return 0;
    }
}

int divoom_custom_photo_get_play_one_flag(void *handle)
{
    Pdivoom_custom_photo_item  phandle = (Pdivoom_custom_photo_item)handle;

    if(phandle)
    {
        return phandle->pause_flag;
    }
    return 0;
}

void *divoom_custom_photo_open(Pdivoom_clock_id_info clock_info, int order_flag, int reverse_flag, int *play_pos)
{
    int old_pic;
    double start_time;
    Pdivoom_custom_photo_item  phandle;

    divoom_system_get_lock(pdivoom_custom_photo_handle->lock_handle);

    phandle = divoom_queue_pop_cmp(pdivoom_custom_photo_handle->photo_queue, clock_info, divoom_custom_photo_handle_compare);
    if(phandle == NULL)
    {
        phandle = divoom_system_malloc(sizeof(divoom_custom_photo_item));
        phandle->last_pic_id = -1;
        memcpy(&phandle->clock_info, clock_info, sizeof(divoom_clock_id_info));
        phandle->total_num = divoom_database_manage_get_total_custom_pic_num(clock_info,order_flag, reverse_flag, -1);
        phandle->record_offset = 0;
        phandle->order_flag = order_flag;
        phandle->reverse_flag = reverse_flag;
        phandle->read_pos = -1;
        if(phandle->total_num)
        {
            old_pic = divoom_custom_photo_get_pos_info(clock_info, &phandle->pause_flag);
            if(order_flag != DIVOOM_PIC_PHOTO_SLIDE_DISP_RANDOM && old_pic > 0)
            {
                divoom_custom_photo_info pic_info;
                
                divoom_database_manage_get_custom_photo_info(old_pic, &pic_info);
                if(order_flag == DIVOOM_PIC_PHOTO_SLIDE_DISP_PHTOT_TIME)
                {
                    start_time = pic_info.photo_time;
                }
                else
                {
                    start_time = pic_info.save_time;
                }
                phandle->record_offset = divoom_database_manage_get_total_custom_pic_num(clock_info, order_flag, reverse_flag, start_time) + 1;
                phandle->last_pic_id = old_pic;
            }
            else
            {
                if(phandle->pause_flag && old_pic > 0)
                {
                    phandle->last_pic_id = old_pic;
                }
                start_time = -1;
            }
            phandle->photo_num = DIVOOM_PIC_MANAGE_LOAD_MAX_PIC_NUM;
            phandle->photo_list = divoom_database_manage_get_custom_photo_list(clock_info, &phandle->photo_num, order_flag, reverse_flag, start_time);
            if(phandle->photo_num == 0 && start_time > 0)
            {
                phandle->photo_num = DIVOOM_PIC_MANAGE_LOAD_MAX_PIC_NUM;
                phandle->photo_list = divoom_database_manage_get_custom_photo_list(clock_info, &phandle->photo_num, order_flag, reverse_flag, -1);
                phandle->record_offset = 0;
            }
            if(order_flag == DIVOOM_PIC_PHOTO_SLIDE_DISP_RANDOM)
            {
                phandle->prand_handle = divoom_random_create(phandle->photo_num);
            }
        }
    }
    phandle->use_time ++;
    divoom_queue_push(pdivoom_custom_photo_handle->photo_queue, phandle);
    divoom_system_free_lock(pdivoom_custom_photo_handle->lock_handle);
    return phandle;
}

void divoom_custom_photo_delete_all_pic(void *handle)
{
    Pdivoom_custom_photo_item  phandle = (Pdivoom_custom_photo_item)handle;
    char file_name[256];

    for(int i = 0; i < phandle->photo_num; i++)
    {
        divoom_custom_photo_get_pic_name_from_id(phandle->photo_list[i], file_name);        
        divoom_pic_phone_manage_cache_delete_pic(file_name);    
    }
    phandle->total_num = 0;
    phandle->photo_num = 0;
    return ;
}

int divoom_custom_photo_get_last_pic_id(void *handle)
{
    Pdivoom_custom_photo_item  phandle = (Pdivoom_custom_photo_item)handle;

    return phandle->last_pic_id;
}

int divoom_custom_photo_get_total_items(void *handle)
{
    Pdivoom_custom_photo_item  phandle = (Pdivoom_custom_photo_item)handle;

    return phandle->total_num;
}

//得到相片ID
int divoom_custom_photo_get_pic_pos(void *handle,int pic_id)
{
    int pos;
    Pdivoom_custom_photo_item  phandle = (Pdivoom_custom_photo_item)handle;
    divoom_custom_photo_info pic_info;
    
    if(divoom_database_manage_get_custom_photo_info(pic_id, &pic_info) == 0)
    {
        return 0;
    }
    pos = divoom_database_manage_get_total_custom_pic_num(&phandle->clock_info,phandle->order_flag, phandle->reverse_flag, phandle->order_flag!=DIVOOM_PIC_PHOTO_SLIDE_DISP_PHTOT_TIME?pic_info.save_time:pic_info.photo_time);
    return pos;
}

//得到相片ID
void divoom_custom_photo_load_more_pic(void *handle)
{
    int last_pic_id;
    divoom_custom_photo_info pic_info;
    Pdivoom_custom_photo_item  phandle = (Pdivoom_custom_photo_item)handle;
    
    if(phandle->read_pos < phandle->photo_num -1)
    {
        phandle->read_pos ++;
        return;
    }

    if(phandle->total_num <= phandle->photo_num)
    {
        phandle->read_pos = 0;
        return;
    }

    phandle->record_offset += phandle->photo_num;
    last_pic_id = phandle->photo_list[phandle->photo_num - 1];
    divoom_database_manage_get_custom_photo_info(last_pic_id, &pic_info);
    phandle->read_pos = 0;
    divoom_system_free(phandle->photo_list);
    phandle->photo_num = DIVOOM_PIC_MANAGE_LOAD_MAX_PIC_NUM;
    
    phandle->photo_list = divoom_database_manage_get_custom_photo_list(&phandle->clock_info, &phandle->photo_num, phandle->order_flag, phandle->reverse_flag, phandle->order_flag!=DIVOOM_PIC_PHOTO_SLIDE_DISP_PHTOT_TIME?pic_info.save_time:pic_info.photo_time);
    if(phandle->photo_num == 0)
    {
        divoom_print("we will load from start!\n");
        phandle->photo_num = DIVOOM_PIC_MANAGE_LOAD_MAX_PIC_NUM;
        phandle->record_offset = 0;
        phandle->photo_list = divoom_database_manage_get_custom_photo_list(&phandle->clock_info, &phandle->photo_num, phandle->order_flag, phandle->reverse_flag, -1);
        
    }
    divoom_print("we load pic!: %d\n", phandle->photo_num);

    if(phandle->order_flag == DIVOOM_PIC_PHOTO_SLIDE_DISP_RANDOM)
    {
        if(phandle->prand_handle)
        {
            divoom_random_destroy(phandle->prand_handle);
            phandle->prand_handle = NULL;
        }
        if(phandle->photo_num)
        {
            phandle->prand_handle = divoom_random_create(phandle->photo_num);
        }
    }
}

//得到相片ID
void divoom_custom_photo_reload_more_pic(void *handle, int pos)
{
    double start_time;
    int read_num, dir, first_id,end_id;
    divoom_custom_photo_info pic_info;
    Pdivoom_custom_photo_item  phandle = (Pdivoom_custom_photo_item)handle;
    
    dir = phandle->reverse_flag;
    if(pos >= phandle->total_num - 1)
    {
        return;
    }
    read_num = phandle->photo_num;
    phandle->read_pos = 0;
    first_id = phandle->photo_list[0];
    end_id = phandle->photo_list[phandle->photo_num -1];
    divoom_system_free(phandle->photo_list);    
    if(pos == 0)
    {
        phandle->record_offset = 0;
        start_time = -1;
        phandle->photo_list = divoom_database_manage_get_custom_photo_list(&phandle->clock_info, &phandle->photo_num, phandle->order_flag, phandle->reverse_flag, start_time);
    }
    else if(pos < phandle->record_offset)
    {
        divoom_database_manage_get_custom_photo_info(first_id, &pic_info);
        if(phandle->order_flag == DIVOOM_PIC_PHOTO_SLIDE_DISP_PHTOT_TIME)
        {
            start_time = pic_info.photo_time;
        }
        else
        {
            start_time = pic_info.save_time;
        }
        if(pos + DIVOOM_PIC_MANAGE_LOAD_MAX_PIC_NUM < phandle->record_offset)
        {
            phandle->photo_num = phandle->record_offset - pos;
        }
        else
        {
            phandle->photo_num = DIVOOM_PIC_MANAGE_LOAD_MAX_PIC_NUM;
        }
        dir = phandle->reverse_flag?0:1;
        phandle->photo_list = divoom_database_manage_get_custom_photo_list_reverse(&phandle->clock_info, &phandle->photo_num, phandle->order_flag, phandle->reverse_flag, start_time);
        for(int i = 0; i < phandle->photo_num/2; i++)
        {
            int tmp = phandle->photo_list[i];
            phandle->photo_list[i] = phandle->photo_list[phandle->photo_num - i - 1];
            phandle->photo_list[phandle->photo_num - i - 1] = tmp;
        }
        phandle->record_offset -= phandle->photo_num;
    }
    else
    {
        divoom_database_manage_get_custom_photo_info(end_id, &pic_info);
        if(phandle->order_flag == DIVOOM_PIC_PHOTO_SLIDE_DISP_PHTOT_TIME)
        {
            start_time = pic_info.photo_time;
        }
        else
        {
            start_time = pic_info.save_time;
        }
        if(pos  > phandle->record_offset + DIVOOM_PIC_MANAGE_LOAD_MAX_PIC_NUM)
        {
            phandle->photo_num = pos - phandle->record_offset + 10;
        }
        else
        {
            phandle->photo_num = DIVOOM_PIC_MANAGE_LOAD_MAX_PIC_NUM;
        }
        phandle->photo_list = divoom_database_manage_get_custom_photo_list(&phandle->clock_info, &phandle->photo_num, phandle->order_flag, phandle->reverse_flag, start_time);
        phandle->record_offset += read_num;
    }

    if(phandle->photo_num == 0)
    {
        divoom_print("we will load from start!\n");
        phandle->photo_num = DIVOOM_PIC_MANAGE_LOAD_MAX_PIC_NUM;
        phandle->record_offset = 0;
        phandle->photo_list = divoom_database_manage_get_custom_photo_list(&phandle->clock_info, &phandle->photo_num, phandle->order_flag, dir, -1);
        
    }
    divoom_print("we load pic!: %d, %d,  %d, %d\n", pos, phandle->record_offset, phandle->photo_num, phandle->total_num);

}


//得到相片ID
int divoom_custom_photo_get_pic_id_inner(void *handle)
{
    int pic_pos;
    Pdivoom_custom_photo_item  phandle = (Pdivoom_custom_photo_item)handle;

    if(phandle->total_num == 0)
    {
        return 0;
    }
    divoom_custom_photo_load_more_pic(handle);
    if(phandle->photo_num == 0)
    {
        return 0;
    }
    if(phandle->order_flag == DIVOOM_PIC_PHOTO_SLIDE_DISP_RANDOM)
    {
        pic_pos = divoom_random_get(phandle->prand_handle);
    }
    else
    {
        pic_pos = phandle->read_pos;
    }
    
    return phandle->photo_list[pic_pos];
}

int divoom_custom_photo_get_file_name(void *handle,char *file_name)
{
    int ret, pic_id;
    Pdivoom_custom_photo_item  phandle = (Pdivoom_custom_photo_item)handle;
    Pdivoom_pic_phone_manage_slide_config pslide_config;

    if(phandle->photo_num == 0 || phandle->delete_flag == 1)
    {
        return 0;
    }
    divoom_system_get_lock(pdivoom_custom_photo_handle->lock_handle);
    pslide_config = divoom_pic_phone_manage_get_slide_config();
    if(phandle->order_flag != pslide_config->disp_order || phandle->reverse_flag != pslide_config->disp_reverse)
    {
        uint32 *photo_list;

        if(phandle->prand_handle)
        {
            divoom_random_destroy(phandle->prand_handle);
            phandle->prand_handle = NULL;
        }
        phandle->order_flag = pslide_config->disp_order;
        phandle->reverse_flag = pslide_config->disp_reverse;
        photo_list = divoom_database_manage_get_custom_photo_list(&phandle->clock_info, &phandle->photo_num, phandle->order_flag, phandle->reverse_flag, -1);
        divoom_system_free(phandle->photo_list);
        phandle->photo_list = photo_list;
        if(phandle->order_flag == DIVOOM_PIC_PHOTO_SLIDE_DISP_RANDOM)
        {
            phandle->prand_handle = divoom_random_create(phandle->photo_num);
        }
    }
    if(phandle->last_pic_id != -1)
    {
        pic_id = phandle->last_pic_id;
        phandle->last_pic_id = -1;
    }
    else
    {

        pic_id = divoom_custom_photo_get_pic_id_inner(handle);
    }
    if(pic_id == 0)
    {
        divoom_system_free_lock(pdivoom_custom_photo_handle->lock_handle);
        return 0;
    }
    divoom_database_manage_get_custom_photo_name(pic_id, file_name);
    divoom_system_free_lock(pdivoom_custom_photo_handle->lock_handle);

    return pic_id;
    
}

int divoom_custom_photo_get_pic_id(void *handle, int pos)
{
    int ret, pic_id;
    Pdivoom_custom_photo_item  phandle = (Pdivoom_custom_photo_item)handle;
    Pdivoom_pic_phone_manage_slide_config pslide_config;

    pos = pos%phandle->total_num;
    if(pos >=phandle->total_num || phandle->photo_num == 0 || phandle->delete_flag == 1)
    {
        return 0;
    }
    divoom_system_get_lock(pdivoom_custom_photo_handle->lock_handle);
    pslide_config = divoom_pic_phone_manage_get_slide_config();

    if(pos >= phandle->record_offset && pos < phandle->record_offset + phandle->photo_num)
    {
        pic_id = phandle->photo_list[pos - phandle->record_offset];
    }
    else
    {
        divoom_custom_photo_reload_more_pic(handle, pos);
        if(pos >= phandle->record_offset && pos < phandle->record_offset + phandle->photo_num)
        {
            pic_id = phandle->photo_list[pos - phandle->record_offset];
        }
        else 
        {
            pic_id = 0;
        }
    }
    divoom_system_free_lock(pdivoom_custom_photo_handle->lock_handle);
    if(pic_id == 0)
    {
        return 0;
    }

    return pic_id;
    
}

void divoom_custom_photo_close(void *handle)
{
    Pdivoom_custom_photo_item  phandle = (Pdivoom_custom_photo_item)handle;

    if(phandle == NULL)
    {
        return;
    }
    divoom_system_get_lock(pdivoom_custom_photo_handle->lock_handle);

    phandle = divoom_queue_pop_cmp(pdivoom_custom_photo_handle->photo_queue, &phandle->clock_info, divoom_custom_photo_handle_compare);
    if(phandle)
    {
        phandle->use_time --;
        if(phandle->use_time == 0)
        {
            divoom_system_free(phandle->photo_list);
            if(phandle->prand_handle)
            {
                divoom_random_destroy(phandle->prand_handle);
            }
            divoom_system_free(phandle);
        }
        else
        {
            divoom_queue_push(pdivoom_custom_photo_handle->photo_queue, phandle);
        }
    }
    else
    {
        divoom_print("this is err1\n");
    }
    divoom_system_free_lock(pdivoom_custom_photo_handle->lock_handle);
}

/*********************************************************************
  Function:        divoom_custom_photo_get_album_local_pic_list
  Description:     APP获取相簿相片列表
  Input:           clock_info
                   start
                   end
  Return:          NONE
**********************************************************************/

void divoom_custom_photo_get_album_local_pic_list(Pdivoom_clock_id_info clock_info, int start, int end)
{
    Pdivoom_pic_id_name_info photo_list;
    int i, pic_num, total_num, src_w, src_h, last_read_pic_id;
    double start_time = -1;
    divoom_port_stat_t file_st;
    Pdivoom_photo_album_get_local_photo_request_info pinfo;
    Pdivoom_pic_phone_manage_slide_config pslide_config = divoom_pic_phone_manage_get_slide_config();

    if(pslide_config == NULL)
    {
        divoom_net_app_send_packet(DIVOOM_NET_COMM_PHOTO_GET_LOCAL_PHOTO_LIST, DIVOOM_APP_TOPIC, NULL);
        return;
    }
    if(start == 0 || memcmp(clock_info, &pdivoom_custom_photo_handle->read_clock_info, sizeof(divoom_clock_id_info)))
    {
        last_read_pic_id = 0;
        start_time = -1;
    }
    else
    {
        divoom_custom_photo_info pic_info;
        
        last_read_pic_id = pdivoom_custom_photo_handle->last_id;
        divoom_database_manage_get_custom_photo_info(last_read_pic_id, &pic_info);
        start_time = pslide_config->disp_order==DIVOOM_PIC_PHOTO_SLIDE_DISP_PHTOT_TIME?pic_info.photo_time:pic_info.save_time;
    }
    
    pic_num = end - start;
    if(pic_num <= 0)
    {
        divoom_net_app_send_packet(DIVOOM_NET_COMM_PHOTO_GET_LOCAL_PHOTO_LIST, DIVOOM_APP_TOPIC, NULL);
        return;
    }
    if(pic_num > 100)
    {
        pic_num = 100;
    }
    
    
    photo_list = divoom_database_manage_get_custom_local_photo_list(clock_info, &pic_num, start_time, pslide_config->disp_order, pslide_config->disp_reverse);
    if(photo_list && pic_num > 0)
    {
        pinfo = divoom_system_malloc(sizeof(divoom_photo_album_get_local_photo_request_info) + pic_num * sizeof(divoom_photo_album_get_local_photo_request_item));
        if(pinfo)
        {
            pinfo->plist = (Pdivoom_photo_album_get_local_photo_request_item)&pinfo[1];
            pinfo->num = 0;
            for(i = 0; i < pic_num; i++)
            {
                pinfo->plist[i].photo_id = photo_list[i].id;
                pinfo->plist[i].save_time = photo_list[i].time;
                strcpy(pinfo->plist[i].big_name, photo_list[i].name);
                divoom_thumb_cache_get_thumb_name(pinfo->plist[pinfo->num].big_name, pinfo->plist[pinfo->num].small_name);
                //判断一下，是不是错误文件,如果文件不存在或者大小写为0，统一认为错误，将重新生成
                if (divoom_port_stat(pinfo->plist[pinfo->num].small_name, &file_st) == 0) {
                    if(file_st.size > 0)
                    {
                        pinfo->num ++;
                    }
                    else
                    {
                        strcpy(pinfo->plist[pinfo->num].small_name, pinfo->plist[pinfo->num].big_name);
                    }
                }
                else
                {
                        strcpy(pinfo->plist[pinfo->num].small_name, pinfo->plist[pinfo->num].big_name);
                }
            }
            divoom_net_app_send_packet(DIVOOM_NET_COMM_PHOTO_GET_LOCAL_PHOTO_LIST, DIVOOM_APP_TOPIC,pinfo);
            divoom_system_free(pinfo);
        }
        memcpy(&pdivoom_custom_photo_handle->read_clock_info, clock_info, sizeof(divoom_clock_id_info));
        pdivoom_custom_photo_handle->last_id = photo_list[pic_num - 1].id;
        divoom_system_free(photo_list);
    }
    else
    {
        divoom_net_app_send_packet(DIVOOM_NET_COMM_PHOTO_GET_LOCAL_PHOTO_LIST, DIVOOM_APP_TOPIC, NULL);
        if(photo_list)
        {
            divoom_system_free(photo_list);
        }
    }
    
}
/*********************************************************************
  Function:        divoom_custom_photo_get_pic_name_from_id
  Description:     通过id得到文件名
  Input:           pic_id
                   file_name
  Return:          NONE
**********************************************************************/

void divoom_custom_photo_get_pic_name_from_id(int pic_id, char *file_name)
{
    divoom_database_manage_get_custom_photo_name(pic_id, file_name);    
}



#include "divoom.h"

#if 0
#define DIVOOM_IMAGE_DOWNLOAD_MANAGE_MAX_CACHE_SIZE   (500*1024)

typedef struct _divoom_image_download_manage_item
{
    uint8   status;         //0：表示此项将没有被使用，可以清理，1:表示这个可以被使用
    uint8   use_num;        //此文件被使用次数
    uint16  check_sum;      //文件名的checksum
    int     pic_len;        //图片内容
    char    file_name[52];  //服务器文件名固定51
    uint8   *pic_data;      //图片数据
}divoom_image_download_manage_item, *Pdivoom_image_download_manage_item;


typedef struct _divoom_image_download_manage_ctrl
{
    int     run_flag;
    int     tol_item;          //总项数
    int     change_item;
    int     ready_num;        //已经加载的文件数目
    int     tol_cache_size;   //使用的总缓冲数据
    void    *lock;
    void    *task_handles;
    void    *load_sem;
    void    *image_queue;
}divoom_image_download_manage_ctrl, *Pdivoom_image_download_manage_ctrl;


static Pdivoom_image_download_manage_ctrl pdivoom_image_download_manage_handle = NULL;


/**
* NAME         divoom_image_download_manage_compare_file
* @BRIEF	   比较文件是否相同
* @PARAM	   file_name
               pic_len      数据长度
* @RETURN	   数据指针
* @RETVAL
*/

typedef struct _divoom_image_download_manage_compare_info
{
    uint16 check_sum;
    char *file_name;
}divoom_image_download_manage_compare_info, *Pdivoom_image_download_manage_compare_info;

int divoom_image_download_manage_compare_file(void *item1, void *item2)
{
    Pdivoom_image_download_manage_compare_info src_info = (Pdivoom_image_download_manage_compare_info)item1;
    Pdivoom_image_download_manage_item image_item = (Pdivoom_image_download_manage_item)item2;

    if(src_info->check_sum == image_item->check_sum)
    {
        return strcmp(src_info->file_name, image_item->file_name);
    }
    return 1;
}

/**
* NAME         divoom_image_download_manage_save
* @BRIEF	   将模块从本地加载之前保存的数据
* @PARAM	   
* @RETURN	 返回NONE
* @RETVAL
*/

void divoom_image_download_manage_load(void)
{
	FILE *divoom_load_handle;
    uint16 i, check_sum = 0, raw_check_sum = 0, tol_item;
    Pdivoom_image_download_manage_item image_item;

	divoom_load_handle = divoom_port_file_open(DIVOOM_IMG_DOWN_FILE_CFG_SAVE_PATH, "rb");
	if(divoom_load_handle == NULL)
	{
        return;
	}
    
    divoom_port_file_read(&tol_item, sizeof(tol_item), 1, divoom_load_handle);
    check_sum = divoom_app_com_get_check_sum((uint8 *)&tol_item, sizeof(tol_item));
    pdivoom_image_download_manage_handle->tol_item = tol_item;
    for(i = 0; i < pdivoom_image_download_manage_handle->tol_item && pdivoom_image_download_manage_handle->tol_item < 1024; i++)
    {
        image_item = divoom_system_malloc(sizeof(divoom_image_download_manage_item));
        if(image_item == NULL)
        {
            break;
        }
        divoom_port_file_read(image_item, sizeof(divoom_image_download_manage_item), 1, divoom_load_handle);
        check_sum += divoom_app_com_get_check_sum((uint8 *)image_item, sizeof(divoom_image_download_manage_item));
        if(image_item->pic_len && image_item->pic_len < 128*1024)
        {
            image_item->pic_data = divoom_system_malloc(image_item->pic_len);
            if(image_item->pic_data == NULL)
            {
                divoom_system_free(image_item);
                break;
            }
            divoom_port_file_read(image_item->pic_data, image_item->pic_len, 1, divoom_load_handle);
            check_sum += divoom_app_com_get_check_sum((uint8 *)image_item->pic_data, image_item->pic_len);
            pdivoom_image_download_manage_handle->ready_num ++;
            pdivoom_image_download_manage_handle->tol_cache_size += image_item->pic_len;
        }
        else
        {
            image_item->pic_data = NULL;
        }
        divoom_queue_push(pdivoom_image_download_manage_handle->image_queue, image_item);
        
    }    
    divoom_port_file_read(&raw_check_sum, sizeof(raw_check_sum), 1, divoom_load_handle);
	divoom_port_file_close(divoom_load_handle);
    if(raw_check_sum != check_sum)
    {
        //保存的数据有问题，我们将清除所有数据
        divoom_print("%s: we fails to load config!tol: %d, sum: %d, %d\n", __func__,pdivoom_image_download_manage_handle->tol_item, raw_check_sum, check_sum);
        while(1)
        {
            image_item = divoom_queue_pop(pdivoom_image_download_manage_handle->image_queue);
            if(image_item == NULL)
            {
                break;
            }
            divoom_system_free(image_item->pic_data);
            divoom_system_free(image_item);
        }
        pdivoom_image_download_manage_handle->ready_num = 0;
        pdivoom_image_download_manage_handle->tol_cache_size = 0;
        pdivoom_image_download_manage_handle->tol_item = 0;
    }
    divoom_print("%s: we load items: %d!\n", __func__, pdivoom_image_download_manage_handle->tol_item);
}

/**
* NAME         divoom_image_download_manage_save
* @BRIEF	   将模块所有已经加载的图片数据保存
* @PARAM	   
* @RETURN	 返回NONE
* @RETVAL
*/

void divoom_image_download_manage_save(void)
{
	FILE *divoom_load_handle;
    uint16 i, check_sum = 0, tol_item;
    Pdivoom_image_download_manage_item image_item;

    if(pdivoom_image_download_manage_handle->change_item == 0)
    {
        divoom_print("we will pass the save beacuse data isn't changed!\n");
        return;
    }
    
	divoom_load_handle = divoom_port_file_open(DIVOOM_IMG_DOWN_FILE_CFG_SAVE_PATH, "wb");
	if(divoom_load_handle == NULL)
	{
        return;
	}
    tol_item = divoom_queue_items(pdivoom_image_download_manage_handle->image_queue);
    divoom_port_file_write(&tol_item, sizeof(tol_item), 1, divoom_load_handle);    
    check_sum = divoom_app_com_get_check_sum((uint8 *)&tol_item, sizeof(tol_item));

    for(i = 0; i < tol_item; i++)
    {
        image_item = divoom_queue_get_index_item(pdivoom_image_download_manage_handle->image_queue, i);
        if(image_item == NULL)
        {
            break;
        }
        
        divoom_port_file_write(image_item, sizeof(divoom_image_download_manage_item), 1, divoom_load_handle);
        check_sum += divoom_app_com_get_check_sum((uint8 *)image_item, sizeof(divoom_image_download_manage_item));
        if(image_item->pic_len)
        {
            divoom_port_file_write(image_item->pic_data, image_item->pic_len, 1, divoom_load_handle);
            check_sum += divoom_app_com_get_check_sum((uint8 *)image_item->pic_data, image_item->pic_len);
        }
    }
    divoom_port_file_write(&check_sum, sizeof(check_sum), 1, divoom_load_handle);
    divoom_port_file_close(divoom_load_handle);
    divoom_print("%s: we save items: %d, sum: %d!\n", __func__, pdivoom_image_download_manage_handle->tol_item, check_sum);
}

int divoom_image_download_manage_get_unload_file(void *item1, void *item2)
{
    Pdivoom_image_download_manage_item image_item = (Pdivoom_image_download_manage_item)item2;

    return image_item->pic_data != NULL;
}

int divoom_image_download_manage_get_unuse_file(void *item1, void *item2)
{
    Pdivoom_image_download_manage_item image_item = (Pdivoom_image_download_manage_item)item2;

    return image_item->use_num > 0;
}

int divoom_image_download_manage_get_find_file(void *item1, void *item2)
{
    return item1 != item2;
}

void divoom_image_download_manage_down_task(void *param)
{
    Pdivoom_image_download_manage_item image_item;
    char file_name[52];
    uint8 *image_data;
    int image_len;
    while(pdivoom_image_download_manage_handle->run_flag)
    {
        divoom_system_get_sem(pdivoom_image_download_manage_handle->load_sem);
        while(pdivoom_image_download_manage_handle->run_flag && pdivoom_image_download_manage_handle->ready_num < pdivoom_image_download_manage_handle->tol_item)
        {
            divoom_system_get_lock(pdivoom_image_download_manage_handle->lock);
            if(pdivoom_image_download_manage_handle->tol_cache_size > DIVOOM_IMAGE_DOWNLOAD_MANAGE_MAX_CACHE_SIZE)
            {
                divoom_system_free_lock(pdivoom_image_download_manage_handle->lock);
                break;
            }
            //取一个没有被加载数据的项
            image_item = divoom_queue_get_cmp(pdivoom_image_download_manage_handle->image_queue, NULL, divoom_image_download_manage_get_unload_file);
            if(image_item)
            {
                strcpy(file_name, image_item->file_name);
            }
            else
            {
                file_name[0] = 0;
            }
            divoom_system_free_lock(pdivoom_image_download_manage_handle->lock);
            if(strlen(file_name))
            {
                image_data = divoom_format_srv_gif_file_to_buf(file_name, &image_len);
                if(image_data)
                {
                    divoom_system_get_lock(pdivoom_image_download_manage_handle->lock);
                    //重新取一次， 怕万一被修改了
                    image_item = divoom_queue_get_cmp(pdivoom_image_download_manage_handle->image_queue, image_item, divoom_image_download_manage_get_find_file);
                    if(image_item && image_item->pic_data == NULL)
                    {
                        pdivoom_image_download_manage_handle->change_item = 1;
                        image_item->pic_data = image_data;
                        image_item->pic_len = image_len;
                        pdivoom_image_download_manage_handle->tol_cache_size += image_len;
                        pdivoom_image_download_manage_handle->ready_num ++;
                    }
                    else
                    {
                        divoom_system_free(image_data);
                    }
                    divoom_system_free_lock(pdivoom_image_download_manage_handle->lock);
                }
            }
            divoom_system_delay_ms(10);
        }
    }
}

/**
* NAME         divoom_image_download_manage_stop
* @BRIEF	   停止文件下载，结束下载线程
* @PARAM	   
* @RETURN	 返回NONE
* @RETVAL
*/

void divoom_image_download_manage_stop(void)
{
    if(pdivoom_image_download_manage_handle)
    {
        pdivoom_image_download_manage_handle->run_flag = 0;
        divoom_system_put_sem(pdivoom_image_download_manage_handle->load_sem);
    }
}

/**
* NAME         divoom_image_download_manage_init
* @BRIEF	   模块初始化，加载所有的以前保存在本地的信息
* @PARAM	   
* @RETURN	 返回NONE
* @RETVAL
*/

void divoom_image_download_manage_init(void)
{
    if(pdivoom_image_download_manage_handle)
    {
        return;
    }
    pdivoom_image_download_manage_handle = divoom_system_malloc(sizeof(divoom_image_download_manage_ctrl));
    if(pdivoom_image_download_manage_handle == NULL)
    {
        return;
    }
    memset(pdivoom_image_download_manage_handle, 0, sizeof(divoom_image_download_manage_ctrl));
    pdivoom_image_download_manage_handle->image_queue = divoom_queue_init(1000);
    pdivoom_image_download_manage_handle->run_flag = 1;
    divoom_image_download_manage_load();
    pdivoom_image_download_manage_handle->lock = divoom_system_create_lock();
    pdivoom_image_download_manage_handle->load_sem = divoom_system_create_sem(1, 0);
    pdivoom_image_download_manage_handle->task_handles = divoom_system_create_task_prio(divoom_image_download_manage_down_task, "divoom_image_download_manage", (void *)pdivoom_image_download_manage_handle, 4*1024, 1);
}



/**
* NAME         divoom_image_download_manage_start_update
* @BRIEF	   通知模块将进行更新，将所有的元素status状态设置为未使用
* @PARAM	   
* @RETURN	 返回NONE
* @RETVAL
*/

void divoom_image_download_manage_start_update(void)
{
    int i, tol_item;
    Pdivoom_image_download_manage_item image_item;
    
    if(pdivoom_image_download_manage_handle == NULL)
    {
        return;
    }
    divoom_system_get_lock(pdivoom_image_download_manage_handle->lock);
    
    tol_item = divoom_queue_items(pdivoom_image_download_manage_handle->image_queue);
    
    for(i = 0; i < tol_item; i++)
    {
        image_item = divoom_queue_get_index_item(pdivoom_image_download_manage_handle->image_queue, i);
        if(image_item == NULL)
        {
            break;
        }
        image_item->use_num = 0;
    }    
    divoom_system_free_lock(pdivoom_image_download_manage_handle->lock);
}

/**
* NAME         divoom_image_download_manage_add_file
* @BRIEF	   将文件加入缓冲队列，由后台加载文件
* @PARAM	   file_name
* @RETURN	 返回NONE
* @RETVAL
*/

void divoom_image_download_manage_add_file(char *file_name)
{
    divoom_image_download_manage_compare_info file_info;
    Pdivoom_image_download_manage_item image_item;
    
    if(pdivoom_image_download_manage_handle == NULL)
    {
        return;
    }
    file_info.file_name = file_name;
    file_info.check_sum = divoom_app_com_get_check_sum((uint8 *)file_name, strlen(file_name));
    divoom_system_get_lock(pdivoom_image_download_manage_handle->lock);
    image_item = divoom_queue_get_cmp(pdivoom_image_download_manage_handle->image_queue, (void *)&file_info, divoom_image_download_manage_compare_file);
    
    if(image_item == NULL)
    {
        //将增加一项
        image_item = divoom_system_malloc(sizeof(divoom_image_download_manage_item));
        if(image_item)
        {
            strcpy(image_item->file_name, file_name);
            image_item->check_sum = divoom_app_com_get_check_sum((uint8 *)file_name, strlen(file_name));
            image_item->use_num = 1;
            image_item->pic_data = NULL;
            image_item->pic_len = 0;
            pdivoom_image_download_manage_handle->tol_item ++;
            divoom_queue_push(pdivoom_image_download_manage_handle->image_queue, image_item);
        }
    }
    else
    {
        image_item->use_num ++;
    }
    divoom_system_free_lock(pdivoom_image_download_manage_handle->lock);
    divoom_system_put_sem(pdivoom_image_download_manage_handle->load_sem);
}


/**
* NAME         divoom_image_download_manage_del_file
* @BRIEF	   将文件从缓冲队列移除
* @PARAM	   file_name
* @RETURN	 返回NONE
* @RETVAL
*/

void divoom_image_download_manage_del_file(char *file_name)
{
    divoom_image_download_manage_compare_info file_info;
    Pdivoom_image_download_manage_item image_item;
    
    if(pdivoom_image_download_manage_handle == NULL)
    {
        return;
    }
    
    file_info.file_name = file_name;
    file_info.check_sum = divoom_app_com_get_check_sum((uint8 *)file_name, strlen(file_name));
    divoom_system_get_lock(pdivoom_image_download_manage_handle->lock);
    image_item = divoom_queue_get_cmp(pdivoom_image_download_manage_handle->image_queue, (void *)&file_info, divoom_image_download_manage_compare_file);
    if(image_item)
    {
        if(image_item->use_num == 1)
        {
            //使用计数为0，将文件删除
            image_item = divoom_queue_pop_cmp(pdivoom_image_download_manage_handle->image_queue, (void *)&file_info, divoom_image_download_manage_compare_file);
            pdivoom_image_download_manage_handle->tol_cache_size -= image_item->pic_len;
            pdivoom_image_download_manage_handle->tol_item --;
            if(image_item->pic_data)
            {
                pdivoom_image_download_manage_handle->ready_num --;
                divoom_system_free(image_item->pic_data);
            }
            divoom_system_free(image_item);
        }
        else
        {
            image_item->use_num --;
        }
    }
    divoom_system_free_lock(pdivoom_image_download_manage_handle->lock);
}

/**
* NAME         divoom_image_download_manage_end_update
* @BRIEF	   更新文件列表完成，将未使用的文件从队列中清除
* @PARAM	   
* @RETURN	 返回NONE
* @RETVAL
*/

void divoom_image_download_manage_end_update(void)
{
    Pdivoom_image_download_manage_item image_item;
    
    if(pdivoom_image_download_manage_handle == NULL)
    {
        return;
    }
    divoom_system_get_lock(pdivoom_image_download_manage_handle->lock);
    while(1)
    {
        image_item = divoom_queue_pop_cmp(pdivoom_image_download_manage_handle->image_queue, NULL, divoom_image_download_manage_get_unuse_file);
        if(image_item == NULL)
        {
            break;
        }
        pdivoom_image_download_manage_handle->tol_cache_size -= image_item->pic_len;
        if(image_item->pic_data)
        {
            pdivoom_image_download_manage_handle->ready_num --;
        }
        pdivoom_image_download_manage_handle->tol_item --;
        divoom_system_free(image_item->pic_data);
        divoom_system_free(image_item);
    }
    divoom_system_free_lock(pdivoom_image_download_manage_handle->lock);
}

/**
* NAME         divoom_image_download_manage_get_file
* @BRIEF	   获取文件数据，
* @PARAM	   file_name
               pic_len      数据长度
* @RETURN	   数据指针
* @RETVAL
*/

uint8 *divoom_image_download_manage_get_file(char *file_name, int *pic_len)
{
    uint8 *image_data = NULL;
    divoom_image_download_manage_compare_info file_info;
    Pdivoom_image_download_manage_item image_item;
    
    
    if(pdivoom_image_download_manage_handle == NULL)
    {
        return divoom_format_srv_gif_file_to_buf(file_name, pic_len);
    }
    file_info.file_name = file_name;
    file_info.check_sum = divoom_app_com_get_check_sum((uint8 *)file_name, strlen(file_name));
    divoom_system_get_lock(pdivoom_image_download_manage_handle->lock);
    image_item = divoom_queue_get_cmp(pdivoom_image_download_manage_handle->image_queue, (void *)&file_info, divoom_image_download_manage_compare_file);
    if(image_item)
    {
        if(image_item->pic_data)
        {
            image_data = divoom_system_malloc(image_item->pic_len);
            memcpy(image_data, image_item->pic_data, image_item->pic_len);
            *pic_len = image_item->pic_len;
            divoom_system_free_lock(pdivoom_image_download_manage_handle->lock);
            return image_data;
        }
        *pic_len = image_item->pic_len;
    }
    divoom_system_free_lock(pdivoom_image_download_manage_handle->lock);
    if(image_item == NULL)
    {
        //将增加一项
        image_data = divoom_format_srv_gif_file_to_buf(file_name, pic_len);
        
        if(image_data)
        {
            image_item = divoom_system_malloc(sizeof(divoom_image_download_manage_item));
            strcpy(image_item->file_name, file_name);
            image_item->check_sum = divoom_app_com_get_check_sum((uint8 *)file_name, strlen(file_name));
            image_item->use_num = 1;
            image_item->pic_data = divoom_system_malloc(*pic_len);
            if(image_item->pic_data)
            {
                memcpy(image_item->pic_data, image_data, *pic_len);
                image_item->pic_len = *pic_len;
                divoom_system_get_lock(pdivoom_image_download_manage_handle->lock);                
                pdivoom_image_download_manage_handle->change_item = 1;
                pdivoom_image_download_manage_handle->tol_cache_size += *pic_len;
                pdivoom_image_download_manage_handle->ready_num ++;
                pdivoom_image_download_manage_handle->tol_item ++;
                divoom_queue_push(pdivoom_image_download_manage_handle->image_queue, image_item);
                divoom_system_free_lock(pdivoom_image_download_manage_handle->lock);
            }
            else
            {
                divoom_free(image_item);
            }
        }
        return image_data;
    }
    if(image_item->pic_data == NULL)
    {
        image_data = divoom_format_srv_gif_file_to_buf(file_name, pic_len);
        if(image_data)
        {
            divoom_system_get_lock(pdivoom_image_download_manage_handle->lock);
            image_item = divoom_queue_get_cmp(pdivoom_image_download_manage_handle->image_queue, image_item, divoom_image_download_manage_get_find_file);
            if(image_item && image_item->pic_data == NULL)
            {
                image_item->pic_data = divoom_system_malloc(*pic_len);
                if(image_item->pic_data)
                {
                    memcpy(image_item->pic_data, image_data, *pic_len);             
                    pdivoom_image_download_manage_handle->change_item = 1;
                    pdivoom_image_download_manage_handle->tol_cache_size += *pic_len;
                    pdivoom_image_download_manage_handle->ready_num ++;
                    image_item->pic_len = *pic_len;
                }
            }
            divoom_system_free_lock(pdivoom_image_download_manage_handle->lock);
        }
    }
    return image_data;
}

#endif


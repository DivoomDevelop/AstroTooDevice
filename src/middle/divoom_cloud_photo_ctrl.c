#include "divoom.h"


typedef struct _divoom_cloud_photo_ctrl_info
{
    int num;
    int index;
    Pdivoom_vision_item plist;
}divoom_cloud_photo_ctrl_info, *Pdivoom_cloud_photo_ctrl_info;


typedef struct _divoom_cloud_photo_ctrl_ctrl
{
    int down_vision_id;
    int play_vision_id;
    int last_update_time;
    void *lock_handle;
    void *del_queue;
    int  use_times;
    Pdivoom_cloud_photo_ctrl_info phandle;
}divoom_cloud_photo_ctrl_ctrl, *Pdivoom_cloud_photo_ctrl_ctrl;


static Pdivoom_cloud_photo_ctrl_ctrl pdivoom_cloud_photo_ctrl_handle = NULL;

/*********************************************************************
  Function:        divoom_cloud_photo_ctrl_init
  Description:     初始化
  Input:           
                   
  Return:          ＮＯＮＥ
**********************************************************************/

void divoom_cloud_photo_ctrl_init(void)
{
	char *table_item = "id INTEGER PRIMARY KEY     NOT NULL," \
		"download INTEGER DEFAULT 0," \
		"update_time INTEGER DEFAULT 0," \
		"file_name CHAR(200)";

    if(divoom_main_get_custom_product_flag() == 0)
    {
        return;
    }
    
	divoom_database_manage_add_new_table(DIVOOM_CLOUD_PHOTO_TABLE_NAME, table_item);
    divoom_database_manage_add_table_index(DIVOOM_CLOUD_PHOTO_TABLE_NAME, "update_time");
    pdivoom_cloud_photo_ctrl_handle = divoom_system_malloc(sizeof(divoom_cloud_photo_ctrl_ctrl));
    pdivoom_cloud_photo_ctrl_handle->lock_handle = divoom_system_create_lock();
    pdivoom_cloud_photo_ctrl_handle->del_queue = divoom_queue_init(10, __func__, __LINE__);
    pdivoom_cloud_photo_ctrl_handle->last_update_time = divoom_database_manage_get_vision_last_update_time(DIVOOM_CLOUD_PHOTO_TABLE_NAME);
    
}

void divoom_cloud_photo_ctrl_add_item_finish_callback(struct _divoom_download_aliyun_file_item *handle)
{
    divoom_database_manage_update_vision_photo_item_download(DIVOOM_CLOUD_PHOTO_TABLE_NAME,handle->photo_id);
    divoom_system_get_lock(pdivoom_cloud_photo_ctrl_handle->lock_handle);
    if(pdivoom_cloud_photo_ctrl_handle->phandle)
    {
        for(int i = 0; i < pdivoom_cloud_photo_ctrl_handle->phandle->num; i++)
        {
            if(pdivoom_cloud_photo_ctrl_handle->phandle->plist[i].vision_id == handle->photo_id)
            {
                pdivoom_cloud_photo_ctrl_handle->phandle->plist[i].download_flag = 1;
                pdivoom_cloud_photo_ctrl_handle->play_vision_id = handle->photo_id;
                break;
            }
        }
    }
    divoom_system_free_lock(pdivoom_cloud_photo_ctrl_handle->lock_handle);
}

/*********************************************************************
  Function:        divoom_cloud_photo_ctrl_add_item
  Description:     增加一个元素
  Input:           vision_id
                   oss_name
  Return:          ＮＯＮＥ
**********************************************************************/

void divoom_cloud_photo_ctrl_request_more_info(int clock_id)
{    
    
    if(pdivoom_cloud_photo_ctrl_handle)
    {
        divoom_device_cloud_photo_request_info pinfo;
        

        pinfo.clock_id = clock_id;
        pinfo.product_flag = divoom_main_get_custom_product_flag();
        pinfo.update_time = pdivoom_cloud_photo_ctrl_handle->last_update_time;
        divoom_net_app_send_packet(DIVOOM_NET_COMM_CLOUD_PHOTO_GET_IMAGE_LIST, DIVOOM_SERVER_TOPIC,&pinfo);
    }
}

/*********************************************************************
  Function:        divoom_cloud_photo_ctrl_add_item
  Description:     增加一个元素
  Input:           vision_id
                   oss_name
  Return:          ＮＯＮＥ
**********************************************************************/

void divoom_cloud_photo_ctrl_add_item(Pdivoom_device_cloud_photo_item pitem_info)
{    
    int i, find_flag = 0;
    char file_name[256];
    
    if(pdivoom_cloud_photo_ctrl_handle == NULL)
    {
        return;
    }
    divoom_system_get_lock(pdivoom_cloud_photo_ctrl_handle->lock_handle);
    divoom_print("aj test!\n");
    sprintf(file_name, "%s/%d.webp", DIVOOM_CLOUD_PHOTO_INFO_DIR, pitem_info->id);
    if(divoom_app_com_check_file_exist(file_name) == 0)
    {
        if(pdivoom_cloud_photo_ctrl_handle->phandle)
        {
            
            for(i = 0; i < pdivoom_cloud_photo_ctrl_handle->phandle->num; i++)
            {
                if(pdivoom_cloud_photo_ctrl_handle->phandle->plist[i].vision_id == pitem_info->id)
                {
                    find_flag = 1;
                    break;
                }
            }
        }
        if(find_flag == 0)
        {
            
            if(pdivoom_cloud_photo_ctrl_handle->phandle)
            {
                Pdivoom_vision_item plist = divoom_system_malloc(sizeof(divoom_vision_item)*(pdivoom_cloud_photo_ctrl_handle->phandle->num + 1));

                memcpy(plist, pdivoom_cloud_photo_ctrl_handle->phandle->plist, sizeof(divoom_vision_item)*(pdivoom_cloud_photo_ctrl_handle->phandle->num ));
                plist[pdivoom_cloud_photo_ctrl_handle->phandle->num].vision_id = pitem_info->id;
                plist[pdivoom_cloud_photo_ctrl_handle->phandle->num].download_flag = 0;            
                pdivoom_cloud_photo_ctrl_handle->phandle->num ++;
                divoom_system_free(pdivoom_cloud_photo_ctrl_handle->phandle->plist);
                pdivoom_cloud_photo_ctrl_handle->phandle->plist = plist;
            }
            divoom_database_manage_insert_vision_photo_item_with_time(DIVOOM_CLOUD_PHOTO_TABLE_NAME,pitem_info->id, pitem_info->oss_name,pitem_info->update_time);
            pdivoom_cloud_photo_ctrl_handle->down_vision_id = pitem_info->id;
            divoom_download_aliyun_file_add_cloud_photo_item(pitem_info->id, pitem_info->oss_flag, pitem_info->oss_name, divoom_cloud_photo_ctrl_add_item_finish_callback);
        }
    }
    if(pdivoom_cloud_photo_ctrl_handle->last_update_time < pitem_info->update_time)
    {
        pdivoom_cloud_photo_ctrl_handle->last_update_time = pitem_info->update_time;
    }
    divoom_print("aj test!\n");
    divoom_system_free_lock(pdivoom_cloud_photo_ctrl_handle->lock_handle);
}


/*********************************************************************
  Function:        divoom_cloud_photo_ctrl_del_item
  Description:     删除一个元素
  Input:           vision_id
  Return:          ＮＯＮＥ
**********************************************************************/

void divoom_cloud_photo_ctrl_del_item(int vision_id)
{
    char file_name[256];
    
    if(pdivoom_cloud_photo_ctrl_handle == NULL)
    {
        return;
    }
    divoom_system_get_lock(pdivoom_cloud_photo_ctrl_handle->lock_handle);
    divoom_print("aj test!\n");
    divoom_database_manage_delete_vision_photo_item(DIVOOM_CLOUD_PHOTO_TABLE_NAME,vision_id);
    sprintf(file_name, "%s/%d.webp", DIVOOM_CLOUD_PHOTO_INFO_DIR, vision_id);
    if(pdivoom_cloud_photo_ctrl_handle->phandle)
    {
        if(vision_id == pdivoom_cloud_photo_ctrl_handle->play_vision_id)
        {
            int *del_id = divoom_system_malloc(sizeof(int));

            *del_id = vision_id;
            divoom_queue_push(pdivoom_cloud_photo_ctrl_handle->del_queue, del_id);
        }
		else
    	{
        	divoom_app_com_del_file(file_name);
   	 	}
        for(int i = 0; i < pdivoom_cloud_photo_ctrl_handle->phandle->num; i++)
        {
            if(pdivoom_cloud_photo_ctrl_handle->phandle->plist[i].vision_id == vision_id)
            {
                if(i != pdivoom_cloud_photo_ctrl_handle->phandle->num -1)
                {
                    memmove(&pdivoom_cloud_photo_ctrl_handle->phandle->plist[i], &pdivoom_cloud_photo_ctrl_handle->phandle->plist[i+1], (pdivoom_cloud_photo_ctrl_handle->phandle->num - (i+1)) * sizeof(divoom_vision_item));
                }
                pdivoom_cloud_photo_ctrl_handle->phandle->num --;
                break;
            }
        }
    }
    else
    {
        divoom_app_com_del_file (file_name);
    }
    divoom_print("aj test!\n");
    divoom_system_free_lock(pdivoom_cloud_photo_ctrl_handle->lock_handle);
}


/*********************************************************************
  Function:        divoom_cloud_photo_ctrl_update_list
  Description:     更新播放资源列表，这个是增量更新
  Input:           pinfo
  Return:          ＮＯＮＥ
**********************************************************************/

void divoom_cloud_photo_ctrl_update_list(Pdivoom_device_cloud_photo_list_info pinfo)
{    
    char file_name[256];
    
    if(pdivoom_cloud_photo_ctrl_handle == NULL)
    {
        return;
    }

    divoom_cloud_photo_ctrl_open_handle();    
    
    divoom_system_get_lock(pdivoom_cloud_photo_ctrl_handle->lock_handle);
    divoom_print("aj test!: %d\n", pdivoom_cloud_photo_ctrl_handle->phandle->num);
    for(int i = 0; i  < pdivoom_cloud_photo_ctrl_handle->phandle->num; i++)
    {
        if(pdivoom_cloud_photo_ctrl_handle->phandle->plist[i].download_flag == 0)
        {
            divoom_database_manage_get_vision_photo_name(DIVOOM_CLOUD_PHOTO_TABLE_NAME,pdivoom_cloud_photo_ctrl_handle->phandle->plist[i].vision_id, file_name);
            divoom_download_aliyun_file_add_vision_item(pdivoom_cloud_photo_ctrl_handle->phandle->plist[i].vision_id, file_name, divoom_cloud_photo_ctrl_add_item_finish_callback);
        }
    }
    divoom_print("aj test!\n");
    divoom_system_free_lock(pdivoom_cloud_photo_ctrl_handle->lock_handle);
    divoom_cloud_photo_ctrl_close_handle();
    

    if(pinfo)
    {
        for(int i = 0; i < pinfo->number; i++)
        {
            if(pinfo->plist[i].del_flag)
            {
                divoom_cloud_photo_ctrl_del_item(pinfo->plist[i].id);
            }
            else
            {
                pinfo->plist[i].clock_id = pinfo->clock_id;
                divoom_cloud_photo_ctrl_add_item(&pinfo->plist[i]);
            }
            if(pdivoom_cloud_photo_ctrl_handle->last_update_time < pinfo->plist[i].update_time)
            {
                pdivoom_cloud_photo_ctrl_handle->last_update_time = pinfo->plist[i].update_time;
            }
        }
        if(pinfo->number == 50)
        {
            divoom_cloud_photo_ctrl_request_more_info(pinfo->clock_id);
        }
    }
    
}



/*********************************************************************
  Function:        divoom_cloud_photo_ctrl_open_handle
  Description:     打开播放资源
  Input:           ＮＯＮＥ
  Return:          ＮＯＮＥ
**********************************************************************/

void *divoom_cloud_photo_ctrl_open_handle(void)
{
    
    if(pdivoom_cloud_photo_ctrl_handle == NULL)
    {
        
        return NULL;
    }
    
    if(pdivoom_cloud_photo_ctrl_handle->phandle)
    {
        divoom_system_get_lock(pdivoom_cloud_photo_ctrl_handle->lock_handle);
        pdivoom_cloud_photo_ctrl_handle->use_times ++;
        divoom_system_free_lock(pdivoom_cloud_photo_ctrl_handle->lock_handle);
        
        return pdivoom_cloud_photo_ctrl_handle->phandle;
    }

    divoom_system_get_lock(pdivoom_cloud_photo_ctrl_handle->lock_handle);

    pdivoom_cloud_photo_ctrl_handle->phandle = divoom_system_malloc(sizeof(divoom_cloud_photo_ctrl_info));
    pdivoom_cloud_photo_ctrl_handle->phandle->plist = divoom_database_manage_get_vision_photo_list(DIVOOM_CLOUD_PHOTO_TABLE_NAME,&pdivoom_cloud_photo_ctrl_handle->phandle->num);
    pdivoom_cloud_photo_ctrl_handle->phandle->index = 0;
    pdivoom_cloud_photo_ctrl_handle->down_vision_id = pdivoom_cloud_photo_ctrl_handle->play_vision_id;
    for(int i = 0; i < pdivoom_cloud_photo_ctrl_handle->phandle->num; i++)
    {
        if(pdivoom_cloud_photo_ctrl_handle->phandle->plist[i].vision_id == pdivoom_cloud_photo_ctrl_handle->play_vision_id)
        {
            pdivoom_cloud_photo_ctrl_handle->phandle->index = i;
        }
    }
    pdivoom_cloud_photo_ctrl_handle->use_times = 1;
    divoom_system_free_lock(pdivoom_cloud_photo_ctrl_handle->lock_handle);
    
    return pdivoom_cloud_photo_ctrl_handle->phandle;
}



/*********************************************************************
  Function:        divoom_cloud_photo_ctrl_close_handle
  Description:     关闭播放资源
  Input:           ＮＯＮＥ
  Return:          ＮＯＮＥ
**********************************************************************/

void divoom_cloud_photo_ctrl_close_handle(void)
{
    if(pdivoom_cloud_photo_ctrl_handle == NULL)
    {
        return;
    }
    
    divoom_system_get_lock(pdivoom_cloud_photo_ctrl_handle->lock_handle);
    divoom_print("aj test!\n");
    pdivoom_cloud_photo_ctrl_handle->use_times --;
    if(pdivoom_cloud_photo_ctrl_handle->use_times == 0 && pdivoom_cloud_photo_ctrl_handle->phandle)
    {
        int *del_id;
        
        char file_name[256];
        
        divoom_print("aj test!\n");
        while(divoom_queue_items(pdivoom_cloud_photo_ctrl_handle->del_queue))
        {
            del_id = divoom_queue_pop(pdivoom_cloud_photo_ctrl_handle->del_queue);            
            sprintf(file_name, "%s/%d.webp", DIVOOM_CLOUD_PHOTO_INFO_DIR, *del_id);
            divoom_app_com_del_file(file_name);
            divoom_system_free(del_id);
        }
        divoom_system_free(pdivoom_cloud_photo_ctrl_handle->phandle->plist);
        divoom_system_free(pdivoom_cloud_photo_ctrl_handle->phandle);
        
        pdivoom_cloud_photo_ctrl_handle->phandle = NULL;
        
    }
    
    divoom_print("aj test!\n");
    divoom_system_free_lock(pdivoom_cloud_photo_ctrl_handle->lock_handle);
}


/*********************************************************************
  Function:        divoom_cloud_photo_ctrl_get_total
  Description:     得到要播放的文件数目
  Input:           ＮＯＮＥ
  Return:          文件数目
**********************************************************************/

int divoom_cloud_photo_ctrl_get_change_flag(void)
{
    if(pdivoom_cloud_photo_ctrl_handle == NULL)
    {
        return 0;
    }
    return divoom_queue_items(pdivoom_cloud_photo_ctrl_handle->del_queue);
}

/*********************************************************************
  Function:        divoom_cloud_photo_ctrl_get_total
  Description:     得到要播放的文件数目
  Input:           ＮＯＮＥ
  Return:          文件数目
**********************************************************************/

int divoom_cloud_photo_ctrl_get_total(void)
{
    int number = 0;
    
    
    if(pdivoom_cloud_photo_ctrl_handle == NULL)
    {
        
        return 0;
    }
    divoom_system_get_lock(pdivoom_cloud_photo_ctrl_handle->lock_handle);
    if(pdivoom_cloud_photo_ctrl_handle->phandle)
    {
        
        number = pdivoom_cloud_photo_ctrl_handle->phandle->num;
    }
    
    divoom_system_free_lock(pdivoom_cloud_photo_ctrl_handle->lock_handle);
    return number;
}

/*********************************************************************
  Function:        divoom_cloud_photo_ctrl_get_item
  Description:     得到要播放的文件名
  Input:           file_name
  Return:          1 or 0
**********************************************************************/

int divoom_cloud_photo_ctrl_get_item(char *file_name)
{
    int ret = 0;
    int *del_id;    
    
    if(pdivoom_cloud_photo_ctrl_handle == NULL)
    {
        return 0;
    }
    
    divoom_system_get_lock(pdivoom_cloud_photo_ctrl_handle->lock_handle);
    
        
    while(divoom_queue_items(pdivoom_cloud_photo_ctrl_handle->del_queue))
    {
        del_id = divoom_queue_pop(pdivoom_cloud_photo_ctrl_handle->del_queue);     
        if(del_id == NULL)
        {
            break;
        }
        sprintf(file_name, "%s/%d.webp", DIVOOM_CLOUD_PHOTO_INFO_DIR, *del_id);
        divoom_app_com_del_file(file_name);
        divoom_system_free(del_id);
    }
    
    for(int i = 0; i < pdivoom_cloud_photo_ctrl_handle->phandle->num; i++)
    {
        if(pdivoom_cloud_photo_ctrl_handle->phandle->index >= pdivoom_cloud_photo_ctrl_handle->phandle->num)
        {
            pdivoom_cloud_photo_ctrl_handle->phandle->index = 0;
        }
        if(pdivoom_cloud_photo_ctrl_handle->phandle->plist[pdivoom_cloud_photo_ctrl_handle->phandle->index].download_flag)
        {
            pdivoom_cloud_photo_ctrl_handle->play_vision_id = pdivoom_cloud_photo_ctrl_handle->phandle->plist[pdivoom_cloud_photo_ctrl_handle->phandle->index].vision_id;
            sprintf(file_name, "%s/%d.webp", DIVOOM_CLOUD_PHOTO_INFO_DIR, pdivoom_cloud_photo_ctrl_handle->play_vision_id);
            pdivoom_cloud_photo_ctrl_handle->phandle->index ++;
            ret = 1;
            break;            
        }
    }
    divoom_system_free_lock(pdivoom_cloud_photo_ctrl_handle->lock_handle);
    return pdivoom_cloud_photo_ctrl_handle->play_vision_id;
}


void divoom_cloud_photo_redownload_pic_id(int pic_id)
{
    char file_name[256];
    if(pdivoom_cloud_photo_ctrl_handle == NULL)
    {
        return ;
    }
    sprintf(file_name, "%s/%d.webp", DIVOOM_CLOUD_PHOTO_INFO_DIR, pic_id);
    divoom_app_com_del_file(file_name);
    divoom_database_manage_get_vision_photo_name(DIVOOM_CLOUD_PHOTO_TABLE_NAME, pic_id, file_name);
    divoom_download_aliyun_file_add_cloud_photo_item(pic_id, divoom_main_get_server_type(), file_name, NULL);

}


int divoom_cloud_photo_get_last_pic_id(void)
{
    int play_id;
    if(pdivoom_cloud_photo_ctrl_handle == NULL)
    {
        return 0;
    }
    if(pdivoom_cloud_photo_ctrl_handle->down_vision_id)
    {
        play_id = pdivoom_cloud_photo_ctrl_handle->down_vision_id;
        pdivoom_cloud_photo_ctrl_handle->down_vision_id = 0;
        return play_id;
    }
    else
    {
        return 0;
    }
}



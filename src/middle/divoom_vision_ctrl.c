#include "divoom.h"


//#define DIVOOM_VISEION_OPEN_SWITCH 

typedef struct _divoom_vision_ctrl_info
{
    int num;
    int index;
    Pdivoom_vision_item plist;
}divoom_vision_ctrl_info, *Pdivoom_vision_ctrl_info;


typedef struct _divoom_vision_ctrl_ctrl
{
    int play_vision_id;
    void *lock_handle;
    void *del_queue;
    int  use_times;
    Pdivoom_vision_ctrl_info phandle;
}divoom_vision_ctrl_ctrl, *Pdivoom_vision_ctrl_ctrl;


static Pdivoom_vision_ctrl_ctrl pdivoom_vision_ctrl_handle = NULL;

/*********************************************************************
  Function:        divoom_vision_ctrl_init
  Description:     初始化
  Input:           
                   
  Return:          ＮＯＮＥ
**********************************************************************/

void divoom_vision_ctrl_init(void)
{
    #ifdef DIVOOM_VISEION_OPEN_SWITCH
	char *table_item = "id INTEGER PRIMARY KEY     NOT NULL," \
		"download INTEGER DEFAULT 0," \
		"update_time INTEGER DEFAULT 0," \
		"file_name CHAR(200)";

	divoom_database_manage_add_new_table(DIVOOM_CUSTOM_VISION_TABLE_NAME, table_item);
    divoom_database_manage_add_table_index(DIVOOM_CUSTOM_VISION_TABLE_NAME, "update_time");
    pdivoom_vision_ctrl_handle = divoom_system_malloc(sizeof(divoom_vision_ctrl_ctrl));
    pdivoom_vision_ctrl_handle->lock_handle = divoom_system_create_lock();
    pdivoom_vision_ctrl_handle->del_queue = divoom_queue_init(10, __func__, __LINE__);
    #endif
}

void divoom_vision_ctrl_add_item_finish_callback(struct _divoom_download_aliyun_file_item *handle)
{
#ifdef DIVOOM_VISEION_OPEN_SWITCH
    divoom_database_manage_update_vision_photo_item_download(DIVOOM_CUSTOM_VISION_TABLE_NAME, handle->photo_id);
    divoom_system_get_lock(pdivoom_vision_ctrl_handle->lock_handle);
    if(pdivoom_vision_ctrl_handle->phandle)
    {
        for(int i = 0; i < pdivoom_vision_ctrl_handle->phandle->num; i++)
        {
            if(pdivoom_vision_ctrl_handle->phandle->plist[i].vision_id == handle->photo_id)
            {
                pdivoom_vision_ctrl_handle->phandle->plist[i].download_flag = 1;
                pdivoom_vision_ctrl_handle->play_vision_id = handle->photo_id;
                break;
            }
        }
    }
    divoom_system_free_lock(pdivoom_vision_ctrl_handle->lock_handle);
#endif
}

/*********************************************************************
  Function:        divoom_vision_ctrl_add_item
  Description:     增加一个元素
  Input:           vision_id
                   oss_name
  Return:          ＮＯＮＥ
**********************************************************************/

void divoom_vision_ctrl_add_item(int vision_id, char *oss_name, int update_time)
{    
#ifdef DIVOOM_VISEION_OPEN_SWITCH
    int i, find_flag = 0;
    char file_name[256];
    divoom_system_get_lock(pdivoom_vision_ctrl_handle->lock_handle);
    sprintf(file_name, "%s/%d.mp4", DIVOOM_VISION_INFO_DIR, vision_id);
    if(divoom_app_com_check_file_exist(file_name) == 0)
    {
        if(pdivoom_vision_ctrl_handle->phandle)
        {
            
            for(i = 0; i < pdivoom_vision_ctrl_handle->phandle->num; i++)
            {
                if(pdivoom_vision_ctrl_handle->phandle->plist[i].vision_id == vision_id)
                {
                    find_flag = 1;
                    break;
                }
            }
        }
        if(find_flag == 0)
        {
            
            if(pdivoom_vision_ctrl_handle->phandle)
            {
                Pdivoom_vision_item plist = divoom_system_malloc(sizeof(divoom_vision_item)*(pdivoom_vision_ctrl_handle->phandle->num + 1));

                memcpy(plist, pdivoom_vision_ctrl_handle->phandle->plist, sizeof(divoom_vision_item)*(pdivoom_vision_ctrl_handle->phandle->num ));
                plist[pdivoom_vision_ctrl_handle->phandle->num].vision_id = vision_id;
                plist[pdivoom_vision_ctrl_handle->phandle->num].download_flag = 0;            
                pdivoom_vision_ctrl_handle->phandle->num ++;
                divoom_system_free(pdivoom_vision_ctrl_handle->phandle->plist);
                pdivoom_vision_ctrl_handle->phandle->plist = plist;
            }
            divoom_database_manage_insert_vision_photo_item_with_time(DIVOOM_CUSTOM_VISION_TABLE_NAME,vision_id, oss_name, update_time);
            divoom_download_aliyun_file_add_vision_item(vision_id, oss_name, divoom_vision_ctrl_add_item_finish_callback);
        }
    }
    divoom_system_free_lock(pdivoom_vision_ctrl_handle->lock_handle);
    #endif
}


/*********************************************************************
  Function:        divoom_vision_ctrl_del_item
  Description:     删除一个元素
  Input:           vision_id
  Return:          ＮＯＮＥ
**********************************************************************/

void divoom_vision_ctrl_del_item(int vision_id)
{
#ifdef DIVOOM_VISEION_OPEN_SWITCH
    char file_name[256];
    
    divoom_system_get_lock(pdivoom_vision_ctrl_handle->lock_handle);
    divoom_database_manage_delete_vision_photo_item(DIVOOM_CUSTOM_VISION_TABLE_NAME,vision_id);
    sprintf(file_name, "%s/%d.mp4", DIVOOM_VISION_INFO_DIR, vision_id);
    if(pdivoom_vision_ctrl_handle->phandle)
    {
        if(vision_id == pdivoom_vision_ctrl_handle->play_vision_id)
        {
            int *del_id = divoom_system_malloc(sizeof(int));

            *del_id = vision_id;
            divoom_queue_push(pdivoom_vision_ctrl_handle->del_queue, del_id);
        }
		else
    	{
        	divoom_app_com_del_file(file_name);
   	 	}
        for(int i = 0; i < pdivoom_vision_ctrl_handle->phandle->num; i++)
        {
            if(pdivoom_vision_ctrl_handle->phandle->plist[i].vision_id == vision_id)
            {
                if(i != pdivoom_vision_ctrl_handle->phandle->num -1)
                {
                    memmove(&pdivoom_vision_ctrl_handle->phandle->plist[i], &pdivoom_vision_ctrl_handle->phandle->plist[i+1], (pdivoom_vision_ctrl_handle->phandle->num - (i+1)) * sizeof(divoom_vision_item));
                }
                pdivoom_vision_ctrl_handle->phandle->num --;
                break;
            }
        }
    }
    else
    {
        divoom_app_com_del_file (file_name);
    }
    divoom_system_free_lock(pdivoom_vision_ctrl_handle->lock_handle);
    #endif
}


/*********************************************************************
  Function:        divoom_vision_ctrl_update_list
  Description:     更新播放资源列表，这个是增量更新
  Input:           pinfo
  Return:          ＮＯＮＥ
**********************************************************************/

void divoom_vision_ctrl_update_list(Pdivoom_device_vision_update_info pinfo)
{    
#ifdef DIVOOM_VISEION_OPEN_SWITCH
    char file_name[256];
    

    divoom_vision_ctrl_open_handle();    
    
    divoom_system_get_lock(pdivoom_vision_ctrl_handle->lock_handle);
    for(int i = 0; i  < pdivoom_vision_ctrl_handle->phandle->num; i++)
    {
        if(pdivoom_vision_ctrl_handle->phandle->plist[i].download_flag == 0)
        {
            divoom_database_manage_get_vision_photo_name(DIVOOM_CUSTOM_VISION_TABLE_NAME,pdivoom_vision_ctrl_handle->phandle->plist[i].vision_id, file_name);
            divoom_download_aliyun_file_add_vision_item(pdivoom_vision_ctrl_handle->phandle->plist[i].vision_id, file_name, divoom_vision_ctrl_add_item_finish_callback);
        }
    }
    divoom_system_free_lock(pdivoom_vision_ctrl_handle->lock_handle);
    divoom_vision_ctrl_close_handle();
    

    if(pinfo)
    {
        for(int i = 0; i < pinfo->number; i++)
        {
            if(pinfo->plist[i].del_flag)
            {
                divoom_vision_ctrl_del_item(pinfo->plist[i].id);
            }
            else
            {
                divoom_vision_ctrl_add_item(pinfo->plist[i].id, pinfo->plist[i].oss_name, pinfo->plist[i].update_time);
            }
        }
    }
    #endif
    
}



/*********************************************************************
  Function:        divoom_vision_ctrl_open_handle
  Description:     打开播放资源
  Input:           ＮＯＮＥ
  Return:          ＮＯＮＥ
**********************************************************************/

void divoom_vision_ctrl_open_handle(void)
{
#ifdef DIVOOM_VISEION_OPEN_SWITCH
    if(pdivoom_vision_ctrl_handle->phandle)
    {
        divoom_system_get_lock(pdivoom_vision_ctrl_handle->lock_handle);
        pdivoom_vision_ctrl_handle->use_times ++;
        divoom_system_free_lock(pdivoom_vision_ctrl_handle->lock_handle);
        return;
    }

    divoom_system_get_lock(pdivoom_vision_ctrl_handle->lock_handle);

    pdivoom_vision_ctrl_handle->phandle = divoom_system_malloc(sizeof(divoom_vision_ctrl_info));
    pdivoom_vision_ctrl_handle->phandle->plist = divoom_database_manage_get_vision_photo_list(DIVOOM_CUSTOM_VISION_TABLE_NAME,&pdivoom_vision_ctrl_handle->phandle->num);
    pdivoom_vision_ctrl_handle->phandle->index = 0;
    for(int i = 0; i < pdivoom_vision_ctrl_handle->phandle->num; i++)
    {
        if(pdivoom_vision_ctrl_handle->phandle->plist[i].vision_id == pdivoom_vision_ctrl_handle->play_vision_id)
        {
            pdivoom_vision_ctrl_handle->phandle->index = i;
        }
    }
    pdivoom_vision_ctrl_handle->use_times = 1;
    divoom_system_free_lock(pdivoom_vision_ctrl_handle->lock_handle);
    #endif
    
}



/*********************************************************************
  Function:        divoom_vision_ctrl_close_handle
  Description:     关闭播放资源
  Input:           ＮＯＮＥ
  Return:          ＮＯＮＥ
**********************************************************************/

void divoom_vision_ctrl_close_handle(void)
{
#ifdef DIVOOM_VISEION_OPEN_SWITCH
    divoom_system_get_lock(pdivoom_vision_ctrl_handle->lock_handle);
    pdivoom_vision_ctrl_handle->use_times --;
    if(pdivoom_vision_ctrl_handle->use_times == 0 && pdivoom_vision_ctrl_handle->phandle)
    {
        int *del_id;
        
        char file_name[256];
        
        while(divoom_queue_items(pdivoom_vision_ctrl_handle->del_queue))
        {
            del_id = divoom_queue_pop(pdivoom_vision_ctrl_handle->del_queue);            
            sprintf(file_name, "%s/%d.mp4", DIVOOM_VISION_INFO_DIR, *del_id);
            divoom_app_com_del_file(file_name);
            divoom_system_free(del_id);
        }
        divoom_system_free(pdivoom_vision_ctrl_handle->phandle->plist);
        divoom_system_free(pdivoom_vision_ctrl_handle->phandle);
        pdivoom_vision_ctrl_handle->phandle = NULL;
        
    }
    
    divoom_system_free_lock(pdivoom_vision_ctrl_handle->lock_handle);
    #endif
}


/*********************************************************************
  Function:        divoom_vision_ctrl_get_total
  Description:     得到要播放的文件数目
  Input:           ＮＯＮＥ
  Return:          文件数目
**********************************************************************/

int divoom_vision_ctrl_get_change_flag(void)
{
#ifdef DIVOOM_VISEION_OPEN_SWITCH
    return divoom_queue_items(pdivoom_vision_ctrl_handle->del_queue);
    #else
    return 0;
    #endif
}

/*********************************************************************
  Function:        divoom_vision_ctrl_get_total
  Description:     得到要播放的文件数目
  Input:           ＮＯＮＥ
  Return:          文件数目
**********************************************************************/

int divoom_vision_ctrl_get_total(void)
{
#ifdef DIVOOM_VISEION_OPEN_SWITCH
    int number = 0;
    
    divoom_system_get_lock(pdivoom_vision_ctrl_handle->lock_handle);
    if(pdivoom_vision_ctrl_handle->phandle)
    {
        number = pdivoom_vision_ctrl_handle->phandle->num;
    }
    divoom_system_free_lock(pdivoom_vision_ctrl_handle->lock_handle);
    return number;
    #else
    return 0;
    #endif
}

/*********************************************************************
  Function:        divoom_vision_ctrl_get_item
  Description:     得到要播放的文件名
  Input:           file_name
  Return:          1 or 0
**********************************************************************/

int divoom_vision_ctrl_get_item(char *file_name)
{
#ifdef DIVOOM_VISEION_OPEN_SWITCH
    int ret = 0;
    int *del_id;    
    
    divoom_system_get_lock(pdivoom_vision_ctrl_handle->lock_handle);
    
        
    while(divoom_queue_items(pdivoom_vision_ctrl_handle->del_queue))
    {
        del_id = divoom_queue_pop(pdivoom_vision_ctrl_handle->del_queue);            
        sprintf(file_name, "%s/%d.mp4", DIVOOM_VISION_INFO_DIR, *del_id);
        divoom_app_com_del_file(file_name);
        divoom_system_free(del_id);
    }
    
    for(int i = 0; i < pdivoom_vision_ctrl_handle->phandle->num; i++)
    {
        if(pdivoom_vision_ctrl_handle->phandle->index >= pdivoom_vision_ctrl_handle->phandle->num)
        {
            pdivoom_vision_ctrl_handle->phandle->index = 0;
        }
        if(pdivoom_vision_ctrl_handle->phandle->plist[pdivoom_vision_ctrl_handle->phandle->index].download_flag)
        {
            pdivoom_vision_ctrl_handle->play_vision_id = pdivoom_vision_ctrl_handle->phandle->plist[pdivoom_vision_ctrl_handle->phandle->index].vision_id;
            sprintf(file_name, "%s/%d.mp4", DIVOOM_VISION_INFO_DIR, pdivoom_vision_ctrl_handle->play_vision_id);
            pdivoom_vision_ctrl_handle->phandle->index ++;
            ret = 1;
            break;            
        }
    }
    divoom_system_free_lock(pdivoom_vision_ctrl_handle->lock_handle);
    return ret;
    #else
    return 0;
    #endif
}




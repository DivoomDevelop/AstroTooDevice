#include "divoom.h"

typedef struct _divoom_user_head_manage_item
{
    uint32 user_id;
    char user_name[200];
    char file_id[64];
}divoom_user_head_manage_item, *Pdivoom_user_head_manage_item;

#define DIVOOM_USER_HEAD_MANAGE_DOWNLOAD_NUM  20
typedef struct _divoom_user_head_manage_ctrl
{
    int   last_request_time;
    int   user_num;
    int   user_list[DIVOOM_USER_HEAD_MANAGE_DOWNLOAD_NUM];  //本地不存在的用户数据，缓冲到一定数目再请求
    void *user_queue;
}divoom_user_head_manage_ctrl, *Pdivoom_user_head_manage_ctrl;

#define DIVOOM_USER_HEAD_MANAGE_CACHE_NUM  50
#define DIVOOM_USER_HEAD_MANAGE_UPDATE_TIME  3600

Pdivoom_user_head_manage_ctrl pdivoom_user_head_manage_handle = NULL;

void divoom_user_head_manage_request_update_list(void);

/*********************************************************************
  Function:        divoom_user_head_manage_init
  Description:     初始化模块数据
  Input:           
                   	
                    
  Return:           none
**********************************************************************/

void divoom_user_head_manage_init(void)
{
    
	char *table_item = "user_id INT PRIMARY KEY     NOT NULL," \
		"user_name CHAR(200)," \
		"file_id CHAR(64)," \
		"update_time INTEGER KEY DEFAULT 0";

	divoom_database_manage_add_new_table(DIVOOM_USER_HEAD_IMAGE_TABLE_NAME, table_item);
    divoom_database_manage_add_table_index(DIVOOM_USER_HEAD_IMAGE_TABLE_NAME, "update_time");
    pdivoom_user_head_manage_handle = divoom_system_malloc(sizeof(divoom_user_head_manage_ctrl));
    pdivoom_user_head_manage_handle->user_queue = divoom_queue_init(DIVOOM_USER_HEAD_MANAGE_CACHE_NUM, __func__, __LINE__);
}

int divoom_user_head_manage_compare_user(void *item1, void *item2)
{
    uint32 *user_id = (uint32 *)item1;
    Pdivoom_user_head_manage_item pcur_item = (Pdivoom_user_head_manage_item)item2;

    return *user_id == pcur_item->user_id?0:1;
}

/*********************************************************************
  Function:        divoom_user_head_manage_update_info
  Description:     更新用户数据，如果头像更新，将重新下载数据
  Input:           user_id
                   user_name
                   file_id	
                    
  Return:           none
**********************************************************************/

int divoom_user_head_manage_update_info(uint32 user_id, char *user_name, char *file_id)
{
    int insert_flag = 0, file_id_change_flag = 0;
    Pdivoom_user_head_manage_item pcur_item;

    if(pdivoom_user_head_manage_handle == NULL)
    {
        return 0;
    }
    pcur_item = divoom_queue_pop_cmp(pdivoom_user_head_manage_handle->user_queue, &user_id, divoom_user_head_manage_compare_user);
    if(pcur_item == NULL)
    {
        if(divoom_queue_is_full(pdivoom_user_head_manage_handle->user_queue))
        {
            pcur_item = divoom_queue_pop(pdivoom_user_head_manage_handle->user_queue);
        }
        else
        {
            pcur_item = divoom_system_malloc(sizeof(divoom_user_head_manage_item));
        }
        pcur_item->user_id = user_id;
        if(divoom_database_manage_get_user_head_item(user_id, pcur_item->user_name, pcur_item->file_id))
        {
        }
        else
        {
            if(pdivoom_user_head_manage_handle->user_num < DIVOOM_USER_HEAD_MANAGE_DOWNLOAD_NUM && user_id)
            {
                pdivoom_user_head_manage_handle->user_list[pdivoom_user_head_manage_handle->user_num] = user_id;
                pdivoom_user_head_manage_handle->user_num ++;
            }
            if(pdivoom_user_head_manage_handle->user_num >= DIVOOM_USER_HEAD_MANAGE_DOWNLOAD_NUM)
            {
                divoom_device_get_user_info_list_request_info puser_info;
                
                
                puser_info.user_num = DIVOOM_USER_HEAD_MANAGE_DOWNLOAD_NUM;
                puser_info.user_list = pdivoom_user_head_manage_handle->user_list;
                divoom_net_app_request_user_info_list(
                    DIVOOM_NET_APP_USER_INFO_SOURCE_USER_HEAD, &puser_info);
                pdivoom_user_head_manage_handle->user_num = 0;
            }
            pcur_item->user_name[0] = 0;
            pcur_item->file_id[0] = 0;
            insert_flag = 1;
        }
    }
    if(strcmp(user_name, pcur_item->user_name) || strcmp(file_id, pcur_item->file_id))
    {
        if(strcmp(file_id, pcur_item->file_id) && strlen(pcur_item->file_id))
        {
            divoom_manage_image_item_info pimage_info = {0};
            divoom_manage_image_del_direct(DIVOOM_MANAGE_IMAGE_USER_HEAD_IMAGE_FILE, &user_id, 1);
            pimage_info.image_id = user_id;
            strcpy(pimage_info.file_addr, file_id);
            divoom_manage_image_add(DIVOOM_MANAGE_IMAGE_USER_HEAD_IMAGE_FILE, &pimage_info, 1);
            file_id_change_flag = 1;
        }
        strcpy(pcur_item->user_name, user_name);
        strcpy(pcur_item->file_id, file_id);
        //更新数据库
        if(insert_flag)
        {
            divoom_database_manage_insert_user_head_item(user_id, user_name, file_id);
        }
        else
        {
            divoom_database_manage_update_user_head_item(user_id, user_name, file_id);
        }
    }
    divoom_queue_push(pdivoom_user_head_manage_handle->user_queue, pcur_item);
    return file_id_change_flag;
}

void divoom_user_head_manage_get_user_info_from_server(uint32 user_id, char *user_name, char *file_id)
{
    //请求服务器的数据，并更新
    int i;
    divoom_device_get_user_info_list_request_info puser_info;
    Pdivoom_net_command ret_data;
    Pdivoom_user_head_manage_item pcur_item;

    if(user_id == 0)
    {
        user_id = divoom_main_get_host_id();
    }

    for(i = 0; i < pdivoom_user_head_manage_handle->user_num; i++)
    {
        if(pdivoom_user_head_manage_handle->user_list[i] == user_id)
        {
            break;
        }
    }
    if(i >= pdivoom_user_head_manage_handle->user_num && user_id)
    {
        if(pdivoom_user_head_manage_handle->user_num >= DIVOOM_USER_HEAD_MANAGE_DOWNLOAD_NUM)
        {
            pdivoom_user_head_manage_handle->user_list[0] = user_id;
        }
        else
        {
            pdivoom_user_head_manage_handle->user_list[i] = user_id;
            pdivoom_user_head_manage_handle->user_num++;
        }
    }
    puser_info.user_num = pdivoom_user_head_manage_handle->user_num;
    puser_info.user_list = pdivoom_user_head_manage_handle->user_list;
    ret_data = divoom_net_app_send_packet_wait(DIVOOM_NET_COMM_GET_USER_INFO_LIST, DIVOOM_SERVER_TOPIC, &puser_info);
    if(ret_data)
    {
        Pdivoom_device_get_user_info_list_respone_info pinfo = (Pdivoom_device_get_user_info_list_respone_info)ret_data->cmd_data;

        if(pinfo)
        {
            
            for(i = 0; i < pinfo->user_num; i++)
            {
                divoom_manage_image_item_info pimage_info = {0};
                

                if(pinfo->puser_list[i].user_id == user_id)
                {
                    
                    strcpy(user_name, pinfo->puser_list[i].user_name);
                    strcpy(file_id, pinfo->puser_list[i].file_id);
                }
    
                pimage_info.image_id = pinfo->puser_list[i].user_id;
                strcpy(pimage_info.file_addr, pinfo->puser_list[i].file_id);
                divoom_manage_image_add(DIVOOM_MANAGE_IMAGE_USER_HEAD_IMAGE_FILE, &pimage_info, 1);
                pcur_item = divoom_queue_pop_cmp(pdivoom_user_head_manage_handle->user_queue, &pinfo->puser_list[i].user_id, divoom_user_head_manage_compare_user);
                if(pcur_item)
                {
                    
                    strcpy(pcur_item->user_name, pinfo->puser_list[i].user_name);
                    strcpy(pcur_item->file_id, pinfo->puser_list[i].file_id);
                }
                divoom_database_manage_update_user_head_item(user_id, pinfo->puser_list[i].user_name, pinfo->puser_list[i].file_id);
            }
        }
        divoom_net_app_free_packet(ret_data);
    
    }
}

/*********************************************************************
  Function:        divoom_user_head_manage_get_info
  Description:     获取用户的用户名
  Input:           user_id
                   user_name
                    
  Return:           none
**********************************************************************/

void divoom_user_head_manage_get_info(uint32 user_id, char *user_name, char *file_id)
{
    Pdivoom_user_head_manage_item pcur_item;

    pcur_item = divoom_queue_pop_cmp(pdivoom_user_head_manage_handle->user_queue, &user_id, divoom_user_head_manage_compare_user);
    if(pcur_item == NULL)
    {
        
        if(divoom_queue_is_full(pdivoom_user_head_manage_handle->user_queue))
        {
            pcur_item = divoom_queue_pop(pdivoom_user_head_manage_handle->user_queue);
        }
        else
        {
            pcur_item = divoom_system_malloc(sizeof(divoom_user_head_manage_item));
        }
        pcur_item->user_id = user_id;
        if(divoom_database_manage_get_user_head_item(user_id, pcur_item->user_name, pcur_item->file_id))
        {
        }
        else
        {
            //请求服务器的数据，并更新
            
            divoom_user_head_manage_get_user_info_from_server(user_id, pcur_item->user_name, pcur_item->file_id);
            divoom_database_manage_insert_user_head_item(user_id, pcur_item->user_name, pcur_item->file_id);
        }
    }
    
    divoom_queue_push(pdivoom_user_head_manage_handle->user_queue, pcur_item);
    if(strlen(pcur_item->file_id) == 0)
    {
        divoom_user_head_manage_get_user_info_from_server(user_id, pcur_item->user_name, pcur_item->file_id);
    }
    if(user_name)
    {
        strcpy(user_name, pcur_item->user_name);
    }
    if(file_id)
    {
        strcpy(file_id, pcur_item->file_id);
    }
    divoom_database_manage_update_user_head_time(user_id);
    divoom_user_head_manage_request_update_list();
    
}

/*********************************************************************
  Function:        divoom_user_head_manage_update_list
  Description:     更新用户列表信息
  Input:           plist
                    
  Return:           none
**********************************************************************/

void divoom_user_head_manage_update_list(Pdivoom_device_get_user_info_list_respone_info plist)
{
    if(plist == NULL)
    {
        return;
    }
    for(int i = 0; i < plist->user_num; i++)
    {
        divoom_user_head_manage_update_info(plist->puser_list[i].user_id, plist->puser_list[i].user_name, plist->puser_list[i].file_id);
    }
}

/*********************************************************************
  Function:        divoom_user_head_manage_request_update_list
  Description:     在获取用户信息时更新一次
  Input:           none
                    
  Return:           none
**********************************************************************/


void divoom_user_head_manage_request_update_list(void)
{
    int user_num;
    uint32 *user_list;
    divoom_device_get_user_info_list_request_info puser_info;

    if(pdivoom_user_head_manage_handle == NULL || pdivoom_user_head_manage_handle->last_request_time > time(0))
    {
        return;
    }
    user_list = divoom_database_manage_get_user_head_list(&user_num);
    if(user_list == NULL)
    {
        return;
    }
    
    puser_info.user_num = user_num;
    puser_info.user_list = user_list;
    divoom_net_app_request_user_info_list(
        DIVOOM_NET_APP_USER_INFO_SOURCE_USER_HEAD, &puser_info);
    pdivoom_user_head_manage_handle->last_request_time = DIVOOM_USER_HEAD_MANAGE_UPDATE_TIME + time(0);

    divoom_system_free(user_list);
}


/*********************************************************************
  Function:        divoom_user_head_manage_get_url_addr
  Description:     获取用户头像的相关数据
  Input:           user_id
                   url_addr 
  Return:           none
**********************************************************************/

void divoom_user_head_manage_get_url_addr(uint32 user_id,char * url_addr)
{
    
    divoom_user_head_manage_get_info(user_id, NULL, url_addr);
}


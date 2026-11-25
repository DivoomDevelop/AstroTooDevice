#include "divoom.h"

typedef struct _divoom_app_pic_manage_ctrl
{
    int num;
    Pdivoom_app_pic_manage_item plist;
}divoom_app_pic_manage_ctrl, *Pdivoom_app_pic_manage_ctrl;


Pdivoom_app_pic_manage_ctrl pdivoom_app_pic_manage_handle = NULL;

void divoom_app_pic_manage_finishcallback(int ret)
{
}


void divoom_app_pic_manage_load(void)
{
    char *json_info;
	divoom_cJSON *root = NULL, *list,*item;
    Pdivoom_app_pic_manage_item plist = NULL;
    divoom_manage_image_item_info pimage_info;
    
    
    if(pdivoom_app_pic_manage_handle == NULL)
    {
        return;
    }
    
    json_info = divoom_app_com_read_file(DIVOOM_ALL_APP_PIC_SAVE_PATH, NULL);
    if(json_info == NULL)
    {
        goto divoom_app_pic_manage_load_err_table;
    }
	root = divoom_cJSON_Parse(json_info);
    divoom_system_free(json_info);
    if(root == NULL)
    {
        goto divoom_app_pic_manage_load_err_table;
    }
    list = divoom_cJSON_GetObjectItem(root, "PicList");
    if(list == NULL)
    {
        divoom_cJSON_Delete(root);
        goto divoom_app_pic_manage_load_err_table;
    }
    pdivoom_app_pic_manage_handle->num = divoom_cJSON_GetArraySize(list);
    if(pdivoom_app_pic_manage_handle->num)
    {
        pdivoom_app_pic_manage_handle->plist = divoom_system_malloc(sizeof(divoom_app_pic_manage_item) * pdivoom_app_pic_manage_handle->num);
        plist = pdivoom_app_pic_manage_handle->plist;
        for(int i = 0; i < pdivoom_app_pic_manage_handle->num; i ++)
        {
            item = divoom_cJSON_GetArrayItem(list, i);
            if(item == NULL)
            {
                break;
            }
            plist[i].app_id = divoom_cJSON_GetObjectInt(item, "app_id");
            divoom_cJSON_GetObjectString(item, "addr", plist[i].addr);
            strcpy(pimage_info.file_addr, plist[i].addr);
            pimage_info.flag = 1;
            pimage_info.image_id = DIVOOM_MANAGE_IMAGE_ID_BASE_APP_PIC + plist[i].app_id;
            pimage_info.pcallback = divoom_app_pic_manage_finishcallback;      //不关心ＦＯＮＴ，只是提高优先级
            //divoom_print("add info: %d, %s\n", pimage_info.image_id, pimage_info.file_addr);
            if(strlen(pimage_info.file_addr))
            {
                divoom_manage_image_add(DIVOOM_MANAGE_IMAGE_CLOCK_DIAL_APP_PIC, &pimage_info, 1);
            }
            
        }
    }
    
    divoom_cJSON_Delete(root);
    return ;
    
    divoom_app_pic_manage_load_err_table:
    divoom_app_com_del_file(DIVOOM_ALL_APP_PIC_SAVE_PATH);
    divoom_system_free(plist);
    pdivoom_app_pic_manage_handle->num = 0;
    
}

/*********************************************************************
  Function:        divoom_app_pic_manage_get_url_addr
  Description:     获取指定文件的网络地址
  Input:           model_id
                   image_id
                   url_addr
                    
  Return:          image_flag
**********************************************************************/

int divoom_app_pic_manage_get_url_addr(int model_id, int image_id, char *url_addr)
{
    int i, app_id = image_id - DIVOOM_MANAGE_IMAGE_ID_BASE_APP_PIC;

	if(pdivoom_app_pic_manage_handle == NULL)
    {
        return 0;
    }
    
    for(i = 0; i < pdivoom_app_pic_manage_handle->num; i ++)
    {
        if(app_id != pdivoom_app_pic_manage_handle->plist[i].app_id)
        {
            strcpy(url_addr, pdivoom_app_pic_manage_handle->plist[i].addr);
        }
    }

    return 1;
}


void divoom_app_pic_manage_save_local(void)
{
    Pdivoom_app_pic_manage_item plist;
    char *json_string;;
    divoom_cJSON *root, *item, *list;
     
    
    if(pdivoom_app_pic_manage_handle == NULL)
    {
        return;
    }
    
    root = divoom_cJSON_CreateObject();
    if(root == NULL)
    {
        return;
    }
    plist = pdivoom_app_pic_manage_handle->plist;
    
 	divoom_cJSON_AddItemToObject(root, "PicList", list = divoom_cJSON_CreateArray());
    for(int i= 0; i <pdivoom_app_pic_manage_handle->num; i++)
    {
		divoom_cJSON_AddItemToArray(list, item = divoom_cJSON_CreateObject());        
        divoom_cJSON_AddNumberToObject(item , "app_id", plist[i].app_id);
        divoom_cJSON_AddStringToObject(item , "addr", plist[i].addr);        
    }
    json_string = divoom_cJSON_Print(root);
    divoom_cJSON_Delete(root);
    divoom_app_com_write_file(DIVOOM_ALL_APP_PIC_SAVE_PATH,json_string, strlen(json_string));
    divoom_system_free(json_string);
   
}

void divoom_app_pic_manage_save(void)
{
    divoom_app_pic_manage_save_local();
}

/*********************************************************************
  Function:        divoom_app_pic_manage_init
  Description:     初始化
  Input:           none
  Return:          none
**********************************************************************/

void divoom_app_pic_manage_init(void)
{
    if(pdivoom_app_pic_manage_handle)
    {
        return;
    }

    pdivoom_app_pic_manage_handle = divoom_system_malloc(sizeof(divoom_app_pic_manage_ctrl));
    divoom_app_pic_manage_load();
}



/*********************************************************************
  Function:        divoom_app_pic_manage_update
  Description:     更新app列表
  Input:           plist
                   num
  Return:          none
**********************************************************************/

void divoom_app_pic_manage_update(Pdivoom_app_pic_manage_item plist, int num)
{
    int i;
    divoom_manage_image_item_info pimage_info;
    Pdivoom_app_pic_manage_item pnew_list;
    
    if(pdivoom_app_pic_manage_handle == NULL)
    {
        return;
    }
    if(num == pdivoom_app_pic_manage_handle->num)
    {
        for(i = 0; i < num; i ++)
        {
            if(plist[i].app_id != pdivoom_app_pic_manage_handle->plist[i].app_id ||
                strcmp(plist[i].addr, pdivoom_app_pic_manage_handle->plist[i].addr))
            {
                break;
            }
        }
        if(i == num)
        {
            divoom_print("no update!\n");
            return ;
        }
    }
    divoom_print("update!: %d\n",pdivoom_app_pic_manage_handle->num);
    for(i = 0; i < num; i ++)
    {
        strcpy(pimage_info.file_addr, plist[i].addr);
        pimage_info.flag = 1;
        pimage_info.image_id = DIVOOM_MANAGE_IMAGE_ID_BASE_APP_PIC + plist[i].app_id;
        pimage_info.pcallback = divoom_app_pic_manage_finishcallback;      //不关心ＦＯＮＴ，只是提高优先级
        divoom_manage_image_add(DIVOOM_MANAGE_IMAGE_CLOCK_DIAL_APP_PIC, &pimage_info, 1);
        
    }
    
    pnew_list = divoom_system_malloc(sizeof(divoom_app_pic_manage_item)*num);
    memcpy(pnew_list, plist, sizeof(divoom_app_pic_manage_item)*num);
    divoom_system_free(pdivoom_app_pic_manage_handle->plist);
    pdivoom_app_pic_manage_handle->plist = pnew_list;
    pdivoom_app_pic_manage_handle->num = num;
    divoom_app_pic_manage_save();

}

/*********************************************************************
  Function:        divoom_app_pic_manage_use_app
  Description:     加载app图片
  Input:           app_id
  Return:          none
**********************************************************************/

void divoom_app_pic_manage_use_app(int app_id)
{
    int i;
    Pdivoom_net_command ret_data;
    
    if(pdivoom_app_pic_manage_handle == NULL)
    {
        return;
    }

    for(i = 0; i < pdivoom_app_pic_manage_handle->num; i ++)
    {
        if(app_id == pdivoom_app_pic_manage_handle->plist[i].app_id)
        {
            return ;
        }
        
    }
    //将重新加载所有信息
    ret_data = divoom_net_app_send_packet_wait(DIVOOM_NET_COMM_DEVICE_GET_TIME_DIAL_APP_PIC, DIVOOM_SERVER_TOPIC,NULL);
    if(ret_data && ret_data->cmd_data)
    {
        Pdivoom_devcice_time_dial_app_pic_respone plist = (Pdivoom_devcice_time_dial_app_pic_respone)ret_data->cmd_data;
        
        divoom_app_pic_manage_update(plist->plist, plist->num);
        
    }
    divoom_net_app_free_packet(ret_data);

}


#include "divoom.h"
#include "divoom_oss_rest.h"
#include "divoom_photo_album_josn.h"

void *divoom_download_aliyun_create_thumb_pic_info(void *para);

static volatile int g_divoom_download_aliyun_file_stop;

int divoom_download_aliyun_file_get_file_direct(char *object_name, char *local_filename, int server_flag)
{
    return divoom_oss_rest_get_object_to_file(object_name, local_filename, server_flag);
}
int divoom_download_aliyun_file_get_file(char *object_name, char *local_filename, int server_flag)
{
    for(int i = 0; i < 3; i++)
    {
        if(g_divoom_download_aliyun_file_stop)
        {
            return 0;
        }
        if(divoom_download_aliyun_file_get_file_direct(object_name, local_filename, server_flag))
        {
            return 1;
        }
        if(divoom_main_get_wifi_con_status() == 0)
        {
            return 0;
        }
        if(i < 2)
        {
            divoom_port_delay_ms((uint32)(300 * (i + 1)));
        }
    }
    return 0;
    
}

int divoom_download_aliyun_file_get_file_length(char *object_name, int server_flag)
{
    return divoom_oss_rest_head_object_length(object_name, server_flag);
}

typedef struct _divoom_download_aliyun_file_ctrl
{
    int run_flag;
    int load_finish_flag;
    int cur_id;
    int total_num;
    int photo_flag;
    int last_album_id;
    int last_pic_id;
    int downloading_flag;
    volatile int task_active;
    void *wait_sem;
    void *file_queue;
    void *local_file_queue;
    void *local_file_sem;
}divoom_download_aliyun_file_ctrl, *Pdivoom_download_aliyun_file_ctrl;

static Pdivoom_download_aliyun_file_ctrl pdivoom_download_aliyun_file_handle = NULL;

void divoom_download_aliyun_file_task(void *user_data)
{
    char file_name[128];
    int  pic_list[64], pic_num = 0, cur_album_id = 0, pic_id;
    int download_result;
    time_t cur_time;
	struct tm time_now; 
    Pdivoom_download_aliyun_file_item pcur_item;
    divoom_photo_album_get_local_photo_request_item pdel_info;
        
    if(pdivoom_download_aliyun_file_handle == NULL ||
       g_divoom_download_aliyun_file_stop ||
       divoom_queue_items(pdivoom_download_aliyun_file_handle->file_queue) == 0 ||
       divoom_main_get_wifi_con_status() == 0)
    {
        return;
    }

    if(divoom_update_system_file_is_downloading())
    {
        return;
    }
    /* 阿里云任务每次只处理一个队列项，下载前最多回收一个最早移除的表盘。 */
    divoom_print("[storage_gc] call source=%s line=%d\n", __func__, __LINE__);
    divoom_clock_manage_clear_removed_clock_data_if_needed();
    pdivoom_download_aliyun_file_handle->downloading_flag = 1;
    pcur_item = divoom_queue_pop(pdivoom_download_aliyun_file_handle->file_queue);
    if(pcur_item == NULL)
    {
        return;
    }
    pdivoom_download_aliyun_file_handle->task_active = 1;
    
    cur_time = time(0);        
    localtime_r(&cur_time, &time_now);
    time_now.tm_year += 1900;
    time_now.tm_mon ++;
    if(pcur_item->no_del_flag == 1)
    {
        sprintf(file_name, "%s/%d.mp4",DIVOOM_VISION_INFO_DIR,pcur_item->photo_id);
    }
    else if(pcur_item->no_del_flag == 2)
    {
        sprintf(file_name, "%s/%d.webp",DIVOOM_CLOUD_PHOTO_INFO_DIR,pcur_item->photo_id);
    }
    else
    {
        sprintf(file_name, "%s/%d%d",DIVOOM_SD_APP_PIC_DIR,time_now.tm_year,time_now.tm_mon);
        divoom_app_com_mk_dir(file_name);
        strcat(file_name, "/");
        strcat(file_name, pcur_item->os_name);
        if(strlen(pcur_item->preview_os_name))
        {
            //有预览图的情况
            int len = strlen(file_name);
            
            file_name[len - 3] = 'w';
            file_name[len - 2] = 'e';
            file_name[len - 1] = 'b';
            file_name[len] = 'p';
            file_name[len + 1] = 0;
            divoom_download_aliyun_file_get_file(pcur_item->preview_os_name, file_name, pcur_item->server_flag);
            if(g_divoom_download_aliyun_file_stop)
            {
                divoom_system_free(pcur_item);
                pdivoom_download_aliyun_file_handle->downloading_flag = 0;
                pdivoom_download_aliyun_file_handle->task_active = 0;
                return;
            }
            {
                pdel_info.photo_id = pcur_item->photo_id;
                pdel_info.server_flag = pcur_item->server_flag;
                strcpy(pdel_info.big_name, pcur_item->preview_os_name);
                divoom_net_app_send_packet(DIVOOM_NET_COMM_DELETE_ALI_PHOTO, DIVOOM_SERVER_TOPIC,&pdel_info);
                divoom_print("we will finish preview file: %s, %s !\n", file_name, pcur_item->preview_os_name);
            }
            //还原文件名
            file_name[len - 3] = 'm';
            file_name[len - 2] = 'p';
            file_name[len - 1] = '4';
            file_name[len] = 0;
        }
    }
    download_result = divoom_download_aliyun_file_get_file(
        pcur_item->os_name, file_name, pcur_item->server_flag);
    if(g_divoom_download_aliyun_file_stop)
    {
        divoom_system_free(pcur_item);
        pdivoom_download_aliyun_file_handle->downloading_flag = 0;
        pdivoom_download_aliyun_file_handle->task_active = 0;
        return;
    }
    if(download_result)
    {
        //下载成功后删除文件
        divoom_print("we will finish the file: %d, %d, %s %ld!\n", pcur_item->album_id, pcur_item->photo_id , pcur_item->os_name,  pcur_item->photo_time);
        if(pcur_item->no_del_flag)
        {
            if(pcur_item->pcallback)
            {
                pcur_item->pcallback(pcur_item);
            }
            divoom_system_free(pcur_item);
        }
        else
        {
            pdel_info.photo_id = pcur_item->photo_id;
            pdel_info.server_flag = pcur_item->server_flag;
            strcpy(pdel_info.big_name, pcur_item->os_name);
            pdivoom_download_aliyun_file_handle->cur_id ++;
            divoom_net_app_send_packet(DIVOOM_NET_COMM_DELETE_ALI_PHOTO, DIVOOM_SERVER_TOPIC,&pdel_info);
            if(pcur_item->album_id >= DIVOOM_PHOTO_START_CLOCK_ID)
            {
                pic_id = divoom_pic_phone_manage_add_picture(file_name, pcur_item->save_time, pcur_item->photo_time , pcur_item->photo_x,pcur_item->photo_y, pcur_item->album_id, pcur_item->user_id, pcur_item->photo_title, pcur_item->photo_w, pcur_item->photo_h, divoom_pic_album_manage_check_album_cover(pcur_item->album_id)?0:1);
                if(pcur_item->pcallback)
                {
                    pcur_item->pcallback(pcur_item);
                }
                divoom_pic_album_manage_add_list_to_name(pcur_item->album_id, &pic_id, 1);
                pdivoom_download_aliyun_file_handle->last_album_id   = pcur_item->album_id;
                pdivoom_download_aliyun_file_handle->last_pic_id   = pic_id;
            }
            else
            {
                divoom_clock_id_info clock_info;
                
                clock_info.clock_id = pcur_item->album_id;
                clock_info.clock_pos = pcur_item->clock_pos;
                clock_info.parent_id = pcur_item->parent_id;
                pic_id = divoom_custom_photo_add_item(&clock_info, pcur_item->save_time, pcur_item->photo_time, file_name);
                pdivoom_download_aliyun_file_handle->last_album_id   = pcur_item->album_id;
                pdivoom_download_aliyun_file_handle->last_pic_id   = pic_id;
            }
            divoom_queue_push(pdivoom_download_aliyun_file_handle->local_file_queue, divoom_app_com_copy_string(file_name, -1));
            divoom_system_put_sem(pdivoom_download_aliyun_file_handle->local_file_sem);
            divoom_system_free(pcur_item);
            if(divoom_queue_items(pdivoom_download_aliyun_file_handle->file_queue) == 0)
            {
                //下载完了，将获取更多
                if(pdivoom_download_aliyun_file_handle->load_finish_flag == 0)
                {
                    divoom_net_app_send_packet(DIVOOM_NET_COMM_GET_ALI_PHOTO_LIST, DIVOOM_SERVER_TOPIC,NULL);
                }
            }
        }
    }
    else
    {        
        if(divoom_main_get_wifi_con_status())
        {
            //如果下载超过十次失败就认为这个文件有问题
            if(pcur_item->no_del_flag == 0)
            {
                divoom_photo_album_get_local_photo_request_item pdel_info;
                //下载成功后删除文件
                pdel_info.server_flag = pcur_item->server_flag;
                pdel_info.photo_id = pcur_item->photo_id;
                strcpy(pdel_info.big_name, pcur_item->os_name);
                pdivoom_download_aliyun_file_handle->cur_id ++;
                divoom_net_app_send_packet(DIVOOM_NET_COMM_DELETE_ALI_PHOTO, DIVOOM_SERVER_TOPIC,&pdel_info);
                divoom_print("we will delete the file: %d, %s beacuse it fails 10 times\n", pcur_item->photo_id , pcur_item->os_name);
            }
            divoom_system_free(pcur_item);
        }
        else
        {
            divoom_queue_push(pdivoom_download_aliyun_file_handle->file_queue, pcur_item);
        }
    }
    pdivoom_download_aliyun_file_handle->task_active = 0;
    if(divoom_queue_items(pdivoom_download_aliyun_file_handle->file_queue) == 0 || divoom_main_get_wifi_con_status() == 0)
    {
        pdivoom_download_aliyun_file_handle->cur_id = pdivoom_download_aliyun_file_handle->total_num;
        pdivoom_download_aliyun_file_handle->downloading_flag = 0;
        return;
    }
    return ;
}

/*********************************************************************
  Function:        divoom_download_aliyun_file_init
  Description:     初始化
  Input:           none
  Return:          none
**********************************************************************/

void divoom_download_aliyun_file_init(void)
{
    pdivoom_download_aliyun_file_handle = divoom_system_malloc(sizeof(divoom_download_aliyun_file_ctrl));

    g_divoom_download_aliyun_file_stop = 0;
    pdivoom_download_aliyun_file_handle->run_flag = 1;
    pdivoom_download_aliyun_file_handle->task_active = 0;
    pdivoom_download_aliyun_file_handle->wait_sem = divoom_system_create_sem(1, 0);
    pdivoom_download_aliyun_file_handle->local_file_sem = divoom_system_create_sem(1, 0);
    pdivoom_download_aliyun_file_handle->file_queue = divoom_queue_init(1000, __func__, __LINE__);
    pdivoom_download_aliyun_file_handle->local_file_queue = divoom_queue_init(1000, __func__, __LINE__);
    //divoom_system_create_task(divoom_download_aliyun_file_task, "ali_download", pdivoom_download_aliyun_file_handle, DIVOOM_TASK_STACK_ALIYUN_DOWNLOAD);
    //divoom_system_create_task(divoom_download_aliyun_create_thumb_pic_info, "ali_thumb", pdivoom_download_aliyun_file_handle, DIVOOM_TASK_STACK_ALIYUN_THUMB);
}

void divoom_download_aliyun_file_stop(void)
{
    int i;

    g_divoom_download_aliyun_file_stop = 1;
    if(pdivoom_download_aliyun_file_handle)
    {
        pdivoom_download_aliyun_file_handle->run_flag = 0;
        pdivoom_download_aliyun_file_handle->downloading_flag = 0;
    }
    divoom_http_client_request_abort();
    if(pdivoom_download_aliyun_file_handle)
    {
        for(i = 0; i < 50; i++)
        {
            if(!pdivoom_download_aliyun_file_handle->task_active)
            {
                break;
            }
            divoom_system_delay_ms(100);
        }
    }
}

int divoom_download_aliyun_file_add_item_compare(void *item1, void *item2)
{
    Pdivoom_download_aliyun_file_item pitem_info = (Pdivoom_download_aliyun_file_item)item2;

    return  pitem_info->photo_id == *(int  *)item1 ? 0:1;
}

/*********************************************************************
  Function:        divoom_download_aliyun_file_set_app_last_info
  Description:     设置下载文件的进度条
  Input:           total
                   cur_id
  Return:          none
**********************************************************************/

void divoom_download_aliyun_file_set_app_last_info(int total, int cur_id)
{
    if(pdivoom_download_aliyun_file_handle)
    {
        if(cur_id >= total)
        {
            cur_id = total;
            pdivoom_download_aliyun_file_handle->downloading_flag = 0;
        }
        else
        {
            pdivoom_download_aliyun_file_handle->downloading_flag = 1;
        }
        pdivoom_download_aliyun_file_handle->last_album_id = 0;
        pdivoom_download_aliyun_file_handle->last_pic_id = 0;
        pdivoom_download_aliyun_file_handle->cur_id = cur_id;
        pdivoom_download_aliyun_file_handle->total_num = total;
        divoom_photo_album_josn_report_transfer_progress(cur_id, total);
    }
}

/*********************************************************************
  Function:        divoom_download_aliyun_file_get_app_last_info
  Description:     获取APP最后发送的相片信息
  Input:           album_id
                   pic_id
  Return:          none
**********************************************************************/

int divoom_download_aliyun_file_get_app_last_info(int *album_id, int *pic_id, int *total, int *cur_id)
{
    *album_id = pdivoom_download_aliyun_file_handle->last_album_id;
    *pic_id = pdivoom_download_aliyun_file_handle->last_pic_id;
    *total = pdivoom_download_aliyun_file_handle->total_num;
    *cur_id = pdivoom_download_aliyun_file_handle->cur_id;
    return pdivoom_download_aliyun_file_handle->downloading_flag;
}

/*********************************************************************
  Function:        divoom_download_aliyun_file_app_del_pic_notice
  Description:     App删除相簿或相片时，调用这个进行更新，如果删除相簿，将pic_id= 0
  Input:           album_id
                   pic_id
  Return:          none
**********************************************************************/

void divoom_download_aliyun_file_app_del_pic_notice(int album_id, int pic_id)
{
    if(pic_id == 0)
    {
        if(pdivoom_download_aliyun_file_handle->last_album_id == album_id)
        {
            pdivoom_download_aliyun_file_handle->last_album_id = 0;
            pdivoom_download_aliyun_file_handle->last_pic_id = 0;
        }
    }
    else if(pdivoom_download_aliyun_file_handle->last_pic_id == pic_id)
    {
        pdivoom_download_aliyun_file_handle->last_album_id = 0;
        pdivoom_download_aliyun_file_handle->last_pic_id = 0;
    }
}

/*********************************************************************
  Function:        divoom_download_aliyun_file_set_download_flag
  Description:     设置下载标记
  Input:           flag
  Return:          none
**********************************************************************/


void divoom_download_aliyun_file_set_download_flag(int flag)
{
    if(pdivoom_download_aliyun_file_handle)
    {
        pdivoom_download_aliyun_file_handle->downloading_flag = flag;
    }
}

/*********************************************************************
  Function:        divoom_download_aliyun_add_create_thumb_pic
  Description:     预览图不存在，增加创建预览图
  Input:           file_name
  Return:          none
**********************************************************************/

void divoom_download_aliyun_add_create_thumb_pic(char *file_name)
{
    divoom_queue_push(pdivoom_download_aliyun_file_handle->local_file_queue, divoom_app_com_copy_string(file_name, -1));
    divoom_system_put_sem(pdivoom_download_aliyun_file_handle->local_file_sem);
}


void *divoom_download_aliyun_create_thumb_pic_info(void *para)
{
    char *pic_path;
    int out_w, out_h;
    
    if(divoom_queue_items(pdivoom_download_aliyun_file_handle->local_file_queue) == 0)
    {
        return;
    }
    pic_path = divoom_queue_pop(pdivoom_download_aliyun_file_handle->local_file_queue);
    if(pic_path == NULL)
    {
        return;
    }
    
    divoom_pic_phone_manage_create_picture_thumb(pic_path, &out_w, &out_h, NULL);

    divoom_system_free(pic_path);
    return NULL;
}
/*********************************************************************
  Function:        divoom_download_aliyun_file_add_local_pic_list
  Description:     APP通过本地发送一批文件
  Input:           papp_info
  Return:          none
**********************************************************************/


void divoom_download_aliyun_file_add_local_pic_list(void *para)
{
    int i, pic_id;
    char file_name[256];
    time_t cur_time;
	struct tm time_now; 
    char dest_name[256];
    Pdivoom_photo_album_app_local_photo_info papp_info = (Pdivoom_photo_album_app_local_photo_info)para;
    
    if(papp_info == NULL)
    {
        return;
    }
    /* LAN 文件进入正式相簿目录前同样执行一次可写盘空间检查。 */
    divoom_print("[storage_gc] call source=%s line=%d\n", __func__, __LINE__);
    divoom_clock_manage_clear_removed_clock_data_if_needed();
    cur_time = time(0);        
    localtime_r(&cur_time, &time_now);
    time_now.tm_year += 1900;
    time_now.tm_mon ++;

    
    strcpy(file_name, DIVOOM_SD_APP_PIC_DIR);
    strcat(file_name, "/");
    strcat(file_name, papp_info->file_name);    
    sprintf(dest_name, "%s/%d%d",DIVOOM_SD_APP_PIC_DIR,time_now.tm_year,time_now.tm_mon);
    divoom_app_com_mk_dir(dest_name);
    strcat(dest_name, "/");
    strcat(dest_name, papp_info->file_name);    
    divoom_app_com_rename_file(file_name, dest_name);
    divoom_print("image name: %s, %s\n", file_name, dest_name);
    if(strlen(papp_info->preview_file_name))
    {
        int len = strlen(dest_name);
        char *preview_name = divoom_app_com_copy_string(dest_name, len + 10);
        preview_name[len - 3] = 'w';
        preview_name[len - 2] = 'e';
        preview_name[len - 1] = 'b';
        preview_name[len] = 'p';
        preview_name[len + 1] = 0;
        strcpy(file_name, DIVOOM_SD_APP_PIC_DIR);
        strcat(file_name, "/");
        strcat(file_name, papp_info->preview_file_name);
        divoom_app_com_rename_file(file_name, preview_name);
        divoom_print("preview name: %s, %s\n", file_name, preview_name);
        
    }
    if(papp_info->photo_flag != pdivoom_download_aliyun_file_handle->photo_flag)
    {
        pdivoom_download_aliyun_file_handle->photo_flag = papp_info->photo_flag;
        pdivoom_download_aliyun_file_handle->total_num = papp_info->photo_total;            
        pdivoom_download_aliyun_file_handle->cur_id = 1;
    }
    else 
    {
        pdivoom_download_aliyun_file_handle->cur_id ++;
    }
    if(papp_info->album_id >= DIVOOM_PHOTO_START_CLOCK_ID)
    {
        
        pic_id = divoom_pic_phone_manage_add_picture(dest_name, papp_info->save_time, papp_info->photo_time, papp_info->photo_x,papp_info->photo_y,
            papp_info->album_id, papp_info->user_id, papp_info->title, papp_info->photo_w, papp_info->photo_h, divoom_pic_album_manage_check_album_cover(papp_info->album_id)?0:1);
        if(papp_info->photo_index && papp_info->photo_index < papp_info->photo_total - 2)
        {
            
            divoom_pic_album_manage_add_list_to_name_no_refresh(papp_info->album_id, &pic_id, 1);
        }
        else
        {
            
            divoom_pic_album_manage_add_list_to_name(papp_info->album_id, &pic_id, 1);
        }
        pdivoom_download_aliyun_file_handle->last_album_id = papp_info->album_id;
        pdivoom_download_aliyun_file_handle->last_pic_id = pic_id;
        pdivoom_download_aliyun_file_handle->total_num = papp_info->photo_total;
    }
    else
    {
        divoom_clock_id_info clock_info;

        clock_info.clock_id = papp_info->album_id;
        clock_info.clock_pos = papp_info->clock_pos;
        clock_info.parent_id = papp_info->parent_id;
        divoom_custom_photo_add_item(&clock_info, papp_info->save_time, papp_info->photo_time, dest_name); 
    }
    divoom_queue_push(pdivoom_download_aliyun_file_handle->local_file_queue, divoom_app_com_copy_string(dest_name, -1));
    divoom_system_put_sem(pdivoom_download_aliyun_file_handle->local_file_sem);
    
    if(pdivoom_download_aliyun_file_handle->cur_id < pdivoom_download_aliyun_file_handle->total_num)
    {
        divoom_main_push_model_event_id(DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_EVENT_APP_SEND_PHOTO);
    }
    
}

/*********************************************************************
  Function:        divoom_download_aliyun_file_add_vision_item
  Description:     增加一个文件下载
  Input:           vision_id
                   oss_name
                   pcallback
  Return:          none
**********************************************************************/


void divoom_download_aliyun_file_add_vision_item(int vision_id, char *oss_name,divoom_download_aliyun_file_finish_callback *pcallback)
{
    Pdivoom_download_aliyun_file_item pitem_info;
    
    pitem_info  = divoom_system_malloc(sizeof(divoom_download_aliyun_file_item));
    if(pitem_info == NULL)
    {
        return;
    }
    pitem_info->fail_times = 0;
    pitem_info->photo_id = vision_id;
    pitem_info->no_del_flag = 1;
    strcpy(pitem_info->os_name, oss_name);
    pitem_info->server_flag = divoom_main_get_server_type();
    pitem_info->pcallback = pcallback;
    divoom_queue_push(pdivoom_download_aliyun_file_handle->file_queue, pitem_info);
    if(divoom_main_get_wifi_con_status())
    {
        divoom_main_push_model_event_id(DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_EVENT_APP_SEND_PHOTO);
    }
    divoom_system_put_sem(pdivoom_download_aliyun_file_handle->wait_sem);
        
}

/*********************************************************************
  Function:        divoom_download_aliyun_file_add_cloud_photo_item
  Description:     增加一个文件下载
  Input:           vision_id
                   oss_name
                   pcallback
  Return:          none
**********************************************************************/


void divoom_download_aliyun_file_add_cloud_photo_item(int vision_id, int oss_flag,char *oss_name,divoom_download_aliyun_file_finish_callback *pcallback)
{
    Pdivoom_download_aliyun_file_item pitem_info;
    
    pitem_info  = divoom_system_malloc(sizeof(divoom_download_aliyun_file_item));
    if(pitem_info == NULL)
    {
        return;
    }
    pitem_info->fail_times = 0;
    pitem_info->photo_id = vision_id;
    pitem_info->no_del_flag = 2;
    strcpy(pitem_info->os_name, oss_name);
    pitem_info->server_flag = oss_flag;
    pitem_info->pcallback = pcallback;
    divoom_queue_push(pdivoom_download_aliyun_file_handle->file_queue, pitem_info);
        
    if(divoom_main_get_wifi_con_status())
    {
        divoom_main_push_model_event_id(DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_EVENT_APP_SEND_PHOTO);
    }
    divoom_system_put_sem(pdivoom_download_aliyun_file_handle->wait_sem);
        
}

/*********************************************************************
  Function:        divoom_download_aliyun_file_add_item
  Description:     增加一个文件下载
  Input:           pnew_item
                   number
                   pcallback
  Return:          none
**********************************************************************/


void divoom_download_aliyun_file_add_item(Pdivoom_download_aliyun_file_item pnew_item, int number, divoom_download_aliyun_file_finish_callback *pcallback, int init_flag)
{
    int i, download_files = 0;
    Pdivoom_download_aliyun_file_item pitem_info;
    
    if(number == 0)
    {
        pdivoom_download_aliyun_file_handle->load_finish_flag = 1;
        return;
    }

    for(i = 0; i < number; i ++)
    {
        pitem_info = divoom_queue_get_cmp(pdivoom_download_aliyun_file_handle->file_queue, &pnew_item[i].photo_id, divoom_download_aliyun_file_add_item_compare);
        if(pitem_info)
        {
        
            continue;
        }
        if(pnew_item[i].file_size >= 50*1024)
        {
            //文件过大，将直接删除
            divoom_photo_album_get_local_photo_request_item pdel_info = {0};
            
            pdel_info.photo_id = pnew_item[i].photo_id;
            pdel_info.server_flag = pnew_item[i].server_flag;
            strcpy(pdel_info.big_name, pnew_item[i].os_name);
            divoom_net_app_send_packet(DIVOOM_NET_COMM_DELETE_ALI_PHOTO, DIVOOM_SERVER_TOPIC,&pdel_info);
            if(strlen(pnew_item[i].preview_os_name))
            {
                strcpy(pdel_info.big_name, pnew_item[i].preview_os_name);
                divoom_net_app_send_packet(DIVOOM_NET_COMM_DELETE_ALI_PHOTO, DIVOOM_SERVER_TOPIC,&pdel_info);
            }
            pdivoom_download_aliyun_file_handle->cur_id ++;
            divoom_print("we will delete too large file: %d, %s, %s !\n", pnew_item[i].file_size, pnew_item[i].os_name, pnew_item[i].preview_os_name);
            continue;
        }
        if(pnew_item[i].photo_flag != pdivoom_download_aliyun_file_handle->photo_flag)
        {
            pdivoom_download_aliyun_file_handle->photo_flag = pnew_item[i].photo_flag;
            pdivoom_download_aliyun_file_handle->total_num = pnew_item[i].photo_total;            
            pdivoom_download_aliyun_file_handle->cur_id = 0;
        }
        pitem_info  = divoom_system_malloc(sizeof(divoom_download_aliyun_file_item));
        if(pitem_info == NULL)
        {
            return;
        }
        download_files ++;
        memcpy(pitem_info, &pnew_item[i], sizeof(divoom_download_aliyun_file_item));
        pitem_info->fail_times = 0;
        pitem_info->pcallback = pcallback;
        divoom_queue_push(pdivoom_download_aliyun_file_handle->file_queue, pitem_info);
    }
    if(init_flag && number < 100)
    {
        pdivoom_download_aliyun_file_handle->load_finish_flag = 1;
    }
    if(download_files)
    {
        divoom_main_push_model_event_id(DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_EVENT_APP_SEND_PHOTO);
    }
        
    divoom_system_put_sem(pdivoom_download_aliyun_file_handle->wait_sem);
        
}

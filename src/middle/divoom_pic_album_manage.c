#include <sys/stat.h>
#include "divoom.h"
#include "divoom_menu_control.h"

typedef struct _divoom_pic_album_manage_create_disp_order_ctrl
{
    int album_id;
    uint8 disp_order;
    uint8 disp_reverse;
    uint8 movie_flag;
    int total_num;  //所有图片总数
    
    int high_total_num; //长图片总数
    int last_high_pic_id; //下一次要显示的图片
    int high_pic_num;       //当前读的数目
    int high_read_pos;
    int cache_high_pic_num;
    
    int width_total_num;    //图片总数
    int width_pic_num;      //当前读的数目
    int width_read_pos;
    int last_width_pic_id;  //下一次要显示的图片
    int cache_width_pic_num;
    
    int last_pic_id;   //下一次要显示的图片
    int pause_flag;
    uint32 *width_pic_list;
    uint32 *high_pic_list;
    void *psticker_width_rand_handle;
    void *psticker_high_rand_handle;
    void *prand_handle;
}divoom_pic_album_manage_create_disp_order_ctrl, *Pdivoom_pic_album_manage_create_disp_order_ctrl;

typedef struct _divoom_pic_album_manage_ctrl
{
    int update_flag;    //是否需要更新相簿播放
    int last_album_id;
    double last_pic_time;
    int last_app_pic_id;
    int last_app_album_id;
    int last_app_order;
    int last_app_reverse;
    int last_pic_id;
    void *app_remove_queue;
    void *lock;
    void *album_list_queue;
    divoom_pic_album_manage_head_data palbum_head;
}divoom_pic_album_manage_ctrl, *Pdivoom_pic_album_manage_ctrl;


static Pdivoom_pic_album_manage_ctrl pdivoom_pic_album_manage_handle = NULL;


/*********************************************************************
  Function:        divoom_pic_album_manage_get_album_id_list
  Description:     获取相簿id列表
  Input:           
  Return:          NONE
**********************************************************************/

int *divoom_pic_album_manage_get_album_id_list(int *album_num)
{
    int i;
	int *album_list;

	(*album_num) = 0;
	
	divoom_system_get_lock(pdivoom_pic_album_manage_handle->lock);

	album_list = (int *)divoom_system_malloc(sizeof(int) * pdivoom_pic_album_manage_handle->palbum_head.album_num);
	for(i = 0; i < pdivoom_pic_album_manage_handle->palbum_head.album_num; i++)
    {
		album_list[*album_num] = pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].id;
		(*album_num)++;
    }

	divoom_system_free_lock(pdivoom_pic_album_manage_handle->lock);

	return album_list;
}


/*********************************************************************
  Function:        divoom_pic_album_manage_get_album_local_pic_list
  Description:     APP获取相簿相片列表
  Input:           album_id
                   start
                   end
  Return:          NONE
**********************************************************************/

void divoom_pic_album_manage_get_album_local_pic_list(int album_id, int start, int end)
{
    Pdivoom_pic_id_name_info pic_list;
    int i, pic_num, total_num, src_w, src_h;
    int last_read_pic_id;
    double start_time;
    divoom_port_stat_t file_st;
    Pdivoom_photo_album_get_local_photo_request_info pinfo;
    void *palbum_handle, *pget_pic_handle;
    Pdivoom_pic_album_manage_list_item pdisp_info;
    Pdivoom_pic_phone_manage_slide_config pslide_config = divoom_pic_phone_manage_get_slide_config();

    pic_num = end - start;
    if(pslide_config == NULL || pic_num <= 0)
    {
        divoom_net_app_send_packet(DIVOOM_NET_COMM_PHOTO_GET_LOCAL_PHOTO_LIST, DIVOOM_APP_TOPIC, NULL);
        return;
    }
    if(pic_num > 100)
    {
        pic_num = 100;
    }
    if(start == 0 || pdivoom_pic_album_manage_handle->last_app_album_id != album_id ||
       pdivoom_pic_album_manage_handle->last_app_order != pslide_config->disp_order ||
       pdivoom_pic_album_manage_handle->last_app_reverse != pslide_config->disp_reverse)
    {
        pdivoom_pic_album_manage_handle->last_app_pic_id = 0;
        start_time = -1;
    }
    else
    {
        start_time = pdivoom_pic_album_manage_handle->last_pic_time;
    }

    pic_list = divoom_database_manage_get_all_pic_id_and_name(album_id, &total_num, pic_num, pslide_config->disp_order, pslide_config->disp_reverse, start_time, pdivoom_pic_album_manage_handle->last_app_pic_id);
    
    if(total_num)
    {
        pinfo = divoom_system_malloc(sizeof(divoom_photo_album_get_local_photo_request_info) + total_num * sizeof(divoom_photo_album_get_local_photo_request_item));
        if(pinfo)
        {
            memset(pinfo, 0, sizeof(*pinfo));
            pinfo->plist = (Pdivoom_photo_album_get_local_photo_request_item)&pinfo[1];
            pinfo->num = 0;
            for(i = 0; i < total_num; i++)
            {
                pinfo->plist[pinfo->num].photo_id = pic_list[i].id;
                if(pslide_config->disp_order == DIVOOM_PIC_PHOTO_SLIDE_DISP_PHTOT_TIME)
                {
                    pinfo->plist[pinfo->num].save_time = pic_list[i].time;
                }
                else
                {
                    pinfo->plist[pinfo->num].save_time = pic_list[i].time/1000;
                }
                strcpy(pinfo->plist[pinfo->num].big_name, pic_list[i].name);
                divoom_thumb_cache_get_thumb_name(pinfo->plist[pinfo->num].big_name, pinfo->plist[pinfo->num].small_name);
                //判断一下，是不是错误文件,如果文件不存在或者大小写为0，统一认为错误，将重新生成
                if (divoom_port_stat(pinfo->plist[pinfo->num].small_name, &file_st) == 0) {
                    if(file_st.size > 0)
                    {
                    }
                    else
                    {
                        divoom_app_com_del_file(pinfo->plist[pinfo->num].small_name);
                        strcpy(pinfo->plist[pinfo->num].small_name, pinfo->plist[pinfo->num].big_name);
                        divoom_download_aliyun_add_create_thumb_pic(pinfo->plist[pinfo->num].big_name);
                    }
                }
                else
                {
                        strcpy(pinfo->plist[pinfo->num].small_name, pinfo->plist[pinfo->num].big_name);
                        divoom_download_aliyun_add_create_thumb_pic(pinfo->plist[pinfo->num].big_name);
                }
                pinfo->num ++;
            }
            pdivoom_pic_album_manage_handle->last_pic_time = pic_list[total_num -1].time;
            pdivoom_pic_album_manage_handle->last_app_pic_id = pic_list[total_num -1].id;
            pdivoom_pic_album_manage_handle->last_app_album_id = album_id;
            pdivoom_pic_album_manage_handle->last_app_order = pslide_config->disp_order;
            pdivoom_pic_album_manage_handle->last_app_reverse = pslide_config->disp_reverse;
            divoom_net_app_send_packet(DIVOOM_NET_COMM_PHOTO_GET_LOCAL_PHOTO_LIST, DIVOOM_APP_TOPIC,pinfo);
            divoom_system_free(pinfo);
        }
    }
    else
    {
        divoom_net_app_send_packet(DIVOOM_NET_COMM_PHOTO_GET_LOCAL_PHOTO_LIST, DIVOOM_APP_TOPIC, NULL);
    }
    divoom_system_free(pic_list);

    
}

/*********************************************************************
  Function:        divoom_pic_album_manage_check_preload_album
  Description:     检查是否要增加预载相簿
  Input:           palarm_info
  Return:          NONE
**********************************************************************/

int divoom_pic_album_manage_check_preload_album(void)
{    
    int preload_album_flag = divoom_pic_phone_manage_set_preload_album_flag(-1);

    if(preload_album_flag == 2)
    {
        return 0;
    }
    if(preload_album_flag == 1)
    {
        return 1;
    }
    if(pdivoom_pic_album_manage_handle->palbum_head.album_num == 0)
    {
        divoom_pic_phone_manage_set_preload_album_flag(1);
        return 1;
    }
    if(pdivoom_pic_album_manage_handle->palbum_head.album_num == 1)
    {
        //只有一个ALL相簿，将检查全部照片张数是否是默认的，还有最后一张的ID是不是45，如果两个都是，将认为是要加载预载相片
        int total = divoom_database_manage_get_total_pic_num(0, 0, 0, -1, 0, 1);

        if(total == 45)
        {
            int max_pic_id = divoom_database_manage_get_max_pic_id();
            if(max_pic_id == 45)
            {
                divoom_pic_phone_manage_set_preload_album_flag(1);
                return 1;
            }
        }
    }
    return 0;
}

/*********************************************************************
  Function:        divoom_pic_album_manage_update_list
  Description:     从服务器新获取相簿列表
  Input:           palarm_info
  Return:          NONE
**********************************************************************/

void divoom_pic_album_manage_update_list(Pdivoom_photo_album_album_list  palarm_info)
{
    char album_file_name[128];
    int i, j, del_num = 0, add_num = 0, update_flag = 0, old_album_num, preload_album_id = -1, preload_album_flag;
    uint32 *album_id_list;

    for(i = 0; i < palarm_info->num; i++)
    {
        if(palarm_info->palbum_list[i].album_type == DIVOOM_PIC_ALBUM_TYPE_PRELOAD)
        {
            preload_album_id = palarm_info->palbum_list[i].album_id;
            break;
        }
    }
    preload_album_flag = divoom_pic_album_manage_check_preload_album();
    old_album_num = pdivoom_pic_album_manage_handle->palbum_head.album_num;
    if(pdivoom_pic_album_manage_handle->palbum_head.album_num)
    {
        album_id_list = divoom_system_malloc(sizeof(uint32) *(pdivoom_pic_album_manage_handle->palbum_head.album_num > palarm_info->num?pdivoom_pic_album_manage_handle->palbum_head.album_num:palarm_info->num));
        if(album_id_list == NULL)
        {
            divoom_print("it fails to malloc!\n");
            return ;
        }
        //先找出已经被删除的相簿，清除相关数据
        for(i = 0; i < pdivoom_pic_album_manage_handle->palbum_head.album_num; i++)
        {
            for(j = 0; j < palarm_info->num; j ++)
            {
                if(palarm_info->palbum_list[j].album_id== pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].id)
                {
                    if(strcmp(pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].album_name, palarm_info->palbum_list[j].album_name))
                    {
                        update_flag = 1;
                        divoom_print("we will update the album name!: %d, %s, %s\n",palarm_info->palbum_list[j].album_id, pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].album_name, palarm_info->palbum_list[j].album_name);
                        strcpy(pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].album_name, palarm_info->palbum_list[j].album_name);
                        divoom_database_manage_update_album_name_record(pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].id, pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].album_name);
                    }
                    break;
                }
            }
            if(j >= palarm_info->num)
            {
                update_flag = 1;
                album_id_list[del_num] = pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].id;
                divoom_print("we will delete the album name!: %d, %s\n",pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].id, pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].album_name);
                del_num ++;
            }
        }
        for(i = 0; i < del_num; i ++)
        {
            divoom_pic_album_manage_del_type(album_id_list[i], 0);
        }
        divoom_system_free(album_id_list);
    }

    //再看有没有增加的
    
    
    for(i = 0; i < palarm_info->num; i++)
    {
    
        for(j = 0; j < pdivoom_pic_album_manage_handle->palbum_head.album_num; j ++)
        {
            if(palarm_info->palbum_list[i].album_id== pdivoom_pic_album_manage_handle->palbum_head.palbum_list[j].id)
            {
                break;
            }
        }
        if(j >= pdivoom_pic_album_manage_handle->palbum_head.album_num)
        {
    
            update_flag = 1;
            divoom_print("we will add album; %d, %s\n", palarm_info->palbum_list[i].album_id, palarm_info->palbum_list[i].album_name);
            divoom_pic_album_manage_add_type(palarm_info->palbum_list[i].album_name, palarm_info->palbum_list[i].album_id, palarm_info->palbum_list[i].album_type);
            
        }
    }
    
    for(i = 0; i < pdivoom_pic_album_manage_handle->palbum_head.album_num; i++)
    {
        divoom_print("album info: %d, id:%d, type: %d, name: %s\n", i, pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].id,
            pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].type,pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].album_name);
    }

    //如果没有设置封面，将删除封面
    if(divoom_pic_phone_manage_set_all_album_cover_pic(-1) == 0)
    {
        int all_album_id = divoom_pic_album_manage_get_album_id_from_type(DIVOOM_PIC_ALBUM_TYPE_ALL);    
        
        sprintf(album_file_name, "%s/%d.webp", DIVOOM_ALBUM_COVER_INFO_DIR, all_album_id);
        
        if(divoom_app_com_check_file_exist(album_file_name) == 1)
        {
            divoom_app_com_del_dir(album_file_name);
        }
    }
    if(preload_album_flag == 1)
    {
        //需要预装
        if(preload_album_id != -1)
        {
            divoom_pic_phone_manage_add_all_preload(preload_album_id);
        }
        else
        {
            //向服务器请求
            divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_CREATE_PRELOAD_ALBUM, DIVOOM_SERVER_TOPIC, NULL);
        }
    } 
}


/*********************************************************************
  Function:        divoom_pic_album_manage_load_head
  Description:     加载相册类型头相关信息
  Input:           none
  Return:          返回句柄
**********************************************************************/

void divoom_pic_album_manage_print_all_album_info(void)
{    
    int i;

    divoom_print("album number: %d\n", pdivoom_pic_album_manage_handle->palbum_head.album_num);
    for(i = 0; i  < pdivoom_pic_album_manage_handle->palbum_head.album_num; i++)
    {
        divoom_print("album info: %d, id:%d; type: %d, name: %s\n", i, pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].id
            , pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].type,
            pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].album_name);
    }
}

/*********************************************************************
  Function:        divoom_pic_album_manage_load_head
  Description:     加载相册类型头相关信息
  Input:           none
  Return:          返回相簿数目
**********************************************************************/

void divoom_pic_album_manage_load_head(void)
{    
    int i;
    pdivoom_pic_album_manage_handle->palbum_head.palbum_list = divoom_database_manage_get_all_album_info(&pdivoom_pic_album_manage_handle->palbum_head.album_num);
    //初始几个默认的名字
    for(i = 0; i  < pdivoom_pic_album_manage_handle->palbum_head.album_num; i++)
    {
        if(pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].type == DIVOOM_PIC_ALBUM_TYPE_ALL)
        {
            strcpy(pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].album_name, "All");
            continue;
        }
        else if(pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].type == DIVOOM_PIC_ALBUM_TYPE_LIKE)
        {
            strcpy(pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].album_name, "Like");
        }
        else if(pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].type == DIVOOM_PIC_ALBUM_TYPE_HIDE)
        {
            strcpy(pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].album_name, "Hide");
        }
        if(pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].id)
        {
            //检查相簿是否存在，不存在将创建
            if(divoom_pic_album_manage_check_album_cover_exist(pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].id) == 0)
            {
                //使用相簿最后一张图做为封面
                int first_pic_id = divoom_pic_phone_manage_get_first_pic_id(pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].id);
                if(first_pic_id > 0)
                {
                    divoom_pic_album_manage_set_album_cover(pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].id, first_pic_id, 1, 0);
                }
            }
        }
    }
    divoom_pic_album_manage_print_all_album_info();
}

/*********************************************************************
  Function:        divoom_pic_album_manage_update_last_play_album_pic_id
  Description:     更新相簿最后播放的照片ID
  Input:           album_id
                   pic_id
  Return:          none
**********************************************************************/

void divoom_pic_album_manage_update_last_play_album_pic_id(int album_id, int pic_id)
{    
    int i;
    for(i = 0; i  < pdivoom_pic_album_manage_handle->palbum_head.album_num; i++)
    {
        if(album_id == 0)
        {
            if(pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].type == DIVOOM_PIC_ALBUM_TYPE_ALL)
            {
                pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].pic_id = pic_id;
                return ;
            }
        }
        else if(pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].id == album_id)
        {
            pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].pic_id = pic_id;
            return ;
        }
    }
}

/*********************************************************************
  Function:        divoom_pic_album_manage_set_one_play_album_pic_id
  Description:     更新相簿只播放的照片ID
  Input:           album_id
                   pic_id
  Return:          none
**********************************************************************/

void divoom_pic_album_manage_set_one_play_album_pic_id(int album_id, int pic_id)
{    
    int i;
    for(i = 0; i  < pdivoom_pic_album_manage_handle->palbum_head.album_num; i++)
    {
        if(album_id == 0)
        {
            if(pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].type == DIVOOM_PIC_ALBUM_TYPE_ALL)
            {
                pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].pic_id = pic_id;
                pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].pause_flag = 1;
                return ;
            }
        }
        else if(pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].id == album_id)
        {
            pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].pic_id = pic_id;
            pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].pause_flag = 1;
            return ;
        }
    }
}

/*********************************************************************
  Function:        divoom_pic_album_manage_update_last_play_album_stricker_pic_id
  Description:     更新相簿最后播放的照片ID
  Input:           album_id
                   width_pic_id
                   high_pic_id
  Return:          none
**********************************************************************/

void divoom_pic_album_manage_update_last_play_album_stricker_pic_id(int album_id, int width_pic_id, int high_pic_id)
{    
    int i;
    for(i = 0; i  < pdivoom_pic_album_manage_handle->palbum_head.album_num; i++)
    {
        if(album_id == 0)
        {
            if(pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].type == DIVOOM_PIC_ALBUM_TYPE_ALL)
            {
                pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].width_pic_id = width_pic_id;
                pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].high_pic_id = high_pic_id;
                if(high_pic_id)
                {
                    pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].pic_id = high_pic_id;
                }
                else
                {
                    pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].pic_id = width_pic_id;
                }
                return ;
            }
        }
        else if(pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].id == album_id)
        {
                pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].width_pic_id = width_pic_id;
                pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].high_pic_id = high_pic_id;
                if(high_pic_id)
                {
                    pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].pic_id = high_pic_id;
                }
                else
                {
                    pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].pic_id = width_pic_id;
                }
            return ;
        }
    }
}

/*********************************************************************
  Function:        divoom_pic_album_manage_get_last_play_album_pic_id
  Description:     得到这个相簿最后播放的照片ID
  Input:           album_id
  Return:          返回照片ID
**********************************************************************/

int divoom_pic_album_manage_get_last_play_album_pic_id(int album_id, int *pause_flag)
{    
    int i;
    char file_name[128];
    for(i = 0; i  < pdivoom_pic_album_manage_handle->palbum_head.album_num; i++)
    {
        if(album_id == 0)
        {
            if(pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].type == DIVOOM_PIC_ALBUM_TYPE_ALL)
            {
                divoom_pic_phone_manage_get_picture_name(file_name, pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].pic_id);
                if(divoom_app_com_check_file_exist(file_name) == 0)
                {
                    pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].pic_id = 0;
                }
                if(pause_flag)
                {
                    *pause_flag = pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].pause_flag;
                }
                pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].pause_flag = 0;
                return pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].pic_id;
            }
        }
        else if(pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].id == album_id)
        {
            divoom_pic_phone_manage_get_picture_name(file_name, pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].pic_id);
            if(divoom_app_com_check_file_exist(file_name) == 0)
            {
                pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].pic_id = 0;
            }
            if(pause_flag)
            {
                *pause_flag = pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].pause_flag;
            }
            pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].pause_flag = 0;
            return pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].pic_id;
        }
    }
    return 0;
}

/*********************************************************************
  Function:        divoom_pic_album_manage_get_last_play_album_pic_id
  Description:     得到这个相簿最后播放的照片ID
  Input:           album_id
  Return:          返回照片ID
**********************************************************************/

void divoom_pic_album_manage_get_last_play_album_stricker_pic_id(int album_id, int *width_id, int *high_id)
{    
    int i;
    for(i = 0; i  < pdivoom_pic_album_manage_handle->palbum_head.album_num; i++)
    {
        if(album_id == 0)
        {
            if(pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].type == DIVOOM_PIC_ALBUM_TYPE_ALL)
            {
                *width_id = pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].width_pic_id;
                *high_id = pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].high_pic_id;
                return;
            }
        }
        else if(pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].id == album_id)
        {
                *width_id = pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].width_pic_id;
                *high_id = pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].high_pic_id;
                return;
        }
    }
    return ;
}

/*********************************************************************
  Function:        divoom_pic_album_manage_init
  Description:     初始化模块信息
  Input:           none
  Return:          返回相簿数目
**********************************************************************/

int divoom_pic_album_manage_init(void)
{
    pdivoom_pic_album_manage_handle = divoom_system_malloc(sizeof(divoom_pic_album_manage_ctrl));
    pdivoom_pic_album_manage_handle->lock = divoom_system_create_lock();
    pdivoom_pic_album_manage_handle->album_list_queue = divoom_queue_init(DIVOOM_PHOTO_MAXALBUM, __func__, __LINE__);
    pdivoom_pic_album_manage_handle->app_remove_queue = divoom_queue_init(200, __func__, __LINE__);
    
    divoom_pic_album_manage_load_head();
    
    return pdivoom_pic_album_manage_handle->palbum_head.album_num;
}

void divoom_pic_album_manage_free_album_item(void *item)
{
    Pdivoom_pic_album_manage_album_item palbum_item = (Pdivoom_pic_album_manage_album_item)item;

    if(palbum_item)
    {
        divoom_system_free(palbum_item->pic_list);
        divoom_system_free(palbum_item);
    }
}


/*********************************************************************
  Function:        divoom_pic_album_manage_clear_all_type
  Description:     清除所有相册
  Input:           none
  Return:          NONE
**********************************************************************/

void divoom_pic_album_manage_clear_all_type(void)
{
    int i;
    char file_name[128];

    if(pdivoom_pic_album_manage_handle == NULL)
    {
        return;
    }

    divoom_database_manage_reset_pic_info();
    
}

/*********************************************************************
  Function:        divoom_pic_album_manage_get_all_album_info
  Description:     得到所有相册信息
  Input:           none
  Return:          相册信息
**********************************************************************/

Pdivoom_pic_album_manage_head_data divoom_pic_album_manage_get_all_album_info(void)
{
    return &pdivoom_pic_album_manage_handle->palbum_head;
}

/*********************************************************************
  Function:        divoom_pic_album_manage_get_album_name
  Description:     得到相册的名称
  Input:           album_name 相册名称
  Return:          相册ID
**********************************************************************/

void divoom_pic_album_manage_get_album_name(int album_id,char *album_name)
{
    int i;

    //只清空当前的相册数据，但是并不释放内存
    for(i = 0; i < pdivoom_pic_album_manage_handle->palbum_head.album_num; i++)
    {
        if(album_id == pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].id)
        {
            strcpy(album_name, pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].album_name);
            break;
        }
    }
}

/*********************************************************************
  Function:        divoom_pic_album_manage_get_static_album_name
  Description:     得到相册的名称
  Input:           album_id 
  Return:          相册名称
**********************************************************************/
 char divoom_album_all_name[] = "AllPic";
char divoom_album_like_name[] = "Like";
char divoom_album_hide_name[] = "Hide";

char* divoom_pic_album_manage_get_static_album_name(int album_id)
{
    char *ret_str, *test_str;
    int i;

    divoom_print("album id: %d\n", album_id);
    if(album_id == 0)
    {
        return divoom_menu_control_get_disp_str(divoom_album_all_name);                
    }
    //只清空当前的相册数据，但是并不释放内存
    for(i = 0; i < pdivoom_pic_album_manage_handle->palbum_head.album_num; i++)
    {
        if(album_id == pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].id)
        {
            divoom_print("album type: %d, %d\n",album_id, pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].type);
            if(pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].type == DIVOOM_PIC_ALBUM_TYPE_ALL)
            {
                return divoom_menu_control_get_disp_str(divoom_album_all_name);                
            }
            else if(pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].type == DIVOOM_PIC_ALBUM_TYPE_LIKE)
            {
                return divoom_menu_control_get_disp_str(divoom_album_like_name);                
            }
            else if(pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].type == DIVOOM_PIC_ALBUM_TYPE_HIDE)
            {
                return divoom_menu_control_get_disp_str(divoom_album_hide_name);
            }
            else
            {
                return pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].album_name;
            }
        }
    }
    return divoom_menu_control_get_disp_str(divoom_album_all_name);                
    return NULL;
}


/*********************************************************************
  Function:        divoom_pic_album_manage_get_album_info
  Description:     查找相册的ID
  Input:           album_id 相册ID
  Return:          相册ID
**********************************************************************/

Pdivoom_pic_album_manage_head_item divoom_pic_album_manage_get_album_info(int album_id)
{
    int i;

    //只清空当前的相册数据，但是并不释放内存
    divoom_system_get_lock(pdivoom_pic_album_manage_handle->lock);
    for(i = 0; i < pdivoom_pic_album_manage_handle->palbum_head.album_num; i++)
    {
        if(album_id == pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].id)
        {
            divoom_system_free_lock(pdivoom_pic_album_manage_handle->lock);
            return &pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i];
            break;
        }
    }
    divoom_system_free_lock(pdivoom_pic_album_manage_handle->lock);
    return NULL;
}

/*********************************************************************
  Function:        divoom_pic_album_manage_get_album_id_from_type
  Description:     相簿类型得到第一个相簿ID
  Input:           album_type 相册类型
  Return:          相册ID
**********************************************************************/

int divoom_pic_album_manage_get_album_id_from_type(int album_type)
{
    int i, album_id = 0;

    //只清空当前的相册数据，但是并不释放内存
    if(pdivoom_pic_album_manage_handle->palbum_head.album_num == 0)
    {
        Pdivoom_net_command pcmd_info;
        pcmd_info = divoom_net_app_send_packet_wait(DIVOOM_NET_COMM_PHOTO_GET_ALBUM_LIST, DIVOOM_SERVER_TOPIC, NULL);
        divoom_print("we will reload album;\n");
        if(pcmd_info)
        {
            if(pcmd_info->cmd_data)
            {
    	        divoom_pic_album_manage_update_list((Pdivoom_photo_album_album_list)pcmd_info->cmd_data);
            }
            divoom_net_app_free_packet(pcmd_info);
        }
    }
    
    for(i = 0; i < pdivoom_pic_album_manage_handle->palbum_head.album_num; i++)
    {
        if(pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].type == album_type)
        {
            album_id = pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].id;
            return album_id;
        }
    }
    divoom_print("it fails get album; %d\n", pdivoom_pic_album_manage_handle->palbum_head.album_num);
    for(i = 0; i < pdivoom_pic_album_manage_handle->palbum_head.album_num; i++)
    {
        divoom_print( "album info; %d, %d\n", pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].id, pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].type);
    }
    if(divoom_main_get_custom_product_flag())
    {
        return DIVOOM_CUSTOM_START_CLOCK_ID;
    }
    else
    {
        return album_id;
    }
}

/*********************************************************************
  Function:        divoom_pic_album_manage_update_album_name
  Description:     更新相簿名称
  Input:           album_id
                   album_name 相册名称
  Return:          none
**********************************************************************/

void divoom_pic_album_manage_update_album_name(int album_id, char *album_name)
{
    int i;


    divoom_system_get_lock(pdivoom_pic_album_manage_handle->lock);
    for(i = 0; i < pdivoom_pic_album_manage_handle->palbum_head.album_num; i++)
    {
        if(album_id == pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].id)
        {
            strcpy(pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].album_name, album_name);
            divoom_database_manage_update_album_name_record(album_id, album_name);
            pdivoom_pic_album_manage_handle->update_flag = 1;
            break;
        }
    }
    divoom_system_free_lock(pdivoom_pic_album_manage_handle->lock);
}

/*********************************************************************
  Function:        divoom_pic_album_manage_check_album_id
  Description:     确认相册的ID，如果不存在返回全部照片
  Input:           album_id 
  Return:          相册ID
**********************************************************************/

int divoom_pic_album_manage_check_album_id(int album_id)
{
    int i, all_album_id = 0;

    //只清空当前的相册数据，但是并不释放内存
    for(i = 0; i < pdivoom_pic_album_manage_handle->palbum_head.album_num; i++)
    {
        if(pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].type == DIVOOM_PIC_ALBUM_TYPE_ALL)
        {
            all_album_id = pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].id;
        }
        if(album_id == pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].id)
        {
            return album_id;
        }
    }
    return all_album_id;
}

/*********************************************************************
  Function:        divoom_pic_album_manage_check_album_cover
  Description:     检查相簿封面是否存在
  Input:           album_id 
  Return:          1 or 0
**********************************************************************/

int divoom_pic_album_manage_check_album_cover(int album_id)
{
    char pic_path[256];

    if(album_id == divoom_pic_album_manage_get_album_id_from_type(DIVOOM_PIC_ALBUM_TYPE_ALL))
    {
        //全部照片，只有APP设置才修改封面
        return 1;
    }
    sprintf(pic_path, "%s/%d.webp", DIVOOM_ALBUM_COVER_INFO_DIR, album_id);
    return divoom_app_com_check_file_exist(pic_path);
}

/*********************************************************************
  Function:        divoom_pic_album_manage_set_album_cover
  Description:     设置相簿封面s
  Input:           album_id 
                   photo_id
                   flag
  Return:          none
**********************************************************************/

void divoom_pic_album_manage_set_album_cover(int album_id, int photo_id, int flag, int app_flag)
{
    char pic_path[256], thumb_path[256];

    if(photo_id < 0)
    {
        return;
    }
    if(app_flag == 0 && album_id == divoom_pic_album_manage_get_album_id_from_type(DIVOOM_PIC_ALBUM_TYPE_ALL))
    {
        //全部照片，只有APP设置才修改封面
        return;
    }
    if(flag  == 0)
    {
        sprintf(pic_path, "%s/%d.webp", DIVOOM_ALBUM_COVER_INFO_DIR, album_id);
        if(divoom_app_com_check_file_exist(pic_path))
        {
            return;
        }
    }
    divoom_pic_phone_manage_get_picture_name(pic_path, photo_id);
    divoom_thumb_cache_get_thumb_name(pic_path, thumb_path);
    sprintf(pic_path, "%s/%d.webp", DIVOOM_ALBUM_COVER_INFO_DIR, album_id);
    divoom_app_com_copy_file(thumb_path, pic_path);
}

static void divoom_pic_album_manage_delete_cover_file(int album_id)
{
    char pic_path[256];

    if(album_id <= 0)
    {
        return;
    }
    sprintf(pic_path, "%s/%d.webp", DIVOOM_ALBUM_COVER_INFO_DIR, album_id);
    if(divoom_app_com_check_file_exist(pic_path))
    {
        divoom_app_com_del_file(pic_path);
    }
}

static void divoom_pic_album_manage_clear_cover_if_empty(int album_id)
{
    if(album_id <= 0)
    {
        return;
    }
    if(divoom_pic_phone_manage_get_total_pic_num(album_id) <= 0)
    {
        divoom_pic_album_manage_delete_cover_file(album_id);
    }
}

void divoom_pic_album_manage_clear_empty_album_covers(void)
{
    int i;
    int n;
    int album_ids[DIVOOM_PHOTO_MAXALBUM + 1];

    if(pdivoom_pic_album_manage_handle == NULL)
    {
        return;
    }
    n = 0;
    divoom_system_get_lock(pdivoom_pic_album_manage_handle->lock);
    for(i = 0; i < pdivoom_pic_album_manage_handle->palbum_head.album_num
        && n < (int)(sizeof(album_ids) / sizeof(album_ids[0])); i++)
    {
        album_ids[n++] = pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].id;
    }
    divoom_system_free_lock(pdivoom_pic_album_manage_handle->lock);
    for(i = 0; i < n; i++)
    {
        divoom_pic_album_manage_clear_cover_if_empty(album_ids[i]);
    }
}

/*********************************************************************
  Function:        divoom_pic_album_manage_get_album_id
  Description:     查找相册的ID
  Input:           album_name 相册名称
  Return:          相册ID
**********************************************************************/

int divoom_pic_album_manage_get_album_id(char *album_name)
{
    int i, album_id = 0;

    //只清空当前的相册数据，但是并不释放内存
    divoom_system_get_lock(pdivoom_pic_album_manage_handle->lock);
    for(i = 0; i < pdivoom_pic_album_manage_handle->palbum_head.album_num; i++)
    {
        if(strcmp(album_name, pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].album_name) == 0)
        {
            album_id = pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].id;
            break;
        }
    }
    divoom_system_free_lock(pdivoom_pic_album_manage_handle->lock);
    return album_id;
}

/*********************************************************************
  Function:        divoom_pic_album_manage_add_type
  Description:     增加一个空相册
  Input:           album_name 相册名称
                   album_id 为0，是本地，否则是APP
  Return:          返回相簿ID,如果已经存在，将返回0
**********************************************************************/

int divoom_pic_album_manage_add_type(char *album_name, int album_id, int alarm_type)
{
    int i, new_album_id;
    Pdivoom_pic_album_manage_head_item pnew_album_list;
    Pdivoom_net_command pnet_data;
    Pdivoom_photo_album_album_info pnew_album_info;

    divoom_print("add new album: %d, %d, %s\n", album_id, alarm_type,album_name);
    if(album_id == 0)
    {
        //本地不会生成相簿
        return 0;
        #if 0
        pnet_data = divoom_net_app_send_packet_wait(DIVOOM_NET_COMM_PHOTO_ADD_NEW_ALBUM, DIVOOM_SERVER_TOPIC, album_name);
        if(pnet_data == NULL)
        {
            return 0;
        }
        pnew_album_info = pnet_data->cmd_data;
        new_album_id = pnew_album_info->album_id;
        alarm_type = DIVOOM_PIC_ALBUM_TYPE_NORMAL;
        divoom_net_app_free_packet(pnet_data);
        #endif
    }
    else
    {
        new_album_id = album_id;
    }
    divoom_system_get_lock(pdivoom_pic_album_manage_handle->lock);
    
    pnew_album_list = (Pdivoom_pic_album_manage_head_item)divoom_system_malloc(sizeof(divoom_pic_album_manage_head_item) * (pdivoom_pic_album_manage_handle->palbum_head.album_num + 1));
    if(pdivoom_pic_album_manage_handle->palbum_head.palbum_list)
    {
        memcpy(pnew_album_list, pdivoom_pic_album_manage_handle->palbum_head.palbum_list, sizeof(divoom_pic_album_manage_head_item) *pdivoom_pic_album_manage_handle->palbum_head.album_num);
        divoom_system_free(pdivoom_pic_album_manage_handle->palbum_head.palbum_list);
    }
    pdivoom_pic_album_manage_handle->palbum_head.palbum_list = pnew_album_list;
    pnew_album_list[pdivoom_pic_album_manage_handle->palbum_head.album_num].id = new_album_id;
    pnew_album_list[pdivoom_pic_album_manage_handle->palbum_head.album_num].type = alarm_type;
    strcpy(pnew_album_list[pdivoom_pic_album_manage_handle->palbum_head.album_num].album_name, album_name);
    pdivoom_pic_album_manage_handle->palbum_head.album_num ++;
    divoom_database_manage_insert_album_item(new_album_id, alarm_type, album_name);
    divoom_system_free_lock(pdivoom_pic_album_manage_handle->lock);
    //pdivoom_pic_album_manage_handle->update_flag = 1;
    return new_album_id;
}


/*********************************************************************
  Function:        divoom_pic_album_manage_del_type
  Description:     删除一个相册
  Input:           album_id 相册ID
  Return:          NONE
**********************************************************************/

void divoom_pic_album_manage_del_type(int album_id, int up_server)
{
    uint32 *pic_list;
    int i, pic_num;
    void *palbum_handle;
    Pdivoom_pic_phone_manage_item_offset pic_info;
    Pdivoom_pic_phone_manage_slide_config pslide_config = divoom_pic_phone_manage_get_slide_config();

    if(pslide_config->album_id == album_id)
    {
        pslide_config->album_id = divoom_pic_album_manage_get_album_id_from_type(DIVOOM_PIC_ALBUM_TYPE_ALL);
        pdivoom_pic_album_manage_handle->update_flag = 1;
    }

    //只清空当前的相册数据，但是并不释放内存
    divoom_system_get_lock(pdivoom_pic_album_manage_handle->lock);
    for(i = 0; i < pdivoom_pic_album_manage_handle->palbum_head.album_num; i++)
    {
        if(album_id == pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i].id)
        {
            if(i != pdivoom_pic_album_manage_handle->palbum_head.album_num - 1)
            {
                memmove(&pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i], &pdivoom_pic_album_manage_handle->palbum_head.palbum_list[i + 1], 
                    (pdivoom_pic_album_manage_handle->palbum_head.album_num - i - 1) * sizeof(divoom_pic_album_manage_head_item));
            }
            pdivoom_pic_album_manage_handle->palbum_head.album_num --;
            break;
        }
    }
    divoom_system_free_lock(pdivoom_pic_album_manage_handle->lock);
    divoom_database_manage_delete_album_table(album_id);
    if(up_server && album_id > 3)
    {
        divoom_net_app_send_packet(DIVOOM_NET_COMM_PHOTO_DELETE_ALBUM, DIVOOM_SERVER_TOPIC, &album_id);
    }
}


int divoom_pic_album_manage_find_album_name(void *item1, void *item2)
{
    int *album_id = (int *)item1;
    Pdivoom_pic_album_manage_create_disp_order_ctrl palbum_item = (Pdivoom_pic_album_manage_create_disp_order_ctrl)item2;

    return *album_id == palbum_item->album_id?0:1;
}



int divoom_pic_album_manage_find_album_hanel(void *item1, void *item2)
{
    return item1 == item2?0:1;
}
Pdivoom_pic_album_manage_album_item divoom_pic_album_manage_load_album_item(int album_id)
{
    int i, pic_num;
    uint32 *pic_list;
    Pdivoom_pic_album_manage_album_item palbum_info;
    Pdivoom_pic_album_manage_head_item palbum_head = divoom_pic_album_manage_get_album_info(album_id);
    Pdivoom_pic_phone_manage_slide_config pslide_config = divoom_pic_phone_manage_get_slide_config();


    palbum_info = divoom_system_malloc(sizeof(divoom_pic_album_manage_album_item));
    palbum_info->album_id = album_id;
    palbum_info->album_type = palbum_head->type;

    pic_list = divoom_database_manage_get_all_pic_from_album_table(album_id, &pic_num, pslide_config->disp_order, pslide_config->disp_reverse, 0, 0);
    palbum_info->pic_num = pic_num;
    palbum_info->disp_order = pslide_config->disp_order;
    palbum_info->disp_reverse = pslide_config->disp_reverse;
    palbum_info->cache_num = pic_num;
    if(pic_num == 0)
    {
        palbum_info->pic_list = NULL;
    }
    else
    {
        palbum_info->cache_num += 50;
        palbum_info->pic_list = divoom_system_malloc(palbum_info->cache_num * sizeof(divoom_pic_phone_manage_item_offset));
        for(i = 0; i < pic_num; i++)
        {
            divoom_database_manage_get_pic_info(pic_list[i], &palbum_info->pic_list[i]);
        }
        divoom_system_free(pic_list);
    }

    return palbum_info;
}

/*********************************************************************
  Function:        divoom_pic_album_manage_check_album_cover_exist
  Description:     得到相簿是否存在封面
  Input:           album_id 相册名称
  Return:          1 or 0
**********************************************************************/

int divoom_pic_album_manage_check_album_cover_exist(uint32 album_id)
{
    char pic_path[256];

     sprintf(pic_path, "%s/%d.webp", DIVOOM_ALBUM_COVER_INFO_DIR, album_id);
     return divoom_app_com_check_file_exist(pic_path);
}

/*********************************************************************
  Function:        divoom_pic_album_manage_get_album_disp_rgba_buf
  Description:     得到相簿
  Input:           album_id 相册名称
                   pic_list     相片ID
                   pic_num
  Return:          NONE
**********************************************************************/

int divoom_pic_album_manage_get_album_disp_rgba_buf(uint32 album_id, int width, int height, uint8 *rgb_buf)
{
    int src_w, src_h;
    uint8 * image_read_buf;
    char pic_path[256];

     sprintf(pic_path, "%s/%d.webp", DIVOOM_ALBUM_COVER_INFO_DIR, album_id);
     if(divoom_app_com_check_file_exist(pic_path) == 0)
     {
        if(album_id == divoom_pic_album_manage_get_album_id_from_type(DIVOOM_PIC_ALBUM_TYPE_ALL))
        {
            char default_path[256];
            
            sprintf(default_path, "%s/0.webp", DIVOOM_ALBUM_COVER_INFO_DIR);
            if(divoom_app_com_check_file_exist(default_path) == 0)
            {
                strcpy(pic_path, DIVOOM_EMPTY_ALBUM_PHOTO_NAME);
            }
            else
            {
                sprintf(default_path, "mv %s/0.webp %s/%d.webp\n", DIVOOM_ALBUM_COVER_INFO_DIR, DIVOOM_ALBUM_COVER_INFO_DIR, album_id);
                exec_command_system(default_path);
            }
        }
        else
        {
            strcpy(pic_path, DIVOOM_EMPTY_ALBUM_PHOTO_NAME);
        }
     }
     
     image_read_buf = divoom_rk_mpp_get_jpeg_info(pic_path, &src_w, &src_h);
     if(image_read_buf == NULL)
     {
        return 0;
     }

    {
        char ext_name[16];
        int src_type;

        divoom_app_com_get_file_ex_name(pic_path, ext_name);
        src_type = (strcasecmp(ext_name, "jpg") == 0 ||
            strcasecmp(ext_name, "jpeg") == 0) ? RK_FORMAT_RGB_565 : RK_FORMAT_BGRA_8888;
        divoom_rk_mpp_stretch_image_lvfmt(image_read_buf, src_w, src_h,
            src_type, rgb_buf, width, height, LV_COLOR_FORMAT_RGB565A8);
    }

	 divoom_system_free(image_read_buf);
     return 1;
    
}


/*********************************************************************
  Function:        divoom_pic_album_manage_add_list_to_name_no_refresh
  Description:     向相册中添加多个相片,但不更新相簿显示，应用在多张照片同时发送时
  Input:           album_id 相册名称
                   pic_list     相片ID
                   pic_num
  Return:          NONE
**********************************************************************/

static int divoom_pic_album_manage_add_list_impl(int album_id, uint32 *pic_list,
    int pic_num, int batch_records)
{    
    int i;
    Pdivoom_pic_phone_manage_slide_config pslide_config = divoom_pic_phone_manage_get_slide_config();    
    Pdivoom_pic_album_manage_create_disp_order_ctrl palbum_item;

    if (batch_records && (!pic_list || pic_num <= 0))
        return 0;
    divoom_print("add info: %d, %d\n", album_id, pic_num);
    
    divoom_pic_album_manage_set_album_cover(album_id, pic_list[0], 0, 0);
    if(divoom_pic_album_manage_get_album_id_from_type(DIVOOM_PIC_ALBUM_TYPE_ALL) != album_id)
    {
        if (batch_records)
        {
            if (!divoom_database_manage_add_album_records_batch(album_id, pic_list, pic_num))
                return 0;
        }
        else
        {
            for(i = 0; i < pic_num; i++)
                divoom_database_manage_add_album_record(album_id, pic_list[i]);
        }
    }
    divoom_system_get_lock(pdivoom_pic_album_manage_handle->lock);
    
    palbum_item = divoom_queue_get_cmp(pdivoom_pic_album_manage_handle->album_list_queue, &album_id, divoom_pic_album_manage_find_album_name);
    
    if(palbum_item)
    {
        int total = palbum_item->total_num; /* Count before the newly inserted photos. */
        for(int i = 0; i < pic_num; i++)
        {
            divoom_pic_album_manage_add_pic_id_from_disp_handle(palbum_item, pic_list[i]);
        }
        
        if(total == 0)
        {
            pdivoom_pic_album_manage_handle->update_flag = 1;
        }
    }
    divoom_system_free_lock(pdivoom_pic_album_manage_handle->lock);
    return 1;
}

void divoom_pic_album_manage_add_list_to_name_no_refresh(int album_id,
    uint32 *pic_list, int pic_num)
{
    (void)divoom_pic_album_manage_add_list_impl(album_id, pic_list, pic_num, 0);
}

int divoom_pic_album_manage_add_preload_list(int album_id, uint32 *pic_list, int pic_num)
{
    return divoom_pic_album_manage_add_list_impl(album_id, pic_list, pic_num, 1);
}

/*********************************************************************
  Function:        divoom_pic_album_manage_get_last_album_pic_id
  Description:     得到最后一张照片ID
  Input:           album_id 相册名称
  Return:          NONE
**********************************************************************/

int divoom_pic_album_manage_get_last_album_pic_id(uint32 album_id)
{
    if(album_id == pdivoom_pic_album_manage_handle->last_album_id)
    {
        pdivoom_pic_album_manage_handle->last_album_id = -1;
        return pdivoom_pic_album_manage_handle->last_pic_id;
    }
    return -1;
}


/*********************************************************************
  Function:        divoom_pic_album_manage_add_list_to_name
  Description:     向相册中添加多个相片
  Input:           album_id 相册名称
                   pic_list     相片ID
                   pic_num
  Return:          NONE
**********************************************************************/

void divoom_pic_album_manage_add_list_to_name(uint32 album_id, uint32 *pic_list, int pic_num)
{    
    Pdivoom_pic_phone_manage_slide_config pslide_config = divoom_pic_phone_manage_get_slide_config();

    #if 0
    if(divoom_pic_album_manage_get_album_id_from_type(DIVOOM_PIC_ALBUM_TYPE_ALL) == album_id)
    {
        //如果是添加到全部，将忽略
        return;
    }
    #endif
    
    divoom_pic_album_manage_add_list_to_name_no_refresh(album_id, pic_list, pic_num);
}


/*********************************************************************
  Function:        divoom_pic_album_manage_add_item
  Description:     向相册中添加一个相片
  Input:           album_name 相册名称
                   pic_id     相片ID
  Return:          NONE
**********************************************************************/

void divoom_pic_album_manage_add_pic_to_album(int album_id, uint32 pic_id)
{    
    Pdivoom_pic_phone_manage_slide_config pslide_config = divoom_pic_phone_manage_get_slide_config();
    
    divoom_pic_album_manage_add_list_to_name(album_id, &pic_id, 1);
    if(pslide_config->album_id == album_id)
    {
        pdivoom_pic_album_manage_handle->update_flag = 1;
    }
}

/*********************************************************************
  Function:        divoom_pic_album_manage_check_remove
  Description:     检查移除的相片是否在当前播放的相簿中，如果在，将添加到移除队列
  Input:           album_id 
                   pic_list     
                   pic_num
  Return:          NONE
**********************************************************************/

int divoom_pic_album_manage_check_remove(int album_id, uint32 *pic_list, int pic_num)
{
    int ret = 0;
    Pdivoom_pic_album_manage_delete_info_item pitem_info;
    Pdivoom_pic_phone_manage_slide_config pslide_config = divoom_pic_phone_manage_get_slide_config();
    
    divoom_get_lvgl_lock();

    if(pslide_config->album_id == album_id)
    {
        if(divoom_menu_control_get_active_menu_id() == DIVOOM_MENU_CONTROL_ID_PHOTO_SLIDE_EFFECT)
        {
            pitem_info = divoom_system_malloc(sizeof(divoom_pic_album_manage_delete_info_item) + pic_num * sizeof(uint32));
            if(pitem_info)
            {
                pitem_info->pic_list = (uint32 *)&pitem_info[1];
                pitem_info->type = 0;
                pitem_info->album_id = album_id;
                pitem_info->pic_num = pic_num;
                memcpy(pitem_info->pic_list, pic_list, pic_num * sizeof(uint32));
                divoom_queue_push(pdivoom_pic_album_manage_handle->app_remove_queue, pitem_info);
                ret = 1;
            }
        }
    }
    divoom_free_lvgl_lock();
    return ret;
}


/*********************************************************************
  Function:        divoom_pic_album_manage_del_list_from_id
  Description:     检查删除相片是否在当前播放的相簿中
  Input:           album_id 
                   pic_list     
                   pic_num
  Return:          NONE
**********************************************************************/

int divoom_pic_album_manage_check_delete_pic(int album_id, uint32 *pic_list, int pic_num)
{
    int ret = 0;
    Pdivoom_pic_album_manage_delete_info_item pitem_info;
    Pdivoom_pic_phone_manage_slide_config pslide_config = divoom_pic_phone_manage_get_slide_config();
    
    divoom_get_lvgl_lock();
    
    if(divoom_menu_control_get_active_menu_id() == DIVOOM_MENU_CONTROL_ID_PHOTO_SLIDE_EFFECT)
    {
        if(pslide_config->album_id == album_id)
        {
            ret = 1;
        }
        else
        {
            //检查所有的好照片是否在当前播放的列表中
            for(int i=  0; i < pic_num; i++)
            {
                if(divoom_database_manage_check_album_include_pic(pslide_config->album_id, pic_list[i]))
                {
                    ret = 1;
                    break;
                }
            }
        }
    }
    if(ret)
    {
        pitem_info = divoom_system_malloc(sizeof(divoom_pic_album_manage_delete_info_item) + pic_num * sizeof(uint32));
        if(pitem_info)
        {
            pitem_info->pic_list = (uint32 *)&pitem_info[1];
            pitem_info->type = 1;
            pitem_info->album_id = album_id;
            pitem_info->pic_num = pic_num;
            memcpy(pitem_info->pic_list, pic_list, pic_num * sizeof(uint32));
            divoom_queue_push(pdivoom_pic_album_manage_handle->app_remove_queue, pitem_info);
        }
    }
    divoom_free_lvgl_lock();
    return ret;
}



/*********************************************************************
  Function:        divoom_pic_album_manage_get_remove_info
  Description:     得到移除队列相关信息
  Input:           none
  Return:          handle
**********************************************************************/

Pdivoom_pic_album_manage_delete_info_item divoom_pic_album_manage_get_remove_info(void)
{
    return divoom_queue_pop(pdivoom_pic_album_manage_handle->app_remove_queue);
}

/*********************************************************************
  Function:        divoom_pic_album_manage_do_remove_info
  Description:     处理移除队列动作
  Input:           none
  Return:          none
**********************************************************************/

void divoom_pic_album_manage_do_remove_info(Pdivoom_pic_album_manage_delete_info_item pitem_info)
{
    divoom_sys_msg_info model_msg = {0};
    
	model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP; 
	model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_REMOVE_POHTO_INFO;
    model_msg.msg_type = 1;
    model_msg.data = (uint8 *)pitem_info;
	divoom_sys_model_crl_msg_push(&model_msg);
}

/*********************************************************************
  Function:        divoom_pic_album_manage_del_list_from_id
  Description:     通过相册ID向相册中删除相片
  Input:           album_name 相册名称
                   pic_id     相片ID
  Return:          NONE
**********************************************************************/

void divoom_pic_album_manage_del_list_from_id(int album_id, uint32 *pic_list, int pic_num)
{
    int i, j, find_flag;
    Pdivoom_pic_album_manage_create_disp_order_ctrl palbum_item;

    if(album_id == 0)
    {
        return;
    }
    divoom_pic_phone_manage_remove_pic_list(pic_list, pic_num);
    divoom_system_get_lock(pdivoom_pic_album_manage_handle->lock);
    
    palbum_item = divoom_queue_get_cmp(pdivoom_pic_album_manage_handle->album_list_queue, &album_id, divoom_pic_album_manage_find_album_name);
    
    if(palbum_item == NULL)
    {
        divoom_system_free_lock(pdivoom_pic_album_manage_handle->lock);
        return;
    }
    for(j = 0; j < pic_num; j ++)
    {
        divoom_pic_album_manage_del_disp_order_pic_id(palbum_item, pic_list[j]);
    }
    if(divoom_pic_album_manage_get_disp_order_pic_num(palbum_item) == 0)
    {
        pdivoom_pic_album_manage_handle->update_flag = 1;
    }
    divoom_system_free_lock(pdivoom_pic_album_manage_handle->lock);
}


/*********************************************************************
  Function:        divoom_pic_album_manage_remove_list_from_id
  Description:     通过相册ID从相册中移除相片
  Input:           album_id 相册ID
                   pic_list     相片ID
                   pic_num
  Return:          NONE
**********************************************************************/

void divoom_pic_album_manage_remove_list_from_id(int album_id, uint32 *pic_list, int pic_num)
{
    int i, j, find_flag;
    Pdivoom_pic_album_manage_create_disp_order_ctrl palbum_item;
    Pdivoom_pic_phone_manage_slide_config pslide_config = divoom_pic_phone_manage_get_slide_config();

    if(album_id == 0 || pic_num == 0)
    {
        return;
    }
    divoom_system_get_lock(pdivoom_pic_album_manage_handle->lock);
    
    palbum_item = divoom_queue_get_cmp(pdivoom_pic_album_manage_handle->album_list_queue, &album_id, divoom_pic_album_manage_find_album_name);
    
    if(palbum_item == NULL)
    {
        divoom_system_free_lock(pdivoom_pic_album_manage_handle->lock);
        //divoom_pic_phone_manage_remove_pic_list(pic_list, pic_num);
        for(i = 0; i < pic_num; i++)
        {
            divoom_database_manage_remove_album_record(album_id, pic_list[i]);
        }
        divoom_pic_album_manage_clear_cover_if_empty(album_id);
        return;
    }
    for(j = 0; j < pic_num; j ++)
    {
        divoom_pic_album_manage_del_disp_order_pic_id(palbum_item, pic_list[j]);
        divoom_database_manage_remove_album_record(album_id, pic_list[j]);
    }
    
    if(divoom_pic_album_manage_get_disp_order_pic_num(palbum_item) == 0)
    {
        pdivoom_pic_album_manage_handle->update_flag = 1;
    }
    divoom_system_free_lock(pdivoom_pic_album_manage_handle->lock);
    divoom_pic_album_manage_clear_cover_if_empty(album_id);
}

/*********************************************************************
  Function:        divoom_pic_album_manage_del_item_from_id
  Description:     通过相册ID向相册中删除一个相片
  Input:           album_name 相册名称
                   pic_id     相片ID
  Return:          NONE
**********************************************************************/

void divoom_pic_album_manage_del_item_from_id(int album_id, int pic_id)
{
    divoom_pic_album_manage_del_list_from_id(album_id, &pic_id, 1);
}

/*********************************************************************
  Function:        divoom_pic_album_manage_set_refresh_album_info
  Description:     设置相簿是否发生变化标记
  Input:           none
  Return:          none
**********************************************************************/

void divoom_pic_album_manage_set_refresh_album_info(void)
{
    pdivoom_pic_album_manage_handle->update_flag = 1;
    
}


/*********************************************************************
  Function:        divoom_pic_album_manage_get_refresh_album_info
  Description:     获取相簿是否发生变化，并清除变化标记
  Input:           none
  Return:          1 or 0
**********************************************************************/

int divoom_pic_album_manage_get_refresh_album_info(void)
{
    int ret = pdivoom_pic_album_manage_handle->update_flag;

    pdivoom_pic_album_manage_handle->update_flag = 0;
    return ret;
}


#define divoom_pic_album_manage_PRE_READ_NUM   60   //预读取文件数目


/*********************************************************************
  Function:        divoom_pic_album_manage_pre_read_cache
  Description:     预读相薄中的照片
  Input:           phandle 相册名称
                   list_id
                   rgb_buf
                   pic_id
                   
  Return:          none
**********************************************************************/
void divoom_pic_album_manage_pre_read_cache(void *phandle, int list_id, int pic_id)
{
    divoom_thumb_cache_pre_read(list_id, pic_id);
}


/*********************************************************************
  Function:        divoom_pic_album_manage_clear_cache
  Description:     清除读CACHE
  Input:           phandle 相册名称
                   list_id
                   
  Return:          none
**********************************************************************/
void divoom_pic_album_manage_clear_cache(void *phandle, int list_id)
{
    divoom_thumb_cache_clear_wait(list_id);
}


/*********************************************************************
  Function:        divoom_pic_album_manage_read_pic
  Description:     读相册图片，这个模块不能阻塞，做异步通知
  Input:           phandle 相册名称
                   list_id
                   pic_id
                   asyn_flag   是否异步
                   rgb_buf
                   pcallback
                   para
                   dir          1:向后读，-1向前读
                   
  Return:          如果读成功返回1，异步通知返回0
**********************************************************************/
int divoom_pic_album_manage_read_pic(void *phandle, int list_id, int pic_id, int asyn_flag,uint8 *rgb_buf, int image_w, int image_h,DIVOOM_PIC_ALBUM_MANAGE_READ_OVER *pcallback, void *para)
{
    return divoom_thumb_cache_read_pic(list_id, pic_id, asyn_flag, rgb_buf, image_w, image_h, pcallback, para);
}



typedef int divoom_pic_album_manage_disp_order_compare_func(Pdivoom_pic_phone_manage_item_offset item1, Pdivoom_pic_phone_manage_item_offset item2);

//拍摄时间从新到旧
int divoom_pic_album_manage_disp_order_compare_photo_time_order(Pdivoom_pic_phone_manage_item_offset item1, Pdivoom_pic_phone_manage_item_offset item2)
{
    if(item1->photo_time > item2->photo_time)
    {
        return 1;
    }
    else if(item1->photo_time < item2->photo_time)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

//拍摄时间从旧到到
int divoom_pic_album_manage_disp_order_compare_photo_time_reverse(Pdivoom_pic_phone_manage_item_offset item1, Pdivoom_pic_phone_manage_item_offset item2)
{
    if(item1->photo_time > item2->photo_time)
    {
        return -1;
    }
    else if(item1->photo_time < item2->photo_time)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}
//储存时间从新到旧

int divoom_pic_album_manage_disp_order_compare_save_time_order(Pdivoom_pic_phone_manage_item_offset item1, Pdivoom_pic_phone_manage_item_offset item2)
{
    if(item1->save_time > item2->save_time)
    {
        return 1;
    }
    else if(item1->save_time < item2->save_time)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}
//储存时间从旧到到

int divoom_pic_album_manage_disp_order_compare_save_time_reverse(Pdivoom_pic_phone_manage_item_offset item1, Pdivoom_pic_phone_manage_item_offset item2)
{
    if(item1->save_time > item2->save_time)
    {
        return -1;
    }
    else if(item1->save_time < item2->save_time)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

/*********************************************************************
  Function:        divoom_pic_album_manage_init_disp_order_info
  Description:     生成位置列表
  Input:           pdisp_list 显示列表
                   pic_list      输出的位置列表 
                   pic_num    
                   pcmp_func     比较函数
  Return:          none
**********************************************************************/

void divoom_pic_album_manage_init_disp_order_info(Pdivoom_pic_phone_manage_item_offset pdisp_list, uint32 *pic_list, int pic_num, divoom_pic_album_manage_disp_order_compare_func *pcmp_func)
{
    //简单做个冒泡排序,大到小
    int i, j , tmp, ret;

    for(i = 0; i < pic_num; i++)
    {
        for(j = i + 1; j < pic_num; j ++)
        {
            ret = pcmp_func(&pdisp_list[pic_list[i]], &pdisp_list[pic_list[j]]);
            if(ret > 0)
            {
                tmp = pic_list[i];
                pic_list[i] = pic_list[j];
                pic_list[j] = tmp;
            }
        }
    }
}
void divoom_pic_album_manage_create_random_array_list(uint32 *array_list, int num, int first_pos)
{
    int i, tmp, j, empty;
    uint8 *flag = divoom_system_malloc(num);
    memset(flag, 0, num);

    for(i = 0; i < num; i ++)
    {
        tmp = rand()%(num - i);
        if(i == 0 && tmp == first_pos && num > 1)
        {
            tmp = (first_pos + num -1)%num;
        }
        for(j = 0, empty = 0; j < num; j ++)
        {
            if(flag[j])
            {
                continue;
            }
            if(empty == tmp)
            {
                flag[j] = 1;
                array_list[i] = j;
                break;
            }
            empty ++;
        }
    }
    divoom_system_free(flag);
}


/*********************************************************************
  Function:        divoom_pic_album_manage_create_disp_order_handle
  Description:     创建一个显示顺序管理的句柄,这个函数会将pdisp_list这个也释放
  Input:           album_id         相簿ID
                   disp_order      //显示顺序 
                   disp_reverse    //显示顺序反转标记 //显示顺序反转标记0;从新到旧，1：从旧到新
                   movie_flag      是否播放电影
  Return:          返回句柄
**********************************************************************/

static void *divoom_pic_album_manage_create_disp_order_handle_ex(int album_id, uint8 disp_order,
    uint8 disp_reverse, int movie_flag, int from_start)
{
    int last_pic_id;
    Pdivoom_pic_album_manage_create_disp_order_ctrl phandle;

    phandle = divoom_system_malloc(sizeof(divoom_pic_album_manage_create_disp_order_ctrl));
    if (phandle == NULL)
    {
        return NULL;
    }
    memset(phandle, 0, sizeof(*phandle));
    phandle->disp_order = disp_order;
    phandle->disp_reverse = disp_reverse;
    phandle->movie_flag = movie_flag;
    if(album_id == 0)
    {
        album_id = divoom_pic_album_manage_get_album_id_from_type(DIVOOM_PIC_ALBUM_TYPE_ALL);
    }
    phandle->total_num = divoom_database_manage_get_total_pic_num(album_id, disp_order, disp_reverse, -1, 0, movie_flag);
    phandle->album_id = album_id;
    phandle->last_pic_id = -1;
    /* Keep only the count; fetch IDs when playback requests a position. */
    if(!from_start && disp_order == DIVOOM_PIC_PHOTO_SLIDE_DISP_RANDOM)
    {
        last_pic_id = divoom_pic_album_manage_get_last_play_album_pic_id(album_id, NULL);
        if(last_pic_id > 0)
        {
            phandle->last_pic_id = last_pic_id;
        }
    }
    divoom_system_get_lock(pdivoom_pic_album_manage_handle->lock);
    divoom_queue_push(pdivoom_pic_album_manage_handle->album_list_queue, phandle);
    divoom_system_free_lock(pdivoom_pic_album_manage_handle->lock);
    return phandle;
}

void *divoom_pic_album_manage_create_disp_order_handle(int album_id, uint8 disp_order, uint8 disp_reverse, int movie_flag)
{
    return divoom_pic_album_manage_create_disp_order_handle_ex(album_id, disp_order, disp_reverse, movie_flag, 0);
}

void *divoom_pic_album_manage_create_disp_order_handle_from_start(int album_id, uint8 disp_order, uint8 disp_reverse, int movie_flag)
{
    return divoom_pic_album_manage_create_disp_order_handle_ex(album_id, disp_order, disp_reverse, movie_flag, 1);
}

/*********************************************************************
  Function:        divoom_pic_album_manage_load_more_stricker_pic_info
  Description:     墙贴效果的加载图片
  Input:           handle
                   
  Return:          none
**********************************************************************/

void divoom_pic_album_manage_load_more_stricker_pic_info(void *handle,  int width_flag)
{
    int last_pic_id;
    divoom_pic_phone_manage_item_offset pic_info;
    Pdivoom_pic_album_manage_create_disp_order_ctrl phandle = (Pdivoom_pic_album_manage_create_disp_order_ctrl)handle;
    Pdivoom_pic_phone_manage_slide_config pslide_config = divoom_pic_phone_manage_get_slide_config();

    if(width_flag)
    {
        if(phandle->width_total_num <= phandle->width_pic_num)
        {
            return;
        }
        
        if(phandle->width_read_pos < phandle->width_pic_num)
        {
            return;
        }
        //要加载其它的
        phandle->width_read_pos = 0;
        last_pic_id = phandle->width_pic_list[phandle->width_pic_num - 1];
        divoom_database_manage_get_pic_info(last_pic_id, &pic_info);
        divoom_system_free(phandle->width_pic_list);
        
        phandle->width_pic_num = DIVOOM_PIC_MANAGE_LOAD_MAX_PIC_NUM;
        phandle->width_pic_list = divoom_database_manage_get_all_pic_id(phandle->album_id, &phandle->width_pic_num, pslide_config->disp_order != DIVOOM_PIC_PHOTO_SLIDE_DISP_PHTOT_TIME?DIVOOM_PIC_PHOTO_SLIDE_DISP_SAVE_TIME:pslide_config->disp_order, pslide_config->disp_reverse, phandle->disp_order!=DIVOOM_PIC_PHOTO_SLIDE_DISP_PHTOT_TIME?pic_info.save_time:pic_info.photo_time, 1, 0);
        if(phandle->width_pic_num == 0)
        {
            divoom_print("we will load from start!\n");
            phandle->width_pic_num = DIVOOM_PIC_MANAGE_LOAD_MAX_PIC_NUM;
            phandle->width_pic_list = divoom_database_manage_get_all_pic_id(phandle->album_id, &phandle->width_pic_num, pslide_config->disp_order != DIVOOM_PIC_PHOTO_SLIDE_DISP_PHTOT_TIME?DIVOOM_PIC_PHOTO_SLIDE_DISP_SAVE_TIME:pslide_config->disp_order, pslide_config->disp_reverse, -1, 1, 0);
        }
        
        divoom_print("we load width pic!:  %d, %d\n", phandle->width_pic_num, phandle->width_total_num);
#if 0
        for(int i = 0; i < phandle->width_pic_num; i++)
        {
            divoom_print("pic: %d\n", phandle->width_pic_list[i]);
        }
#endif
        
        divoom_random_destroy(phandle->psticker_width_rand_handle);
        if(phandle->width_pic_num)
        {
            phandle->psticker_width_rand_handle = divoom_random_create(phandle->width_pic_num);
        }
        else
        {
            phandle->psticker_width_rand_handle = NULL;
        }
    }
    else
    {
        if(phandle->high_total_num <= phandle->high_pic_num)
        {
            return;
        }
        
        if(phandle->high_read_pos < phandle->high_pic_num)
        {
            return;
        }
        //要加载其它的
        phandle->high_read_pos = 0;
        last_pic_id = phandle->high_pic_list[phandle->high_pic_num - 1];
        divoom_database_manage_get_pic_info(last_pic_id, &pic_info);
        divoom_system_free(phandle->high_pic_list);
        
        phandle->high_pic_num = DIVOOM_PIC_MANAGE_LOAD_MAX_PIC_NUM;
        phandle->high_pic_list = divoom_database_manage_get_all_pic_id(phandle->album_id, &phandle->high_pic_num, pslide_config->disp_order != DIVOOM_PIC_PHOTO_SLIDE_DISP_PHTOT_TIME?DIVOOM_PIC_PHOTO_SLIDE_DISP_SAVE_TIME:pslide_config->disp_order, pslide_config->disp_reverse, phandle->disp_order!=DIVOOM_PIC_PHOTO_SLIDE_DISP_PHTOT_TIME?pic_info.save_time:pic_info.photo_time, 2, 0);
        if(phandle->high_pic_num == 0)
        {
            divoom_print("we will load from start!\n");
            phandle->high_pic_num = DIVOOM_PIC_MANAGE_LOAD_MAX_PIC_NUM;
            phandle->high_pic_list = divoom_database_manage_get_all_pic_id(phandle->album_id, &phandle->high_pic_num, pslide_config->disp_order != DIVOOM_PIC_PHOTO_SLIDE_DISP_PHTOT_TIME?DIVOOM_PIC_PHOTO_SLIDE_DISP_SAVE_TIME:pslide_config->disp_order, pslide_config->disp_reverse, -1, 2, 0);
        }
        divoom_print("we load high pic!:  %d, %d\n", phandle->high_pic_num, phandle->high_total_num);
        #if 0
        for(int i = 0; i < phandle->high_pic_num; i++)
        {
            divoom_print("pic: %d\n", phandle->high_pic_list[i]);
        }
        #endif
        divoom_random_destroy(phandle->psticker_high_rand_handle);
        if(phandle->high_pic_num)
        {
            phandle->psticker_high_rand_handle = divoom_random_create(phandle->high_pic_num);
        }
        else
        {
            phandle->psticker_high_rand_handle = NULL;
        }
    }
}

/*********************************************************************
  Function:        divoom_pic_album_manage_get_first_play_info
  Description:     获取第一个播放的信息
  Input:           handle         
                   pic_id
  
  Return:          位置
**********************************************************************/

int divoom_pic_album_manage_get_first_play_info(void *handle, int *pic_id)
{
    int last_pic_id, pic_pos;
    Pdivoom_pic_album_manage_create_disp_order_ctrl phandle = (Pdivoom_pic_album_manage_create_disp_order_ctrl)handle;

    last_pic_id = divoom_pic_album_manage_get_last_play_album_pic_id(phandle->album_id, NULL);
    if(last_pic_id == 0)
    {
        pic_pos = 0;
    }
    else
    {
        if(!divoom_pic_album_manage_get_pic_info_from_pic_id(phandle, last_pic_id, &pic_pos, NULL))
        {
            last_pic_id = 0;
        }
    }
    if(pic_id)
    {
        *pic_id = last_pic_id;
    }
    return pic_pos;
}

/*********************************************************************
  Function:        divoom_pic_album_manage_remove_pic_id_from_disp_handle
  Description:     从列表中删除某个照片
  Input:           handle 显示列表
                   pic_id
  Return:          none
**********************************************************************/

void divoom_pic_album_manage_remove_pic_id_from_disp_handle(void *handle, int pic_id)
{
    divoom_pic_album_manage_del_disp_order_pic_id(handle, pic_id);
}

/*********************************************************************
  Function:        divoom_pic_album_manage_add_pic_id_from_disp_handle
  Description:     向列表中增加某个照片
  Input:           handle 显示列表
                   pic_id
  Return:          none
**********************************************************************/

void divoom_pic_album_manage_add_pic_id_from_disp_handle(void *handle, int pic_id)
{
    Pdivoom_pic_album_manage_create_disp_order_ctrl phandle = handle;
    if(phandle == NULL)
    {
        return;
    }
    if(phandle->prand_handle)
    {
        divoom_random_destroy(phandle->prand_handle);
        phandle->prand_handle = NULL;
    }
    phandle->last_pic_id = pic_id;
}

/*********************************************************************
  Function:        divoom_pic_album_manage_get_pic_info_from_pic_id
  Description:     通过照片ID获取照片相关信息
  Input:           handle 
                   pic_id
  Return:          none
**********************************************************************/

int divoom_pic_album_manage_get_pic_info_from_pic_id(void *handle, int pic_id, int *pic_pos, Pdivoom_pic_phone_manage_item_offset pic_info)
{
    int pos;
    Pdivoom_pic_album_manage_create_disp_order_ctrl phandle = handle;

    if(pic_pos)
    {
        *pic_pos = 0;
    }
    if(pic_info)
    {
        memset(pic_info, 0, sizeof(*pic_info));
    }
    if(phandle == NULL || pic_id <= 0)
    {
        return 0;
    }
    pos = divoom_database_manage_get_disp_order_pic_pos(phandle->album_id,
        phandle->disp_order, phandle->disp_order == DIVOOM_PIC_PHOTO_SLIDE_DISP_RANDOM ? 0 : phandle->disp_reverse,
        phandle->movie_flag, pic_id);
    if(pos < 0)
    {
        return 0;
    }
    if(pic_info && !divoom_database_manage_get_pic_info(pic_id, pic_info))
    {
        return 0;
    }
    if(pic_pos)
    {
        *pic_pos = pos;
    }
    return 1;
}

/*********************************************************************
  Function:        divoom_pic_album_manage_get_disp_order_pic_num
  Description:     得到照片数目
  Input:           handle 显示列表
  Return:          文件信息
**********************************************************************/

int divoom_pic_album_manage_get_disp_order_pic_num(void *handle)
{
    Pdivoom_pic_album_manage_create_disp_order_ctrl phandle = handle;
    int total_num;
    if(phandle == NULL)
    {
        return 0;
    }
    /* Notifications can precede the DB mutation or reach a handle twice. */
    total_num = divoom_database_manage_get_total_pic_num(phandle->album_id,
        phandle->disp_order, phandle->disp_reverse, -1, 0, phandle->movie_flag);
    if(total_num != phandle->total_num && phandle->prand_handle)
    {
        divoom_random_destroy(phandle->prand_handle);
        phandle->prand_handle = NULL;
    }
    phandle->total_num = total_num;
    return total_num;
}

/*********************************************************************
  Function:        divoom_pic_album_manage_del_disp_order_pic_id
  Description:     删除某个位置的图片
  Input:           handle 显示列表
                   pos      //显示顺序
  Return:          文件信息
**********************************************************************/

void divoom_pic_album_manage_del_disp_order_pic_id(void *handle, int pic_id)
{
    Pdivoom_pic_album_manage_create_disp_order_ctrl phandle = handle;
    if(phandle == NULL)
    {
        return;
    }
    if(phandle->last_pic_id == pic_id)
    {
        phandle->last_pic_id = -1;
    }
    if(phandle->prand_handle)
    {
        divoom_random_destroy(phandle->prand_handle);
        phandle->prand_handle = NULL;
    }
}

/*********************************************************************
  Function:        divoom_pic_album_manage_get_disp_last_pic_id
  Description:     获取最后一张要播放的照片
  Input:           handle 显示列表
  Return:          最后照片的ID
**********************************************************************/

int divoom_pic_album_manage_get_disp_last_pic_id(void *handle)
{
    Pdivoom_pic_album_manage_create_disp_order_ctrl phandle = (Pdivoom_pic_album_manage_create_disp_order_ctrl)handle;

    return phandle->last_pic_id;
}

/*********************************************************************
  Function:        divoom_pic_album_manage_get_disp_order_pic_id
  Description:     从显示顺序管理的句柄里得到显示的位置的照片ID
  Input:           handle 显示列表
                   dir      //0:向后取，1：向前取
  Return:          文件信息
**********************************************************************/

int divoom_pic_album_manage_get_disp_order_pic_id(void *handle, int pos, Pdivoom_pic_phone_manage_item_offset pic_info)
{
    int pic_id;
    Pdivoom_pic_album_manage_create_disp_order_ctrl phandle = handle;

    if(pic_info)
    {
        memset(pic_info, 0, sizeof(*pic_info));
    }
    if(phandle == NULL || divoom_pic_album_manage_get_disp_order_pic_num(handle) <= 0)
    {
        return 0;
    }
    if(phandle->last_pic_id > 0)
    {
        pic_id = phandle->last_pic_id;
        phandle->last_pic_id = -1;
        if(divoom_pic_album_manage_get_pic_info_from_pic_id(handle, pic_id, NULL, pic_info))
        {
            return pic_id;
        }
    }
    pos %= phandle->total_num;
    if(pos < 0)
    {
        pos += phandle->total_num;
    }
    if(phandle->disp_order == DIVOOM_PIC_PHOTO_SLIDE_DISP_RANDOM)
    {
        if(phandle->prand_handle == NULL)
        {
            phandle->prand_handle = divoom_random_create(phandle->total_num);
        }
        if(phandle->prand_handle == NULL)
        {
            return 0;
        }
        pos = divoom_random_get(phandle->prand_handle);
    }
    pic_id = divoom_database_manage_get_disp_order_pic_id(phandle->album_id,
        phandle->disp_order, phandle->disp_order == DIVOOM_PIC_PHOTO_SLIDE_DISP_RANDOM ? 0 : phandle->disp_reverse,
        phandle->movie_flag, pos);
    if(pic_id <= 0 || (pic_info && !divoom_database_manage_get_pic_info(pic_id, pic_info)))
    {
        return 0;
    }
    return pic_id;
}

/*********************************************************************
  Function:        divoom_pic_album_manage_get_stricker_disp_order_pic_id
  Description:     从显示顺序管理的句柄里得到显示的位置的照片ID
  Input:           handle 显示列表
                   pos      //显示顺序
                   width_flag  1:宽的，0：长的
  Return:          文件信息
**********************************************************************/

int divoom_pic_album_manage_get_stricker_disp_order_pic_id(void *handle, int pos, int width_flag, Pdivoom_pic_phone_manage_item_offset pic_info)
{
    int ramdon, pic_id;
    Pdivoom_pic_album_manage_create_disp_order_ctrl phandle = (Pdivoom_pic_album_manage_create_disp_order_ctrl)handle;

    if(handle == NULL)
    {
        divoom_print("it is null!\n");
        return 0;
    }
    if(phandle->width_pic_num == 0 && phandle->high_pic_num == 0)
    {
        divoom_print("it is er!\n");
        return 0;
    }
    divoom_pic_album_manage_load_more_stricker_pic_info(handle, width_flag);
    if(width_flag)
    {
        divoom_pic_album_manage_get_stricker_disp_order_pic_id_get_width_table:
        if(phandle->width_pic_num == 0)
        {
            if(phandle->high_pic_num == 0)
            {
                divoom_print("it is er!\n");
                return 0;
            }
            goto divoom_pic_album_manage_get_stricker_disp_order_pic_id_get_high_table;
        }
        else
        {
            if(phandle->last_width_pic_id > 0)
            {
                pic_id = phandle->last_width_pic_id;
                phandle->last_width_pic_id = -1;
            }
            else
            {
                ramdon = divoom_random_get(phandle->psticker_width_rand_handle);
                pic_id = phandle->width_pic_list[ramdon];
            }
            if(pic_info)
            {
                divoom_database_manage_get_pic_info(pic_id, pic_info);
            }
            phandle->width_read_pos ++;
            return pic_id;
        }
    }
    else
    {
        divoom_pic_album_manage_get_stricker_disp_order_pic_id_get_high_table:
        if(phandle->high_pic_num == 0)
        {
            if(phandle->width_pic_num == 0)
            {
                divoom_print("it is er!\n");
                return 0;
            }
            goto divoom_pic_album_manage_get_stricker_disp_order_pic_id_get_width_table;
        }
        else
        {
            if(phandle->last_high_pic_id > 0)
            {
                pic_id = phandle->last_high_pic_id;
                phandle->last_high_pic_id = -1;
            }
            else
            {
                ramdon = divoom_random_get(phandle->psticker_high_rand_handle);
                pic_id = phandle->high_pic_list[ramdon];
            }
            if(pic_info)
            {
                divoom_database_manage_get_pic_info(pic_id, pic_info);
            }
            phandle->high_read_pos ++;
            return pic_id;
        }
    }
    return 0;
}


/*********************************************************************
  Function:        divoom_pic_album_manage_destory_disp_order_handle
  Description:     释放句柄资源
  Input:           handle 显示列表
  Return:          none
**********************************************************************/

void divoom_pic_album_manage_destory_disp_order_handle(void *handle)
{
    Pdivoom_pic_album_manage_create_disp_order_ctrl phandle = (Pdivoom_pic_album_manage_create_disp_order_ctrl)handle;

    if(phandle == NULL)
    {
        return;
    }
    divoom_system_get_lock(pdivoom_pic_album_manage_handle->lock);
    
    void *tmp = divoom_queue_pop_cmp(pdivoom_pic_album_manage_handle->album_list_queue, &phandle->album_id, divoom_pic_album_manage_find_album_name);
    divoom_system_free_lock(pdivoom_pic_album_manage_handle->lock);
    if(phandle->width_pic_list)
    {
        divoom_system_free(phandle->width_pic_list);
    }
    if(phandle->high_pic_list)
    {
        divoom_system_free(phandle->high_pic_list);
    }
    if(phandle->prand_handle)
    {
        divoom_random_destroy(phandle->prand_handle);
    }
    if(phandle->psticker_high_rand_handle)
    {
        divoom_random_destroy(phandle->psticker_high_rand_handle);
    }
    if(phandle->psticker_width_rand_handle)
    {
        divoom_random_destroy(phandle->psticker_width_rand_handle);
    }
    divoom_system_free(phandle);
}



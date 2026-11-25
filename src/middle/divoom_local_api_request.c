#include "divoom.h"
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

typedef struct _divoom_local_api_picture_info
{
    int user_id;
    int album_id;
    int photo_x;
    int photo_y;
    int photo_w;
    int photo_h;
    char photo_title[256];
}divoom_local_api_picture_info, *Pdivoom_local_api_picture_info;

void divoom_local_api_get_photo_info(Pdivoom_local_api_picture_info photo_info, char *json_str)
{
	divoom_cJSON *root = NULL;
    
	root = divoom_cJSON_Parse(json_str);
    if(root == NULL)
    {
        return;
    }
    photo_info->album_id = divoom_cJSON_GetObjectInt(root, "AlbumId");
    photo_info->user_id = divoom_cJSON_GetObjectInt(root, "UserId");
    photo_info->photo_x = divoom_cJSON_GetObjectInt(root, "PhotoX");
    photo_info->photo_y = divoom_cJSON_GetObjectInt(root, "PhotoY");
    photo_info->photo_w = divoom_cJSON_GetObjectInt(root, "PhotoWidth");
    photo_info->photo_h = divoom_cJSON_GetObjectInt(root, "PhotoHeight");
    divoom_cJSON_GetObjectString(root, "PhotoTitle",photo_info->photo_title);    
    divoom_cJSON_Delete(root);
}

/**
 * NAME         divoom_local_api_request_do
 * @BRIEF	APP这边的文件发送参数与局域网的参数请求
 * @PARAM	flag    0:APP发送文件 1：局域网请求
                    
 * @RETURN	none
 * @RETVAL	
 */



void divoom_local_api_request_do(int flag)
{
    int file_len;
    char *file_buf;
    char file_name[128], path_name[64], dest_dir_name[64],dest_name[128];
    int  pic_list[64], pic_num = 0, cur_album_id = 0, pic_id;
    DIR *pic_dir_handle;
    const divoom_port_dirent_t *dirp;
    struct stat file_info;
    time_t cur_time;
	struct tm time_now; 
	Pdivoom_net_command pcmd_info;
    divoom_local_api_picture_info photo_info;


    if(flag == 0)
    {
        pic_dir_handle = divoom_port_dir_open(DIVOOM_SD_APP_PARA_DIR);
        strcpy(path_name, DIVOOM_SD_APP_PARA_DIR);
    }
    else
    {
        pic_dir_handle = divoom_port_dir_open(DIVOOM_SD_APP_API_DIR);
        strcpy(path_name, DIVOOM_SD_APP_API_DIR);
    }
    if(pic_dir_handle == NULL)
    {
        return;
    }
    
    cur_time = time(0);
	localtime_r(&cur_time, &time_now);
    time_now.tm_year += 1900;
	time_now.tm_mon ++;

    if(flag == 0)
    {
        sprintf(dest_dir_name, "%s/%d%02d",DIVOOM_SD_APP_PIC_DIR, time_now.tm_year, time_now.tm_mon);
        divoom_app_com_mk_dir(dest_dir_name);
    }
    
    while((dirp = divoom_port_dir_read(pic_dir_handle)) != NULL)
    {
        divoom_print("find: type: %d, %s\n", dirp->is_dir, dirp->name);
        if(dirp->is_dir == 0)
        {
            //普通文件
            strcpy(file_name, path_name);
            strcat(file_name, "/");
            strcat(file_name, dirp->name);
            file_buf = divoom_app_com_read_file(file_name, NULL);
            divoom_app_com_del_file(file_name);
            //去除文件最后的回车
            if(file_buf)
            {
                file_len = strlen(file_buf);
                while(file_len > 0 && file_buf[file_len -1])
                {
                    if(file_buf[file_len -1] == '}')
                    {
                        break;
                    }
                    file_len --;
                }
                file_buf[file_len] = 0;
                
                if(flag == 0)
                {
                    //将图像数据移动到指定目录
                    strcpy(file_name, DIVOOM_SD_APP_PIC_DIR);
                    strcat(file_name, "/");
                    strcat(file_name, dirp->name);
                    strcpy(dest_name, dest_dir_name);
                    strcat(dest_name, "/");
                    strcat(dest_name, dirp->name);
                    divoom_app_com_rename_file(file_name, dest_name);
                    //execl("mv", file_name, dest_name, NULL);
                    divoom_local_api_get_photo_info(&photo_info, file_buf);
                    divoom_print("get photo info: %d, %d, %d, %d, %s\n", photo_info.photo_x , photo_info.photo_y, photo_info.album_id, photo_info.user_id,photo_info.photo_title);
                    pic_id = divoom_pic_phone_manage_add_picture(dest_name, (double)cur_time, (uint32)cur_time, 
                        photo_info.photo_x , photo_info.photo_y, photo_info.album_id, photo_info.user_id,photo_info.photo_title, photo_info.photo_w, photo_info.photo_h, 0);
                    if(cur_album_id != photo_info.album_id)
                    {
                        if(pic_num)
                        {
                            divoom_pic_album_manage_add_list_to_name(cur_album_id, pic_list, pic_num);
                            pic_num = 0;
                        }
                    }
                    cur_album_id = photo_info.album_id;
                    pic_list[pic_num] = pic_id;
                    pic_num ++;
                    if(pic_num >= 64)
                    {
                        divoom_pic_album_manage_add_list_to_name(cur_album_id, pic_list, pic_num);
                        pic_num = 0;
                    }
                }
                else
                {
                    //如果是API参数
                    pcmd_info = divoom_net_command_json_unpacket((char *)file_buf, 0);
                    if(pcmd_info)
                    {
                        divoom_net_app_push_cmd(pcmd_info);
                    }
                }
                divoom_system_free(file_buf);
            }
        }
    }
    if(pic_num)
    {
        divoom_pic_album_manage_add_list_to_name(cur_album_id, pic_list, pic_num);
        pic_num = 0;
    }
    
    
    divoom_port_dir_close(pic_dir_handle);
    
}


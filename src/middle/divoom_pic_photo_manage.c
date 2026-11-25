#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "divoom.h"
#include "divoom_pic_phone_manage.h"
#include "divoom_pic_check.h"
#include "divoom_thumb_cache.h"
#include "divoom_pic_album_manage.h"
#include "divoom_lvgl_photo_slide_effect.h"
#ifdef  DIVOOM_USE_RK_IC
#include "rk_mpi.h"

#include <rga/im2d.h>
#include <rga/rga.h>
#include <rga/RgaApi.h>
#endif

#include "divoom.h"
#include "divoom_menu_control.h"
static int divoom_pic_phone_manage_get_decode_src_type(const char *file_name)
{
    char ext_name[16];

    if(file_name == NULL)
    {
        return RK_FORMAT_BGRA_8888;
    }
    divoom_app_com_get_file_ex_name((char *)file_name, ext_name);
    if(strcasecmp(ext_name, "jpg") == 0 || strcasecmp(ext_name, "jpeg") == 0)
    {
        return RK_FORMAT_RGB_565;
    }
    return RK_FORMAT_BGRA_8888;
}
/*
照片数据加保存如下：
表名为：pic_info
id(key),save_time(order),photo_time(order),like_flag(order),hide_flag(order),fill_flag,mirror_flag,rotate_flag,pic_video_flag,pathname


*/
/*
首先需要一个存文件信息的文件，按添加顺序进行储存，文件头存放divoom_pic_phone_manage_head, 文件内容存放N个divoom_pic_phone_manage_item，
为了避免文件损坏，所有的文件进行双备份
*/
/*
文件索引信息,这个信息保存在文件中，开机自动读取
*/

typedef struct _divoom_pic_phone_manage_ctrl 
{
    char   *base_dir;
    int    all_album_cover;    //是否设置了全部照片封面
    int    preload_flag;    //是否要初始预装相簿
    divoom_pic_phone_manage_slide_config slide_config;
    void *del_file_queue;
    void *del_file_sem;
}divoom_pic_phone_manage_ctrl, *Pdivoom_pic_phone_manage_ctrl;

Pdivoom_pic_phone_manage_ctrl pdivoom_pic_phone_manage_handle = NULL;


int divoom_pic_phone_manage_read_local_pic(int reset_all);

void *divoom_pic_phone_manage_reload_all_pic_file(void *param)
{
    char file_name[256];
    double file_time;
    int cur_time = time(0), index;
    void *random_handle;

    file_time = (double)cur_time * 1000;
    random_handle = divoom_random_create(39+6);

    for(int i = 0; i < 39+6; i++)
    {
        index = divoom_random_get(random_handle);
        file_time ++;
        if(index >=39)
        {
            sprintf(file_name, "/userdata/app_pic/default_pic/Ai%d.mp4", index - 38);
            if(divoom_app_com_check_file_exist(file_name))
            {
                divoom_pic_phone_manage_add_picture(file_name, file_time, cur_time, 0, 0, 0,0, "", 800,1280, 1);
            }
        }
        else
        {
            sprintf(file_name, "/userdata/app_pic/default_pic/%d@1x.webp", index + 1);
            if(divoom_app_com_check_file_exist(file_name))
            {
                divoom_pic_phone_manage_add_picture(file_name, file_time, cur_time, 0, 0, 0,0, "", 800,index>=30?450:1280, 1);
            }
        }
    }
    divoom_random_destroy(random_handle);
    sync();
    

    #if 0
    for(int i = 0; i < 39; i++)
    {
        sprintf(file_name, "/userdata/app_pic/default_pic/%d@1x.webp", 39-i);
        if(39-i>30)
        {
            divoom_pic_phone_manage_add_picture(file_name, cur_time, cur_time, 0, 0, 0,0, "", 800,450);
        }
        else
        {
            divoom_pic_phone_manage_add_picture(file_name, cur_time, cur_time, 0, 0, 0,0, "", 800,1280);
        }
    }
    for(int i = 0; i < 6; i++)
    {
        sprintf(file_name, "/userdata/app_pic/default_pic/Ai%d.mp4", 6-i);
        divoom_pic_phone_manage_add_picture(file_name, cur_time, cur_time, 0, 0, 0,0, "", 800,1280);
    }
    #endif
    pdivoom_pic_phone_manage_handle->preload_flag = 1;
    divoom_pic_phone_manage_save_slide_config(0);
    //divoom_pic_phone_manage_read_local_pic(param != NULL);
    return NULL;
}

/*********************************************************************
  Function:        divoom_pic_phone_manage_set_preload_album_flag
  Description:     设置预装相簿的ID
  Input:           flag      
  Return:          是否在拷贝预装
**********************************************************************/

int divoom_pic_phone_manage_set_preload_album_flag(int flag)
{
    if(flag >= 0)
    {
        pdivoom_pic_phone_manage_handle->preload_flag = flag;
    }
    return pdivoom_pic_phone_manage_handle->preload_flag;
}

/*********************************************************************
  Function:        divoom_pic_phone_manage_set_all_album_cover_pic
  Description:     设置全部照片封面
  Input:           flag      
  Return:          返回当前值
**********************************************************************/

int divoom_pic_phone_manage_set_all_album_cover_pic(int flag)
{
    if(flag >= 0)
    {
        pdivoom_pic_phone_manage_handle->all_album_cover = flag;
    }
    return pdivoom_pic_phone_manage_handle->all_album_cover;
}

/*********************************************************************
  Function:        divoom_pic_phone_manage_get_pic_offset_info
  Description:     得到图片相关信息
  Input:           pic_id      
  Return:          返回照片信息
**********************************************************************/

void divoom_pic_phone_manage_get_pic_offset_info(int pic_id, Pdivoom_pic_phone_manage_item_offset pic_info)
{
    divoom_database_manage_get_pic_info(pic_id, pic_info);
}

/*********************************************************************
  Function:        divoom_pic_phone_manage_get_first_pic_id
  Description:     得到第一张的图片ID
  Input:           
  Return:          返回照片列表ID
**********************************************************************/

int divoom_pic_phone_manage_get_first_pic_id(int album_id)
{
    return divoom_database_manage_get_first_pic_id(album_id);
}

/*********************************************************************
  Function:        divoom_pic_phone_manage_get_total_pic_num
  Description:     得到图片总数
  Input:           
  Return:          返回图片总数
**********************************************************************/

int divoom_pic_phone_manage_get_total_pic_num(int album_id)
{
    return divoom_database_manage_get_total_pic_num(album_id, 0, 0, -1, 0, 1);
}


/*********************************************************************
  Function:        divoom_pic_phone_manage_get_slide_config
  Description:     得到幻灯片播放配置
  Input:           none
  Return:          配置
**********************************************************************/

Pdivoom_pic_phone_manage_slide_config divoom_pic_phone_manage_get_slide_config(void)
{
    return &pdivoom_pic_phone_manage_handle->slide_config;
}

/*********************************************************************
  Function:        divoom_pic_phone_manage_get_slide_config
  Description:     得到幻灯片播放配置
  Input:           none
  Return:          配置
**********************************************************************/

Pdivoom_pic_phone_manage_slide_config divoom_pic_phone_manage_update_slide_config(void)
{
    return &pdivoom_pic_phone_manage_handle->slide_config;
}

#define PHOTO_PLAY_MIN_INTERVAL DIVOOM_PHOTO_SLIDE_MIN_TIME  // 最快切换间隔：3秒
#define PHOTO_PLAY_MAX_INTERVAL DIVOOM_PHOTO_SLIDE_MAX_TIME  // 最慢切换间隔：20秒


/*********************************************************************
  Function:        divoom_pic_phone_manage_convert_slide_speed_to_percent
  Description:     将显示速度转化为百分比
  Input:           速度
  Return:          百分比
**********************************************************************/

int divoom_pic_phone_manage_convert_slide_speed_to_percent(int play_speed)
{
    // 确保interval在有效范围内
    if (play_speed < PHOTO_PLAY_MIN_INTERVAL) play_speed = PHOTO_PLAY_MIN_INTERVAL;
    if (play_speed > PHOTO_PLAY_MAX_INTERVAL) play_speed = PHOTO_PLAY_MAX_INTERVAL;
    
    // 首先将时间映射到0-1范围
    float normalized = (float)(play_speed - PHOTO_PLAY_MIN_INTERVAL) / (float)(PHOTO_PLAY_MAX_INTERVAL - PHOTO_PLAY_MIN_INTERVAL);
    
    // 对应上一个函数中的平方映射，这里使用平方根
    float curve = sqrtf(normalized);
    
    // 映射到0-100范围并反转（使得时间越短，速度值越大）
    int speed = (int)(100.0f * (1.0f - curve) + 0.5f);
    
    // 确保结果在0-100范围内
    if (speed < 0) speed = 0;
    if (speed > 100) speed = 100;
    return speed;
}

/*********************************************************************
  Function:        divoom_pic_phone_manage_convert_percent_to_slide_speed
  Description:     将百分比转化为显示速度
  Input:           百分比
  Return:          速度
**********************************************************************/

int divoom_pic_phone_manage_convert_percent_to_slide_speed(int speed)
{   
    
    // 确保speed在0-100范围内
    if (speed < 0) speed = 0;
    if (speed > 100) speed = 100;
    
    // 将speed转换为0-1的范围，并反转（使得速度值越大，时间越短）
    float normalized = (100 - speed) / 100.0f;
    
    // 使用指数映射使得调节更加平滑
    // power = 2.0会产生平方曲线，使得在慢速区间调节更精细
    float curve = powf(normalized, 2.0f);
    
    // 映射到实际的时间区间
    return PHOTO_PLAY_MIN_INTERVAL + (PHOTO_PLAY_MAX_INTERVAL - PHOTO_PLAY_MIN_INTERVAL) * curve;
}


/*********************************************************************
  Function:        divoom_pic_phone_manage_get_default_slide_config
  Description:     得到默认的幻灯片播放配置
  Input:           none
  Return:          none
**********************************************************************/

void divoom_pic_phone_manage_get_default_slide_config(void)
{
    pdivoom_pic_phone_manage_handle->slide_config.effect_type = 0;     //幻灯片播放效果
    pdivoom_pic_phone_manage_handle->slide_config.bg_type = 0;         //背景模式
    pdivoom_pic_phone_manage_handle->slide_config.fill_flag = 0;       //是否填充像框标记
    pdivoom_pic_phone_manage_handle->slide_config.skip_fill_flag = 0;  //是否忽略单张照片填充标记， 只有fill_flag打开才有效
    pdivoom_pic_phone_manage_handle->slide_config.disp_order = 0;      //显示顺序1:
    pdivoom_pic_phone_manage_handle->slide_config.disp_reverse = 0;    //显示顺序反转标记0;从新到旧，1：从旧到新
    pdivoom_pic_phone_manage_handle->slide_config.clock_flag = 1;      //显示时钟标记
    pdivoom_pic_phone_manage_handle->slide_config.weather_flag = 1;    //显示天气标记
    pdivoom_pic_phone_manage_handle->slide_config.title_flag = 0;      //显示标题标记
    pdivoom_pic_phone_manage_handle->slide_config.video_auto_play = 1; //视频自动播放标记
    pdivoom_pic_phone_manage_handle->slide_config.video_play_flag = 0;    //视频播放设置；0：只播放一次；1：循环播放
    pdivoom_pic_phone_manage_handle->slide_config.video_auto_mute = 0; //视频播放十分钟后自动静音标记
    pdivoom_pic_phone_manage_handle->slide_config.video_volume = 50;    //视频播放音量
    /* play_time 单位是毫秒（3s–20s）；50% 对应设置页速度滑条中位 */
    pdivoom_pic_phone_manage_handle->slide_config.play_time =
        divoom_pic_phone_manage_convert_percent_to_slide_speed(50);
    pdivoom_pic_phone_manage_handle->slide_config.album_id = 0;
    pdivoom_pic_phone_manage_handle->preload_flag = 1;
}

/*********************************************************************
  Function:        divoom_pic_phone_manage_load_offset_info
  Description:     加载图片的偏移相关信息
  Input:           none
  Return:          none
**********************************************************************/

void divoom_pic_phone_manage_load_offset_info(void)
{
    return ;
    
}

/*********************************************************************
  Function:        divoom_pic_phone_manage_update_album_update_time
  Description:     更新相簿定时最新时间
  Input:           update_time
  Return:          最新时间
**********************************************************************/

int divoom_pic_phone_manage_update_album_update_time(int update_time)
{   
    if(update_time > 0)
    {
        if(update_time > pdivoom_pic_phone_manage_handle->slide_config.album_update_time)
        {
            pdivoom_pic_phone_manage_handle->slide_config.album_update_time = update_time;
            divoom_pic_phone_manage_save_slide_config(0);
        }
    }
    return pdivoom_pic_phone_manage_handle->slide_config.album_update_time;
}

/*********************************************************************
  Function:        divoom_pic_phone_manage_remove_pic_list
  Description:     删除一批图片
  Input:           pic_list       图片ID
                   pic_num   
                   pitem_list 返回删除照片的相关信息
  Return:          返回实际删除的个数
**********************************************************************/

int divoom_pic_phone_manage_remove_pic_list(int *pic_list, int pic_num)
{    
    int i, j;
    char path_name[256];

    for(i = 0; i < pic_num; i++)
    {
        divoom_database_manage_get_pic_path_name(pic_list[i], path_name);
        divoom_pic_phone_manage_cache_delete_pic(path_name);             
        divoom_database_manage_delete_pic(pic_list[i]);
    }

    divoom_pic_album_manage_clear_empty_album_covers();
    return pic_num;

}


/*********************************************************************
  Function:        divoom_pic_phone_manage_remove_all_pic
  Description:     删除所有图片
  Input:           none   
                   
  Return:          none
**********************************************************************/

void divoom_pic_phone_manage_remove_all_pic(void)
{    
    int j, pic_num = 100, start_time;
    uint32 *pic_list;
    char path_name[256];


    while(1)
    {
        pic_num = 100;
        pic_list =  divoom_database_manage_get_all_pic_id(0, &pic_num, 0, 0, -1, 0, 1);
        if(pic_num == 0)
        {
            break;
        }
        for(j = 0; j < pic_num; j++)
        {
            divoom_database_manage_get_pic_path_name(pic_list[j], path_name);
            divoom_app_com_del_file(path_name);  
            divoom_pic_phone_manage_del_picture_thumb(path_name);              
            divoom_database_manage_delete_pic(pic_list[j]);
        }
        if(pic_list)
        {
            divoom_system_free(pic_list);
        }
    }
    divoom_pic_album_manage_clear_empty_album_covers();
}


/*********************************************************************
  Function:        divoom_pic_phone_manage_add_piclist_to_album
  Description:     更新一批相片到相簿
  Input:           pic_list       图片ID
                   pic_num
                   album_id     相册ID
  Return:          none
**********************************************************************/

void divoom_pic_phone_manage_add_piclist_to_album(uint32 *pic_list, int pic_num,  uint32 album_id)
{    
    divoom_pic_album_manage_add_list_to_name(album_id, pic_list, pic_num);
}

void divoom_pic_phone_manage_save_slide_config(int send_flag)
{
    divoom_photo_album_album_config_info pconfig_info;
    char *json_string = NULL;
    
    divoom_cJSON *root;
    
    
    root = divoom_cJSON_CreateObject();
    divoom_cJSON_AddNumberToObject(root, "all_album_cover", pdivoom_pic_phone_manage_handle->all_album_cover);
    divoom_cJSON_AddNumberToObject(root, "preload_flag", pdivoom_pic_phone_manage_handle->preload_flag);
    divoom_cJSON_AddNumberToObject(root, "effect_type", pdivoom_pic_phone_manage_handle->slide_config.effect_type);
    divoom_cJSON_AddNumberToObject(root, "sub_effect_type", pdivoom_pic_phone_manage_handle->slide_config.sub_effect_type);
    divoom_cJSON_AddNumberToObject(root, "bg_type", pdivoom_pic_phone_manage_handle->slide_config.bg_type);
    divoom_cJSON_AddNumberToObject(root, "fill_flag", pdivoom_pic_phone_manage_handle->slide_config.fill_flag);
    divoom_cJSON_AddNumberToObject(root, "skip_fill_flag", pdivoom_pic_phone_manage_handle->slide_config.skip_fill_flag);
    divoom_cJSON_AddNumberToObject(root, "disp_order", pdivoom_pic_phone_manage_handle->slide_config.disp_order);
    divoom_cJSON_AddNumberToObject(root, "disp_reverse", pdivoom_pic_phone_manage_handle->slide_config.disp_reverse);
    divoom_cJSON_AddNumberToObject(root, "clock_flag", pdivoom_pic_phone_manage_handle->slide_config.clock_flag);
    divoom_cJSON_AddNumberToObject(root, "weather_flag", pdivoom_pic_phone_manage_handle->slide_config.weather_flag);
    divoom_cJSON_AddNumberToObject(root, "title_flag", pdivoom_pic_phone_manage_handle->slide_config.title_flag);
    divoom_cJSON_AddNumberToObject(root, "video_auto_play", pdivoom_pic_phone_manage_handle->slide_config.video_auto_play);
    divoom_cJSON_AddNumberToObject(root, "video_play_flag", pdivoom_pic_phone_manage_handle->slide_config.video_play_flag);
    divoom_cJSON_AddNumberToObject(root, "video_auto_mute", pdivoom_pic_phone_manage_handle->slide_config.video_auto_mute);
    divoom_cJSON_AddNumberToObject(root, "video_volume", pdivoom_pic_phone_manage_handle->slide_config.video_volume);
    divoom_cJSON_AddNumberToObject(root, "play_time", pdivoom_pic_phone_manage_handle->slide_config.play_time);
    divoom_cJSON_AddNumberToObject(root, "album_id", pdivoom_pic_phone_manage_handle->slide_config.album_id);
    divoom_cJSON_AddNumberToObject(root, "album_update_time", pdivoom_pic_phone_manage_handle->slide_config.album_update_time);
    
    json_string = divoom_cJSON_Print(root);
    divoom_cJSON_Delete(root);
    divoom_app_com_write_file(DIVOOM_PHOTO_SETTING_CFG_PATH,json_string, strlen(json_string));
    divoom_system_free(json_string);
    if(send_flag)
    {
        pconfig_info.SlideshowSpeed = divoom_pic_phone_manage_convert_slide_speed_to_percent(pdivoom_pic_phone_manage_handle->slide_config.play_time);
        pconfig_info.SlideshowTheme = pdivoom_pic_phone_manage_handle->slide_config.effect_type;
        pconfig_info.FillFrame = pdivoom_pic_phone_manage_handle->slide_config.fill_flag;
        pconfig_info.FrameBackground = pdivoom_pic_phone_manage_handle->slide_config.bg_type;
        pconfig_info.DisplayOrder = pdivoom_pic_phone_manage_handle->slide_config.disp_order;
        pconfig_info.ReversePhotoOrder = pdivoom_pic_phone_manage_handle->slide_config.disp_reverse;
        pconfig_info.ShowTitle  = pdivoom_pic_phone_manage_handle->slide_config.title_flag;
        pconfig_info.ShowClock = pdivoom_pic_phone_manage_handle->slide_config.clock_flag;
        pconfig_info.ShowWeather = pdivoom_pic_phone_manage_handle->slide_config.weather_flag;
        pconfig_info.VideoAutoPlay = pdivoom_pic_phone_manage_handle->slide_config.video_auto_play;
        pconfig_info.VideoPlayBack = pdivoom_pic_phone_manage_handle->slide_config.video_play_flag;
        pconfig_info.VideoAutoMute = pdivoom_pic_phone_manage_handle->slide_config.video_auto_mute;
        pconfig_info.VideoVolume = pdivoom_pic_phone_manage_handle->slide_config.video_volume ;
        divoom_net_app_send_packet(DIVOOM_NET_COMM_PHOTO_SET_ALBUM_CONFIG, DIVOOM_SERVER_TOPIC,&pconfig_info);
    }
}


void divoom_pic_phone_manage_load_slide_config(void)
{

    char *json_info;
	divoom_cJSON *root;

    json_info = divoom_app_com_read_file(DIVOOM_PHOTO_SETTING_CFG_PATH, NULL);
    if(json_info == NULL)
    {
        divoom_pic_phone_manage_get_default_slide_config();
        return;
    }
	root = divoom_cJSON_Parse(json_info);
    divoom_system_free(json_info);
    if(root == NULL)
    {
        divoom_pic_phone_manage_get_default_slide_config();
        return;
    }
    
    pdivoom_pic_phone_manage_handle->all_album_cover = divoom_cJSON_GetObjectInt(root, "all_album_cover");
    pdivoom_pic_phone_manage_handle->preload_flag = divoom_cJSON_GetObjectInt(root, "preload_flag");
    pdivoom_pic_phone_manage_handle->slide_config.effect_type = divoom_cJSON_GetObjectInt(root, "effect_type");
    pdivoom_pic_phone_manage_handle->slide_config.sub_effect_type = divoom_cJSON_GetObjectInt(root, "sub_effect_type");
    pdivoom_pic_phone_manage_handle->slide_config.bg_type = divoom_cJSON_GetObjectInt(root, "bg_type");
    pdivoom_pic_phone_manage_handle->slide_config.fill_flag = divoom_cJSON_GetObjectInt(root, "fill_flag");
    pdivoom_pic_phone_manage_handle->slide_config.skip_fill_flag = divoom_cJSON_GetObjectInt(root, "skip_fill_flag");
    pdivoom_pic_phone_manage_handle->slide_config.disp_order = divoom_cJSON_GetObjectInt(root, "disp_order");
    pdivoom_pic_phone_manage_handle->slide_config.disp_reverse = divoom_cJSON_GetObjectInt(root, "disp_reverse");
    pdivoom_pic_phone_manage_handle->slide_config.clock_flag = divoom_cJSON_GetObjectInt(root, "clock_flag");
    pdivoom_pic_phone_manage_handle->slide_config.weather_flag = divoom_cJSON_GetObjectInt(root, "weather_flag");
    pdivoom_pic_phone_manage_handle->slide_config.title_flag = divoom_cJSON_GetObjectInt(root, "title_flag");
    pdivoom_pic_phone_manage_handle->slide_config.video_auto_play = divoom_cJSON_GetObjectInt(root, "video_auto_play");
    pdivoom_pic_phone_manage_handle->slide_config.video_play_flag = divoom_cJSON_GetObjectInt(root, "video_play_flag");
    pdivoom_pic_phone_manage_handle->slide_config.video_auto_mute = divoom_cJSON_GetObjectInt(root, "video_auto_mute");
    pdivoom_pic_phone_manage_handle->slide_config.video_volume = divoom_cJSON_GetObjectInt(root, "video_volume");
    pdivoom_pic_phone_manage_handle->slide_config.play_time = divoom_cJSON_GetObjectInt(root, "play_time");
    pdivoom_pic_phone_manage_handle->slide_config.album_id = divoom_cJSON_GetObjectInt(root, "album_id");
    pdivoom_pic_phone_manage_handle->slide_config.album_update_time = divoom_cJSON_GetObjectInt(root, "album_update_time");
    if(pdivoom_pic_phone_manage_handle->slide_config.play_time < PHOTO_PLAY_MIN_INTERVAL)
    {
        pdivoom_pic_phone_manage_handle->slide_config.play_time = PHOTO_PLAY_MIN_INTERVAL;
    }
    if(pdivoom_pic_phone_manage_handle->slide_config.play_time > PHOTO_PLAY_MAX_INTERVAL)
    {
        pdivoom_pic_phone_manage_handle->slide_config.play_time = PHOTO_PLAY_MAX_INTERVAL;
    }

    divoom_cJSON_Delete(root);
}

/*********************************************************************
  Function:        divoom_pic_phone_manage_cache_delete_pic
  Description:     延时删除文件，放在后台处理
  Input:           file_name
  Return:          none
**********************************************************************/

void divoom_pic_phone_manage_cache_delete_pic(char *file_name)
{
    char *pic_name = divoom_app_com_copy_string(file_name, -1);

    if(pic_name)
    {
        divoom_queue_push(pdivoom_pic_phone_manage_handle->del_file_queue, pic_name);
        divoom_system_put_sem(pdivoom_pic_phone_manage_handle->del_file_sem);
    }
}

void* divoom_pic_phone_manage_delete_pic_task_main(void *param)
{
    char *file_name;
    
    while(divoom_queue_items(pdivoom_pic_phone_manage_handle->del_file_queue))
    {
        file_name = divoom_queue_pop(pdivoom_pic_phone_manage_handle->del_file_queue);
        if(file_name)
        {
            divoom_app_com_del_file(file_name);  
            divoom_pic_phone_manage_del_picture_thumb(file_name);              
            divoom_system_free(file_name);
            divoom_system_delay_ms(50);
        }
    }
}

/*********************************************************************
  Function:        divoom_pic_phone_manage_init
  Description:     初始化图片管理模块，应用程序启动时加载，将所有文件偏移信息加载到内存中
  Input:           handle
  Return:          none
**********************************************************************/

void divoom_pic_phone_manage_init(char *base_dir, int first_flag)
{
    int album_num;
    pdivoom_pic_phone_manage_handle = divoom_system_malloc(sizeof(divoom_pic_phone_manage_ctrl));
    if(pdivoom_pic_phone_manage_handle == NULL)
    {
        return;
    }
    memset(pdivoom_pic_phone_manage_handle, 0, sizeof(divoom_pic_phone_manage_ctrl));
    pdivoom_pic_phone_manage_handle->base_dir = divoom_app_com_copy_string(base_dir, 0);
    pdivoom_pic_phone_manage_handle->del_file_queue = divoom_queue_init(100000, __func__, __LINE__);
    pdivoom_pic_phone_manage_handle->del_file_sem = divoom_system_create_sem(1, 0);
    album_num = divoom_pic_album_manage_init();
    divoom_thumb_cache_init(60);
    divoom_pic_phone_manage_load_slide_config();
    divoom_pic_phone_manage_load_offset_info();
    if(pdivoom_pic_phone_manage_handle->slide_config.play_time < PHOTO_PLAY_MIN_INTERVAL)
    {
        pdivoom_pic_phone_manage_handle->slide_config.play_time = PHOTO_PLAY_MIN_INTERVAL;
    }
    if(pdivoom_pic_phone_manage_handle->slide_config.play_time > PHOTO_PLAY_MAX_INTERVAL)
    {
        pdivoom_pic_phone_manage_handle->slide_config.play_time = PHOTO_PLAY_MAX_INTERVAL;
    }
    if(pdivoom_pic_phone_manage_handle->slide_config.album_id == 0)
    {
        pdivoom_pic_phone_manage_handle->slide_config.album_id = divoom_pic_album_manage_get_album_id_from_type(DIVOOM_PIC_ALBUM_TYPE_ALL);
    }
    //divoom_system_create_task(divoom_pic_phone_manage_delete_pic_task_main, "pic_delete", pdivoom_pic_phone_manage_handle, DIVOOM_TASK_STACK_PIC_DELETE);
}



/*********************************************************************
  Function:        divoom_pic_phone_manage_convert_video_volume
  Description:     转换视频音量，目前最大音量要做限制
  Input:           voluem
  Return:          转换后的音量
**********************************************************************/

int divoom_pic_phone_manage_convert_video_volume(int volume)
{
    return volume * DIVOOM_PLAY_MOIVE_MAX_VOLUME / 100;
}

/*********************************************************************
  Function:        divoom_pic_phone_manage_add_all_preload
  Description:     初始化预装相簿数据
  Input:           album_id
  Return:          none
**********************************************************************/

void divoom_pic_phone_manage_add_all_preload(int album_id)
{
    int pic_num;
    uint32 * pic_list;

    if(pdivoom_pic_phone_manage_handle->preload_flag)
    {
        pic_num = 100;
        pic_list =  divoom_database_manage_get_all_pic_id(0, &pic_num, 0, 0, -1, 0, 1);
        if(pic_list == NULL)
        {
            return;
        }
        if (!divoom_pic_album_manage_add_preload_list(album_id, pic_list, pic_num))
        {
            divoom_system_free(pic_list);
            return;
        }
        divoom_pic_album_manage_set_album_cover(album_id, pic_list[pic_num - 1], 0, 0);
        divoom_system_free(pic_list);
        pdivoom_pic_phone_manage_handle->preload_flag = 2;
        divoom_pic_phone_manage_save_slide_config(0);
    }
}

/*********************************************************************
  Function:        divoom_pic_phone_manage_get_picture_disp_buf
  Description:     从文件解码出来的图像得到显示图像
  Input:           file_name
                   image_buf
                   dest_w
                   dest_h
                   rotate_flag  旋转角度
                   fill_flag    填充类型
                   bg_buf   //背景缓冲区，不需要为空
                   bg_type  //背景类型
  Return:          1：成功；0：失败
**********************************************************************/

int divoom_pic_phone_manage_get_picture_disp_buf(char *file_name, uint8 *image_buf, int *dest_w, int *dest_h, int rotate_flag, int fill_flag, int mirror_flag, uint8 *bg_buf, int bg_type)
{
    uint8 *jpeg_buf;
    int image_w, image_h;
    int src_type;
    
    jpeg_buf = divoom_rk_mpp_get_jpeg_info(file_name, &image_w, &image_h);
    if(jpeg_buf)
    {
        src_type = divoom_pic_phone_manage_get_decode_src_type(file_name);
        if((rotate_flag || mirror_flag) && src_type == RK_FORMAT_RGB_565)
        {
            uint8 *tmp_bgra = divoom_system_malloc(image_w * image_h * 4);
            if(tmp_bgra == NULL)
            {
                divoom_system_free(jpeg_buf);
                return 0;
            }
            else
            {
                divoom_rk_mpp_stretch_image_direct(jpeg_buf, image_w, image_h,
                    RK_FORMAT_RGB_565, tmp_bgra, image_w, image_h,
                    RK_FORMAT_BGRA_8888);
                divoom_system_free(jpeg_buf);
                jpeg_buf = tmp_bgra;
                src_type = RK_FORMAT_BGRA_8888;
            }
        }
        if(rotate_flag || mirror_flag)
        {
            uint8 *tmp_buf = divoom_system_malloc(image_w * image_h *4);

            if(tmp_buf)
            {
                divoom_pic_phone_manage_convert_rgb_buf(jpeg_buf, tmp_buf, image_w, image_h, rotate_flag, mirror_flag);
                if(rotate_flag & 1)
                {
                    //旋转90，270度时，要交换长与宽
                    int tmp = image_w;
                    image_w = image_h;
                    image_h = tmp;
                }
                divoom_system_free(jpeg_buf);
                jpeg_buf = tmp_buf;
            }
        }
        divoom_rk_mpp_get_disp_image_info(jpeg_buf, image_w, image_h, src_type, image_buf, dest_w, dest_h, RK_FORMAT_BGRA_8888, fill_flag);
        divoom_system_free(jpeg_buf);
        return 1;
    }
    return 0;
}

/*********************************************************************
  Function:        divoom_pic_phone_manage_decode_picture_rgb_buf
  Description:     从文件解码出来的图像得到显示图像
  Input:           pic_id
                   image_buf
                   dest_w
                   dest_h
  Return:          1：成功；0：失败
**********************************************************************/

int divoom_pic_phone_manage_decode_picture_rgb_buf(int pic_id, uint8 *image_buf, int *dest_w, int *dest_h)
{
    char pic_name[256];
    divoom_pic_phone_manage_item_offset pic_info;

    divoom_pic_phone_manage_get_pic_offset_info(pic_id, &pic_info);

    divoom_pic_phone_manage_get_picture_name(pic_name, pic_id);

    return divoom_pic_phone_manage_get_picture_disp_buf(pic_name, image_buf, dest_w, dest_h, pic_info.rotate_flag, pic_info.fill_flag, pic_info.mirror_flag, NULL, 0);
}


/*********************************************************************
  Function:        divoom_pic_phone_manage_stretch_picture_disp_buf
  Description:     从文件解码出来的图像缩放到指定大小
  Input:           file_name
                   image_buf
                   dest_w
                   dest_h
                   rotate_flag  旋转角度
                   mirror_flag  镜像模式
  Return:          1：成功；0：失败
**********************************************************************/

int divoom_pic_phone_manage_stretch_picture_disp_buf(char *file_name, uint8 *image_buf, int dest_w, int dest_h, int rotate_flag, int mirror_flag)
{
    uint8 *jpeg_buf, *crop_buf;
    int image_w, image_h, start, len;
    float factor_w, factor_h;
    int src_type;

    jpeg_buf = divoom_rk_mpp_get_jpeg_info(file_name, &image_w, &image_h);
    if(jpeg_buf)
    {
        src_type = divoom_pic_phone_manage_get_decode_src_type(file_name);
        if((rotate_flag || mirror_flag) && src_type == RK_FORMAT_RGB_565)
        {
            uint8 *tmp_bgra = divoom_system_malloc(image_w * image_h * 4);
            if(tmp_bgra == NULL)
            {
                divoom_system_free(jpeg_buf);
                return 0;
            }
            else
            {
                divoom_rk_mpp_stretch_image_direct(jpeg_buf, image_w, image_h,
                    RK_FORMAT_RGB_565, tmp_bgra, image_w, image_h,
                    RK_FORMAT_BGRA_8888);
                divoom_system_free(jpeg_buf);
                jpeg_buf = tmp_bgra;
                src_type = RK_FORMAT_BGRA_8888;
            }
        }
        if(rotate_flag || mirror_flag)
        {
            uint8 *tmp_buf, *out_buf = NULL;
            tmp_buf = divoom_system_malloc(image_w * image_h * 4);
            if(tmp_buf)
            {
                if(rotate_flag)
                {
                    divoom_rk_mpp_rotate_image(jpeg_buf, &image_w, &image_h, RK_FORMAT_BGRA_8888, tmp_buf, rotate_flag);
                    out_buf = tmp_buf;
                }
                if(mirror_flag)
                {
                    if(rotate_flag)
                    {
                        divoom_rk_mpp_mirror_image(tmp_buf, image_w, image_h, RK_FORMAT_BGRA_8888, jpeg_buf);
                        out_buf = jpeg_buf;
                    }
                    else
                    {
                        divoom_rk_mpp_mirror_image(jpeg_buf, image_w, image_h, RK_FORMAT_BGRA_8888, tmp_buf);
                        out_buf = tmp_buf;
                    }
                }
                if(out_buf == tmp_buf)
                {
                    divoom_system_free(jpeg_buf);
                }
                else
                {
                    divoom_system_free(tmp_buf);
                }
                jpeg_buf = out_buf;
            }
            else
            {
                divoom_print("err malloc!\n");
            }
        }
        /* 缩略图随后由 WebPEncodeBGRA 编码，输出必须保持 BGRA，避免红蓝通道互换。 */
        divoom_rk_mpp_get_disp_image_info(jpeg_buf, image_w, image_h, src_type,
            image_buf, &dest_w, &dest_h, RK_FORMAT_BGRA_8888, 1);
        //divoom_rk_mpp_stretch_raw_picture_data(jpeg_buf, image_w, image_h, image_buf, dest_w, dest_h);
        divoom_system_free(jpeg_buf);
       
        return 1;
    }
    return 0;
}


/*********************************************************************
  Function:        divoom_pic_phone_manage_stretch_bgra_buf
  Description:     将图像缩放到指定大小
  Input:           jpeg_buf
                   image_w
                   image_h
                   image_buf
                   dest_w
                   dest_h
                   rotate_flag  旋转角度
                   mirror_flag  镜像模式
  Return:          none
**********************************************************************/

void divoom_pic_phone_manage_stretch_bgra_buf(uint8 *jpeg_buf, int image_w, int image_h,uint8 *image_buf, int dest_w, int dest_h, int rotate_flag, int mirror_flag)
{
    uint8 *crop_buf;
    int  start, len;
    float factor_w, factor_h;
    
    if(rotate_flag || mirror_flag)
    {
        uint8 *tmp_buf, *out_buf = NULL;
        tmp_buf = divoom_system_malloc(image_w * image_h * 4);
        if(tmp_buf)
        {
            if(rotate_flag)
            {
                divoom_rk_mpp_rotate_image(jpeg_buf, &image_w, &image_h, RK_FORMAT_BGRA_8888, tmp_buf, rotate_flag);
                out_buf = tmp_buf;
            }
            if(mirror_flag)
            {
                if(rotate_flag)
                {
                    divoom_rk_mpp_mirror_image(tmp_buf, image_w, image_h, RK_FORMAT_BGRA_8888, jpeg_buf);
                    out_buf = jpeg_buf;
                }
                else
                {
                    divoom_rk_mpp_mirror_image(jpeg_buf, image_w, image_h, RK_FORMAT_BGRA_8888, tmp_buf);
                    out_buf = tmp_buf;
                }
            }
            if(out_buf == tmp_buf)
            {
                divoom_system_free(jpeg_buf);
            }
            else
            {
                divoom_system_free(tmp_buf);
            }
            jpeg_buf = out_buf;
        }
        else
        {
            divoom_print("err malloc!\n");
        }
    }
    divoom_rk_mpp_get_disp_image_info(jpeg_buf, image_w, image_h, RK_FORMAT_RGBA_8888, image_buf, &dest_w, &dest_h, RK_FORMAT_RGBA_8888, 1);
    //divoom_rk_mpp_stretch_raw_picture_data(jpeg_buf, image_w, image_h, image_buf, dest_w, dest_h);
}

/*********************************************************************
  Function:        divoom_pic_phone_manage_decode_picture
  Description:     将文件解码到目标大小的缓冲区
  Input:           pic_path
                   dest_w
                   dest_h
                   rgb_buf
  Return:          1：成功；0：失败
**********************************************************************/

uint8 divoom_pic_phone_manage_decode_picture(char *pic_path, int *dest_w, int *dest_h, uint8 *rgb_buf)
{
    int image_w, image_h;
    int src_type;
    uint8 *image_buf;

    image_buf = divoom_rk_mpp_get_jpeg_info(pic_path, &image_w, &image_h);
    if(image_buf == NULL)
    {
        return 0;
    }
    src_type = divoom_pic_phone_manage_get_decode_src_type(pic_path);
    divoom_rk_mpp_stretch_image(image_buf, image_w, image_h, src_type, rgb_buf, *dest_w, *dest_h);
    divoom_system_free(image_buf);
    return 1;
}

void divoom_encode_rgb_to_jpeg(char *file_name, unsigned char *rgb_buffer, int width, int height); 

/*********************************************************************
  Function:        divoom_pic_phone_manage_encode_picture
  Description:     将图像编码JPEG到目标文件
  Input:           pic_path
                   dest_w
                   dest_h
                   rgb_buf
  Return:          none
**********************************************************************/

void divoom_pic_phone_manage_encode_picture(char *pic_path, int dest_w, int dest_h, uint8 *rgb_buf)
{
    divoom_libwebp_encode_image(pic_path, rgb_buf, dest_w, dest_h);
    //divoom_encode_rgb_to_jpeg(pic_path, rgb_buf, dest_w, dest_h);
}

/*********************************************************************
  Function:        divoom_pic_phone_manage_del_picture_thumb
  Description:     删除缩略图文件
  Input:           pic_path
  Return:          none
**********************************************************************/

void divoom_pic_phone_manage_del_picture_thumb(char *pic_path)
{
    char thumb_path[256];
    
    divoom_thumb_cache_get_thumb_name(pic_path, thumb_path);
    divoom_app_com_del_file(thumb_path);
}

/*********************************************************************
  Function:        divoom_pic_phone_manage_create_picture_thumb
  Description:     创建缩略图文件，并输出缩略图缓冲区
  Input:           pic_path
                   dest_w
                   dest_h
                   rgb_buf
  Return:          none
**********************************************************************/


void divoom_pic_phone_manage_create_picture_thumb(char *pic_name_info, int *out_w, int *out_h,uint8 *rgb_buf)
{
    int len, src_len, image_w, image_h, dest_w, dest_h;
    char *thumb_path, *pic_path,*base_ptr,*find_ptr;    
    divoom_port_stat_t file_st;

    thumb_path = divoom_system_malloc(256);
    pic_path = divoom_system_malloc(256);
    
    strcpy(pic_path, pic_name_info);
    base_ptr = strstr(pic_path, ".mp4");
    if(base_ptr)
    {
        strcpy(base_ptr, ".webp");
    }
    find_ptr = &pic_path[strlen(DIVOOM_SD_APP_PIC_DIR)];  
    //首先要确认thumb_path路径是否全部存在,不存在将创建目录
    strcpy(thumb_path, DIVOOM_THUMB_BASE_PATH);
    /* 版本化缓存根首次使用时先单独创建，后续再逐级创建相片子目录。 */
    divoom_app_com_mk_dir(thumb_path);
    strcat(thumb_path, "/");
    find_ptr ++;
    divoom_print("find: %s\n", find_ptr);
    while(find_ptr)
    {
        base_ptr = find_ptr;
        find_ptr = strchr(find_ptr, '/');
        if(find_ptr == NULL)
        {
            strcat(thumb_path, base_ptr);
            break;
        }
        len = (int)(find_ptr - base_ptr);
        if(len)
        {
            src_len = strlen(thumb_path);
            memcpy(&thumb_path[src_len], base_ptr, len);
            thumb_path[src_len + len] = 0;
            divoom_app_com_mk_dir(thumb_path);
            thumb_path[src_len + len] = '/';
            thumb_path[src_len + len + 1] = 0;
        }
        find_ptr ++;
    } 
    len = strlen(thumb_path) -1;
    while(len > 0 && thumb_path[len] != '.')
    {
        len --;
    }
    thumb_path[len + 1] = 0;
    strcat(thumb_path, "webp"); 
    
    if (divoom_port_stat(thumb_path, &file_st) == 0) {
        if(file_st.size > 0)
        {
            divoom_print("pass pic path: %s, thumb path: %s\n", pic_path, thumb_path);  
            divoom_system_free(thumb_path);
            divoom_system_free(pic_path);
            return;
        }
        else if(file_st.size == 0)
        {
           divoom_print("we will delete size=0 pic path: %s, thumb path: %s\n", pic_path, thumb_path);  
           divoom_app_com_del_file(thumb_path);
        }
    }
    if(strstr(pic_path, ".mp4"))
    {
        *out_w = DIVOOM_THUMB_PIC_WIDTH;
        *out_h = DIVOOM_THUMB_PIC_HIGH;
        divoom_decode_movie_review_pic_create(pic_path, thumb_path, DIVOOM_THUMB_PIC_WIDTH, DIVOOM_THUMB_PIC_HIGH, 0, 0);
    }
    else
    {
        int malloc_flag = 0;
        
        *out_w = DIVOOM_THUMB_PIC_WIDTH;
        *out_h = DIVOOM_THUMB_PIC_HIGH;
        if(rgb_buf == NULL)
        {
            rgb_buf = divoom_system_malloc(DIVOOM_THUMB_PIC_WIDTH*DIVOOM_THUMB_PIC_HIGH*4);
            malloc_flag = 1;
        }
        if(divoom_pic_phone_manage_stretch_picture_disp_buf(pic_path, rgb_buf, DIVOOM_THUMB_PIC_WIDTH, DIVOOM_THUMB_PIC_HIGH, 0, 0))
        {
            divoom_pic_phone_manage_encode_picture(thumb_path, DIVOOM_THUMB_PIC_WIDTH, DIVOOM_THUMB_PIC_HIGH, rgb_buf);
        }
        if(malloc_flag)
        {
            divoom_system_free(rgb_buf);
        }
    }
    divoom_main_update_last_download_file();
    divoom_system_free(thumb_path);
    divoom_system_free(pic_path);
}


/*********************************************************************
  Function:        divoom_pic_phone_manage_convert_rgb_buf
  Description:     转换输出BUFFER
  Input:           src_rgb           
                   dest_rgb
                   image_w  
                   image_h
                   rotate_flag
                   mirror_flag                   
  Return:          none
**********************************************************************/

void divoom_pic_phone_manage_convert_rgb_buf(uint8 *src_rgb, uint8 *dest_rgb, int image_w, int image_h, int rotate_flag, int mirror_flag)
{  
    uint8 *tmp_buf;
    if(rotate_flag == 0 && mirror_flag == 0)
    {
        memcpy(dest_rgb, src_rgb, image_w * image_h *4);
        return;
    }

    if(mirror_flag == 0)
    {
        //只旋转
        divoom_rk_mpp_rotate_image(src_rgb, &image_w, &image_h, RK_FORMAT_BGRA_8888, dest_rgb, rotate_flag);
        return;
    }
    else if(rotate_flag == 0)
    {
        //只镜像
        divoom_rk_mpp_mirror_image(src_rgb, image_w, image_h, RK_FORMAT_BGRA_8888, dest_rgb);
        return;
    }
    //先镜像再旋转
    tmp_buf  = divoom_system_malloc(image_w * image_h * 4);
    divoom_rk_mpp_mirror_image(src_rgb, image_w, image_h, RK_FORMAT_BGRA_8888, tmp_buf);
    divoom_rk_mpp_rotate_image(tmp_buf, &image_w, &image_h, RK_FORMAT_BGRA_8888, dest_rgb, rotate_flag);
    divoom_system_free(tmp_buf);
}

/*********************************************************************
  Function:        divoom_pic_phone_manage_set_picture_like_attr
  Description:     设置照片的收藏属性
  Input:           pic_id           图片ID
                   like_flag       
  Return:          none
**********************************************************************/

void divoom_pic_phone_manage_set_picture_like_attr(int pic_id, int like_flag)
{    

}

/*********************************************************************
  Function:        divoom_pic_phone_manage_set_picture_hide_attr
  Description:     设置照片的隐藏属性
  Input:           pic_id           图片ID
                   hide_flag        0:显示，1：隐藏
  Return:          none
**********************************************************************/

void divoom_pic_phone_manage_set_picture_hide_attr(int pic_id, int hide_flag)
{    

}

/*********************************************************************
  Function:        divoom_pic_phone_manage_set_picture_mirror_attr
  Description:     设置照片的镜像属性,删除缩略图
  Input:           pic_id           图片ID
                   mirror_flag        
  Return:          none
**********************************************************************/

void divoom_pic_phone_manage_set_picture_mirror_attr(int pic_id, int mirror_flag)
{    
}

/*********************************************************************
  Function:        divoom_pic_phone_manage_set_picture_hide_attr
  Description:     设置照片的填充属性,删除缩略图
  Input:           pic_id           图片ID
                   fill_flag
  Return:          none
**********************************************************************/

void divoom_pic_phone_manage_set_picture_fill_attr(int pic_id, int fill_flag)
{    
}

/*********************************************************************
  Function:        divoom_pic_phone_manage_app_update_slide_config
  Description:     APP设置相关幻灯片属性
  Input:           pconfig_info           图片ID
                   
  Return:          none
**********************************************************************/

void divoom_pic_phone_manage_app_update_slide_config(Pdivoom_photo_album_album_config_info pconfig_info)
{   
    int update_slide_flag = 0;
    pdivoom_pic_phone_manage_handle->slide_config.play_time = divoom_pic_phone_manage_convert_percent_to_slide_speed(pconfig_info->SlideshowSpeed);
    if(pdivoom_pic_phone_manage_handle->slide_config.play_time < PHOTO_PLAY_MIN_INTERVAL)
    {
        pdivoom_pic_phone_manage_handle->slide_config.play_time = PHOTO_PLAY_MIN_INTERVAL;
    }
    if(pdivoom_pic_phone_manage_handle->slide_config.effect_type < DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_APP_END_TYPE)
    {
        if(pdivoom_pic_phone_manage_handle->slide_config.effect_type != pconfig_info->SlideshowTheme)
        {
            pdivoom_pic_phone_manage_handle->slide_config.effect_type = pconfig_info->SlideshowTheme;
            if(pdivoom_pic_phone_manage_handle->slide_config.cur_play_effect_type < DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_APP_END_TYPE)
            {
                pdivoom_pic_phone_manage_handle->slide_config.cur_play_effect_type = pconfig_info->SlideshowTheme;
                update_slide_flag = 1;
            }
        }
    }
    pdivoom_pic_phone_manage_handle->slide_config.sub_effect_type = pconfig_info->SubSlideshowTheme;
    pdivoom_pic_phone_manage_handle->slide_config.fill_flag = pconfig_info->FillFrame;
    pdivoom_pic_phone_manage_handle->slide_config.bg_type = pconfig_info->FrameBackground;
    if(pdivoom_pic_phone_manage_handle->slide_config.disp_order != pconfig_info->DisplayOrder)
    {
        update_slide_flag = 1;
        pdivoom_pic_phone_manage_handle->slide_config.disp_order = pconfig_info->DisplayOrder;
    }
    if(pdivoom_pic_phone_manage_handle->slide_config.disp_reverse != pconfig_info->ReversePhotoOrder)
    {
        update_slide_flag = 1;
        pdivoom_pic_phone_manage_handle->slide_config.disp_reverse = pconfig_info->ReversePhotoOrder;
    }
    pdivoom_pic_phone_manage_handle->slide_config.title_flag = pconfig_info->ShowTitle;
    pdivoom_pic_phone_manage_handle->slide_config.clock_flag = pconfig_info->ShowClock;
    pdivoom_pic_phone_manage_handle->slide_config.weather_flag = pconfig_info->ShowWeather;
    pdivoom_pic_phone_manage_handle->slide_config.video_auto_play = pconfig_info->VideoAutoPlay;
    pdivoom_pic_phone_manage_handle->slide_config.video_play_flag = pconfig_info->VideoPlayBack;
    pdivoom_pic_phone_manage_handle->slide_config.video_auto_mute = pconfig_info->VideoAutoMute;
    pdivoom_pic_phone_manage_handle->slide_config.video_volume = pconfig_info->VideoVolume;
    divoom_pic_phone_manage_save_slide_config(0);
    if(update_slide_flag)
    {
        divoom_pic_album_manage_set_refresh_album_info();
    }
}

/*********************************************************************
  Function:        divoom_pic_phone_manage_set_picture_rotate_attr
  Description:     设置照片的旋转属性
  Input:           pic_id           图片ID
                   rotate_flag
  Return:          none
**********************************************************************/

void divoom_pic_phone_manage_set_picture_rotate_attr(int pic_id, int rotate_flag)
{    
}


/*********************************************************************
  Function:        divoom_pic_phone_manage_add_picture
  Description:     增加一个图片，这个一般由APP传文件进行触发
  Input:           pic_path
                   album_name
                   file_time
                   photo_time
  Return:          none
**********************************************************************/

int divoom_pic_phone_manage_add_picture(char *pic_path, double file_time, uint32 photo_time, int photo_x, int photo_y,int album_id, int user_id,char *pic_title, int pic_w, int pic_h, int create_thumb_flag)
{    
    int pic_id, out_w, out_h, video_flag, width_flag = 0;
    char thumb_path[256];

    if(strstr(pic_path, ".mp4"))
    {
        video_flag = 1;
    }
    else
    {
        video_flag = 0;
        if(pic_w == 0 || pic_h == 0)
        {
            divoom_rk_mpp_get_pic_width_info(pic_path, &pic_w, &pic_h);
            divoom_print("pic info: %d, %d\n", pic_w, pic_h);
        }
        width_flag = pic_w > pic_h;
    }
    pic_id = divoom_database_manage_add_pic_record(file_time, photo_time, pic_path, photo_x, photo_y, user_id,pic_title, video_flag, width_flag);
    if(create_thumb_flag)
    {
        divoom_pic_phone_manage_create_picture_thumb(pic_path, &out_w, &out_h, NULL);
    }
    if(album_id != 0)
    {
        //divoom_pic_album_manage_add_pic_to_album(album_id, pic_id);    
    }
    divoom_pic_album_manage_set_album_cover(album_id, pic_id, 0, 0);
    return pic_id;
    
}


/*********************************************************************
  Function:        divoom_pic_phone_manage_get_picture_name
  Description:     通过文件ID，得到文件名
  Input:           pic_path
                   pic_id
  Return:          none
**********************************************************************/

void divoom_pic_phone_manage_get_picture_name(char *pic_path, int pic_id)
{    
    divoom_database_manage_get_pic_path_name(pic_id, pic_path);
}

/*********************************************************************
  Function:        divoom_pic_phone_manage_del_picture
  Description:     删除一个图片，这个是设备照片管理手动删除照片
  Input:           pic_id
  Return:          none
**********************************************************************/

void divoom_pic_phone_manage_del_err_pic_name(char *file_name)
{
    int pic_id = divoom_database_manage_get_pic_id_from_path_name(file_name);

    if(pic_id)
    {
        divoom_pic_phone_manage_del_picture(pic_id);
    }
    divoom_app_com_del_file(file_name);
}


/*********************************************************************
  Function:        divoom_pic_phone_manage_del_picture
  Description:     删除一个图片，这个是设备照片管理手动删除照片
  Input:           pic_id
  Return:          none
**********************************************************************/

void divoom_pic_phone_manage_del_picture(int pic_id)
{
    divoom_database_manage_delete_pic(pic_id);
}

typedef struct _divoom_pic_phone_manage_find_pic_ctrl
{
    int update_flag;
    void *find_handle;
    void *pic_handle;
}divoom_pic_phone_manage_find_pic_ctrl, *Pdivoom_pic_phone_manage_find_pic_ctrl;

typedef void divoom_pic_phone_manage_find_pic_from_dir_callback(void *handle, char *file_name, uint32 file_time);
/*********************************************************************
  Function:        divoom_pic_phone_manage_find_pic_from_dir
  Description:     查找这个目录中包括的所有图像文件
  Input:           cur_dir_handle
                   parent_path
                   pcallback
                   para
  Return:          1:成功；0：失败
**********************************************************************/
 

void divoom_pic_phone_manage_find_pic_from_dir(DIR *dp, char *parent_path, divoom_pic_phone_manage_find_pic_from_dir_callback *pcallback, void *para)
{
    char file_ext_name[16];
    DIR *cur_dir_handle;
    int len = strlen(parent_path);
    const divoom_port_dirent_t *dirp;
    divoom_port_stat_t file_info;

    while((dirp = divoom_port_dir_read(dp)) != NULL)
    {
        divoom_print("find: type: %d, %s\n", dirp->is_dir, dirp->name);
        if(dirp->is_dir == 0)
        {
            //普通文件
            divoom_app_com_get_file_ex_name(dirp->name, file_ext_name);
            if(strcasecmp(file_ext_name, "jpg")==0 || strcasecmp(file_ext_name, "webp")==0 || strcasecmp(file_ext_name, "gif")==0 ||
                strcasecmp(file_ext_name, "HEIF")==0 ||strcasecmp(file_ext_name, "jpeg")==0 ||strcasecmp(file_ext_name, "mp4")==0)
            {
                divoom_print("find file: %s\n", dirp->name);
                parent_path[len] = 0;
                strcat(parent_path, dirp->name);
                divoom_print("find file: %s, %s\n", dirp->name, parent_path);
                divoom_port_stat(parent_path, &file_info);
                pcallback(para, parent_path, (time_t)file_info.mtime);
            }
            else
            {
                divoom_print("pass file: %s\n", dirp->name);
            }
        }
        else if(dirp->is_dir)
        {
            //目录
            if(strcmp(dirp->name, "divoom_thumb")== 0 || strcmp(dirp->name, "divoom")== 0 )
            {
                //这个是存放缩略图的目录，不再统计
            }
            else if(!((strcmp(dirp->name, ".")== 0 || strcmp(dirp->name, "..")==0)))
            {
                if(parent_path[len -1 ] != '/')
                {
                    parent_path[len] = '/';
                    len ++;
                }
                parent_path[len] = 0;
                strcat(parent_path, dirp->name);
                cur_dir_handle = divoom_port_dir_open(parent_path);
                parent_path[strlen(parent_path) + 1] = 0;
                parent_path[strlen(parent_path)] = '/';
                divoom_print("find dir: %s, %s\n", dirp->name ,parent_path);
                if(cur_dir_handle)
                {
                    divoom_pic_phone_manage_find_pic_from_dir(cur_dir_handle, parent_path, pcallback, para);
                    divoom_port_dir_close(cur_dir_handle);
                }
                else
                {
                    divoom_print("it fails open dir: %s\n", parent_path);
                    
                }
            }
        }
    }
    
    
    
}

/*********************************************************************
  Function:        divoom_pic_phone_manage_pic_callback
  Description:     本地找到一个文件，添加到图片队列
  Input:           handle  
                   file_name
  Return:          none
**********************************************************************/

void divoom_pic_phone_manage_pic_callback(void *handle, char *file_name, uint32 file_time)
{
    Pdivoom_pic_phone_manage_find_pic_ctrl pctrl_info = (Pdivoom_pic_phone_manage_find_pic_ctrl)handle;

    if(divoom_pic_check_check_file_exist(pctrl_info->find_handle, file_name) == 0)
    {
        divoom_print("find file: %s\n", file_name);
        divoom_pic_phone_manage_add_picture(file_name, (double)file_time, file_time, 0, 0, 0,0, "", 800, 1280, 1);
        pctrl_info->update_flag = 1;
    }
}

/*********************************************************************
  Function:        divoom_pic_phone_manage_read_local_pic
  Description:     设备在连接U盘后退出或本地文件损坏时再重建照片列表时调用
  Input:           reset_all  1:是否清除相册信息
  Return:          1:成功；0：失败
**********************************************************************/

int divoom_pic_phone_manage_read_local_pic(int reset_all)
{
    #if 0
    int i;
    DIR *pic_dir_handle;
    char file_name[256];
    void *handle, *del_pic_queue, *pic_data_handle;

    divoom_print("start search picture!\n");
    if(reset_all)
    {
        divoom_print("start search picture!\n");
        divoom_pic_album_manage_clear_all_type();
    }
    

    handle = divoom_pic_check_init(pdivoom_pic_phone_manage_handle->base_dir);
    if(reset_all == 0)
    {
        //不清空的话，将先读原来的文件信息
            //使用以前的文件信息
        for(i = 0; i < pdivoom_pic_phone_manage_handle->pic_num; i++)
        {
            divoom_database_manage_get_pic_path_name(pdivoom_pic_phone_manage_handle->offset_table[i].id, file_name);
            divoom_pic_check_add_src_path(handle, file_name, pdivoom_pic_phone_manage_handle->offset_table[i].id);
        }
    }
    //查找目录下面的所有图片文件
    pic_dir_handle = divoom_port_dir_open(pdivoom_pic_phone_manage_handle->base_dir);
    if(pic_dir_handle)
    {
        divoom_pic_phone_manage_find_pic_ctrl pctrl_info = {0};
        pctrl_info.find_handle = handle;
        strcpy(file_name, pdivoom_pic_phone_manage_handle->base_dir);
        divoom_pic_phone_manage_find_pic_from_dir(pic_dir_handle, file_name, divoom_pic_phone_manage_pic_callback, &pctrl_info);
        divoom_port_dir_close(pic_dir_handle);
    }
    else
    {
        divoom_print("it fails to open dir: %s\n", pdivoom_pic_phone_manage_handle->base_dir);
    }
    del_pic_queue = divoom_pic_check_get_next_del_file_path(handle);
    if(divoom_queue_items(del_pic_queue))
    {
        Pdivoom_pic_check_item_data pdel_item;
        while(1)
        {
            pdel_item = divoom_queue_pop(del_pic_queue);
            if(pdel_item == NULL)
            {
                break;
            }
            divoom_pic_phone_manage_del_picture(pdel_item->pic_id);
            divoom_system_free(pdel_item->file_name);
            divoom_system_free(pdel_item);
        }
    }
    divoom_queue_destroy(del_pic_queue, NULL);
    divoom_pic_check_destory(handle);
    #endif
    return 1;
}

/*********************************************************************
  Function:        divoom_pic_phone_manage_recreate_list
  Description:     设备在连接U盘后退出或本地文件损坏时再重建照片列表时调用
  Input:           reset_all  1:是否清除相册信息
  Return:          1:成功；0：失败
**********************************************************************/

int divoom_pic_phone_manage_recreate_list(int reset_all)
{
    divoom_system_create_task(divoom_pic_phone_manage_reload_all_pic_file, "pic_reload", reset_all?pdivoom_pic_phone_manage_handle:NULL, DIVOOM_TASK_STACK_PIC_RELOAD);
    return 1;
}

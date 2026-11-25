#ifndef _divoom_photo_album_josn_h_
#define _divoom_photo_album_josn_h_

#include "divoom_download_aliyun_file.h"
typedef struct _divoom_photo_album_album_info
{
    int album_id;
    int album_type;
    char album_name[256];
}divoom_photo_album_album_info, *Pdivoom_photo_album_album_info;

void divoom_photo_album_add_new_album_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_photo_album_add_new_album_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
typedef struct _divoom_photo_album_album_list
{
    int num;
    Pdivoom_photo_album_album_info palbum_list;
}divoom_photo_album_album_list, *Pdivoom_photo_album_album_list;

int divoom_photo_album_get_album_list_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_photo_album_get_album_id_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
typedef struct _divoom_photo_album_album_config_info
{
    int SlideshowSpeed;
    int SlideshowTheme;
    int SubSlideshowTheme;
    int FillFrame;
    int FrameBackground;
    int DisplayOrder;
    int ReversePhotoOrder;
    int ShowTitle;
    int ShowClock;
    int ShowWeather;
    int VideoAutoPlay;
    int VideoPlayBack;
    int VideoAutoMute;
    int VideoVolume;
}divoom_photo_album_album_config_info, *Pdivoom_photo_album_album_config_info;

void divoom_photo_album_set_album_config_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_photo_album_get_album_list_config_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_photo_album_set_album_config_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_photo_album_delete_ali_photo_config_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_photo_album_get_ali_photo_list_config_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
typedef struct _divoom_photo_album_get_ali_photo_info
{
    int num;
    Pdivoom_download_aliyun_file_item plist;
}divoom_photo_album_get_ali_photo_info ,*Pdivoom_photo_album_get_ali_photo_info;

int divoom_photo_album_get_ali_photo_list_config_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_photo_album_app_ali_photo_item_config_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

typedef struct _divoom_photo_album_get_local_photo_respone_info
{
    int album_id;
    int clock_pos;
    int parent_id;
    int start;
    int end;
}divoom_photo_album_get_local_photo_respone_info ,*Pdivoom_photo_album_get_local_photo_respone_info;

typedef struct _divoom_photo_album_get_local_photo_request_item
{
    int server_flag;
    int photo_id;
    int save_time;
    char small_name[128];
    char big_name[128];
}divoom_photo_album_get_local_photo_request_item ,*Pdivoom_photo_album_get_local_photo_request_item;


typedef struct _divoom_photo_album_get_local_photo_request_info
{
    int num;
    Pdivoom_photo_album_get_local_photo_request_item plist;
}divoom_photo_album_get_local_photo_request_info ,*Pdivoom_photo_album_get_local_photo_request_info;


int divoom_photo_album_get_local_photo_list_config_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

typedef struct _divoom_photo_album_cover_respone_info
{
    int album_id;
    int pic_id;
}divoom_photo_album_cover_respone_info ,*Pdivoom_photo_album_cover_respone_info;

int divoom_photo_album_set_photo_album_cover_config_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

typedef struct _divoom_photo_album_app_photo_respone_info
{
    int album_id;
    int parent_id;
    int clock_pos;
    int pic_num;
    uint32 *pic_list;
}divoom_photo_album_app_photo_respone_info ,*Pdivoom_photo_album_app_photo_respone_info;

int divoom_photo_album_delete_photo_list_config_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_photo_album_add_photo_list_config_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_photo_album_remove_photo_list_config_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_photo_album_get_local_photo_list_config_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

typedef struct _divoom_photo_album_app_local_photo_info
{
    int album_id;
    int parent_id;
    int clock_pos;
    int user_id;
    double  save_time;
    int  photo_time;
    int  photo_x;
    int  photo_y;
    int  photo_w;
    int  photo_h;
    int  photo_index;
    int  photo_total;
    int  photo_flag;
    char file_name[128];
    char preview_file_name[128];
    char title[512];
}divoom_photo_album_app_local_photo_info ,*Pdivoom_photo_album_app_local_photo_info;
int divoom_photo_album_add_local_photo_list_config_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

/* 传图进度 UI 占位：由 divoom_lvgl_gallery_transfer 承接 */
void divoom_photo_album_josn_report_transfer_progress(int current, int total);
#endif



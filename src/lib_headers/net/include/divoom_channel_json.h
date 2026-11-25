#ifndef _divoom_channel_json_h_
#define _divoom_channel_json_h_


typedef struct _divoom_channel_item_url_head_info
{
    char key[64];
    char word[512];
}divoom_channel_item_url_head_info, *Pdivoom_channel_item_url_head_info;


typedef struct _divoom_channel_item_disp_data_info
{
    char AnalysisStr[128];
}divoom_channel_item_disp_data_info, *Pdivoom_channel_item_disp_data_info;


typedef struct _divoom_channel_item_info
{
    int AppID;
    int DivoomAppType;
    char AppName[64];
    char PicAddr[64];
    int Type;
    int url_head_num;
    Pdivoom_channel_item_url_head_info purl_head;
    char *UrlInfo;
    char *Config;
    int UpdateTime;
    int DispNumber;
    Pdivoom_channel_item_disp_data_info pDispData;    
}divoom_channel_item_info, *Pdivoom_channel_item_info;

typedef struct _divoom_channel_list_info
{
    int num;
    int all_flag;   //0:更新，１：全部
    int update_time;
    Pdivoom_channel_item_info list;
}divoom_channel_list_info, *Pdivoom_channel_list_info;


typedef struct _divoom_channel_channel_cfg_info
{
    int CurClockId;
    int RotationFlag;
    int ClockTime;
    int GalleryTime;
    int SingleGalleyTime;
    int StartUpClockOnOff;
    int ChannelIndex;
    int PowerOnClockId;
    int GalleryShowTimeFlag;
}divoom_channel_channel_cfg_info, *Pdivoom_channel_channel_cfg_info;


typedef struct _divoom_channel_channel_change_info
{
    int AppID;
    int Flag;
}divoom_channel_channel_change_info, *Pdivoom_channel_channel_change_info;


typedef struct _divoom_channel_set_custom_play_time_info
{
    int clock_id;
    int clock_pos;
    int parent_id;
    int index;
    int play_time;
    int show_time_flag;
    int voice_flag;
}divoom_channel_set_custom_play_time_info, *Pdivoom_channel_set_custom_play_time_info;




typedef struct _divoom_channel_channel_move_info
{
    int SrcID;
    int DestID;
}divoom_channel_channel_move_info, *Pdivoom_channel_channel_move_info;



typedef struct _divoom_channel_get_clock_info
{
    int bright; //亮度
    int time;   //时间间隔
    int index;  //时钟序号
}divoom_channel_get_clock_info, *Pdivoom_channel_get_clock_info;



typedef struct _divoom_channel_set_subscribe_info
{
    int index;
    int type; //订阅类型0:推荐图库1：收藏2：订阅作者
    int mode;   //0：循环最新1：随机全部
    int album_id;
    int play_id;
    int user_num;
    int *user_id_list;  //时钟序号
}divoom_channel_set_subscribe_info, *Pdivoom_channel_set_subscribe_info;

typedef struct _divoom_channel_set_subscribe_index
{
    int index;
    int type; //订阅类型0:推荐图库1：收藏2：订阅作者
}divoom_channel_set_subscribe_index, *Pdivoom_channel_set_subscribe_index;

typedef struct _divoom_channel_eq_item_info
{
    int clock_id;
    int clock_pos;
    int parent_id;
    int eq_index;
    int update_time;
    int eq_num;
    int author_id[12];
    char author_name[12][200];
    char author_file_id[12][64];
    char image_name[12][200];
    char file_addr[12][56];  //时钟序号
}divoom_channel_eq_item_info, *Pdivoom_channel_eq_item_info;

typedef struct _divoom_channel_eq_list_info
{
    int num;
    divoom_channel_eq_item_info *eq_list;  //时钟序号
}divoom_channel_eq_list_info, *Pdivoom_channel_eq_list_info;


typedef struct _divoom_channel_eq_item_select
{
    divoom_clock_id_info clock_info;
    int index;
    int id;
}divoom_channel_eq_item_select, *Pdivoom_channel_eq_item_select;


typedef struct _divoom_channel_update_subscribe_info
{
    divoom_clock_id_info clock_info;
    divoom_channel_set_subscribe_info subscribe_info;
}divoom_channel_update_subscribe_info, *Pdivoom_channel_update_subscribe_info;

void divoom_channel_get_channel_config_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_channel_get_channel_config_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_channel_set_channel_config_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_channel_set_channel_config_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_channel_channel_change_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);


int divoom_channel_channel_move_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_channel_channel_select_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);


void divoom_channel_get_clock_info_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_channel_get_clock_info_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_channel_set_brightness_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_channel_set_produce_time_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

typedef struct _divoom_channel_select_clock_index
{
    int clock_id;
    int sys_update_time; //系统表盘更新时间
}divoom_channel_select_clock_index, *Pdivoom_channel_select_clock_index;

int divoom_channel_set_select_index_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_channel_set_subscribe_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_channel_set_clock_select_index_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);


void divoom_channel_get_custom_list_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_channel_get_custom_list_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_channel_set_custom_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_channel_delete_custom_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_channel_get_custom_time_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_channel_set_custom_time_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_channel_clean_custom_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_channel_get_eq_list_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_channel_get_eq_list_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_channel_add_eq_item_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_channel_del_eq_item_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);


void divoom_channel_get_eq_pos_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_channel_get_eq_pos_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_channel_set_eq_pos_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_channel_set_eq_pos_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_channel_set_start_up_channel_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_channel_set_start_up_channel_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_channel_get_start_up_channel_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_channel_get_start_up_channel_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_channel_set_custom_play_channel_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_channel_set_custom_play_channel_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_channel_set_cloud_channel_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_channel_set_suscribe_play_time_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_channel_set_custom_play_time_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_channel_set_channel_index_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_channel_set_channel_index_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);


void divoom_channel_get_channel_index_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_channel_get_channel_index_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);


typedef struct _divoom_channel_user_define_schedule_info
{
    int clock_id;
    int clock_pos;
    int parent_id;
    uint32 start_time;  //开始时间
    uint32 end_time;    //结束时间
    uint32 cyc_flag;
    int id;
    uint8 status;
    uint8 voice_flag;
    int update_time;
}divoom_channel_user_define_schedule_info, *Pdivoom_channel_user_define_schedule_info;

void divoom_channel_get_subscribe_info_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_channel_get_subscribe_info_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_channel_set_user_define_schedule_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_channel_get_user_define_gallery_time_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_channel_set_all_user_define_schedule_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_channel_init_user_define_schedule_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_channel_set_user_define_schedule_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

typedef struct _divoom_channel_clock_notify_item
{
    int clock_id;
    int type;
    int status;
}divoom_channel_clock_notify_item, *Pdivoom_channel_clock_notify_item;
typedef struct _divoom_channel_clock_notify_list
{
    int num;
    Pdivoom_channel_clock_notify_item plist;
}divoom_channel_clock_notify_list, *Pdivoom_channel_clock_notify_list;

int divoom_channel_get_notify_list_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_channel_set_notify_item_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

typedef struct _divoom_channel_app_clock_config_item
{
    int ItemType;
    char ItemId[128];
    char ItemName[128];
    char ItemValue[128];
}divoom_channel_app_clock_config_item, *Pdivoom_channel_app_clock_config_item;

typedef struct _divoom_channel_app_clock_config_info
{
    int clock_id;
    int num;
    Pdivoom_channel_app_clock_config_item plist;
}divoom_channel_app_clock_config_info, *Pdivoom_channel_app_clock_config_info;

void divoom_channel_set_clock_config_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_channel_set_clock_config_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_channel_get_clock_config_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_channel_get_clock_config_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_channel_get_rgb_info_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_channel_get_rgb_info_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_channel_set_rgb_info_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_channel_set_rgb_info_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

typedef struct _divoom_channel_move_my_clock_info
{
    int clock_id;
    int position;
}divoom_channel_move_my_clock_info, *Pdivoom_channel_move_my_clock_info;
typedef struct _divoom_channel_my_clock_favorite_info
{
    int clock_id;
    int favorite_flag;
}divoom_channel_my_clock_favorite_info, *Pdivoom_channel_my_clock_favorite_info;

void divoom_channel_remove_my_clock_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_channel_set_my_clock_favorite_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_channel_set_my_clock_favorite_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_channel_get_default_clock_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_channel_move_my_clock_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_channel_remove_my_clock_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_channel_set_night_view_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

#endif


#ifndef _divoom_net_alarm_h_
#define _divoom_net_alarm_h_
/*

    "AlarmID": 1,
    "Title": "",
    "Time": 0,
    "Week1": true,
    "Week2": true,
    "Week3": true,
    "Week4": true,
    "Week5": true,
    "Week6": true,
    "Week7": true,
    "SceneType": 0,
    "RingType": 0,
    "MusicType": 0,
    "MusicUri": "",
    "MusicAlbumUri": "",
    "ImageName": "",
"ImageID": "",
"Enable": true,
    "Volume": 0

*/
    
typedef struct _divoom_net_update_alarm_info_
{
    int   alarm_id;
    int   tencent_id;
}divoom_net_update_alarm_info, *Pdivoom_net_update_alarm_info;

typedef struct _divoom_net_alarm_set_alarm_info
{
    int   retquest;
	int   alarm_id;	
	int   utc_time;
	int   update_time;
    int   on_off;
    int   volume;
    int   sound_type;
    int   sound_system;
	int   cyc[7];
    char  image_id[52];
    char  voice_id[52];
    char  alarm_name[64];
}divoom_net_alarm_set_alarm_info, *Pdivoom_net_alarm_set_alarm_info;

typedef struct _divoom_net_alarm_get_alarm_item
{
	int alarm_num;
	Pdivoom_net_alarm_set_alarm_info alarm_list;
}divoom_net_alarm_get_alarm_item, *Pdivoom_net_alarm_get_alarm_item;

typedef struct _divoom_net_alarm_set_alarm_respone_info
{
    int alarm_id;
}divoom_net_alarm_set_alarm_respone_info, *Pdivoom_net_alarm_set_alarm_respone_info;

typedef struct _divoom_net_alarm_listen_alarm_respone_info
{
    int start_flag;
    int sound_type;
    int volume;
}divoom_net_alarm_listen_alarm_respone_info, *Pdivoom_net_alarm_listen_alarm_respone_info;

typedef struct _divoom_net_alarm_get_list_info
{
    int last_update_time;
}divoom_net_alarm_get_list_info, *Pdivoom_net_alarm_get_list_info;

typedef struct _divoom_net_alarm_get_alarm_info
{
    int is_get_all;
    int update_time;
    int alarm_id;
}divoom_net_alarm_get_alarm_info, *Pdivoom_net_alarm_get_alarm_info;

typedef struct _divoom_net_alarm_unactive_alarm_info
{
	uint8 alarm_list_len;
	uint8 *alarm_list;
}divoom_net_alarm_unactive_alarm_info, *Pdivoom_net_alarm_unactive_alarm_info;

typedef struct _divoom_net_alarm_get_update_list_info
{
	int num;
	int alarm_id[256];
}divoom_net_alarm_get_update_list_info, *Pdivoom_net_alarm_get_update_list_info;


void divoom_alarm_json_get_alarm_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_alarm_json_get_alarm_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_alarm_json_set_alarm_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_alarm_json_update_alarm_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_alarm_json_set_alarm_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_alarm_json_get_info_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_alarm_json_get_info_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_alarm_json_update_alarm_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_alarm_json_update_alarm_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_alarm_json_del_alarm_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_alarm_json_del_alarm_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_alarm_json_del_all_alarm_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_alarm_json_del_all_alarm_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_alarm_json_unactive_alarm_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_alarm_json_listen_alarm_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_alarm_json_listen_alarm_set_volume_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);


void divoom_alarm_json_get_update_list_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_alarm_json_get_update_list_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

#endif



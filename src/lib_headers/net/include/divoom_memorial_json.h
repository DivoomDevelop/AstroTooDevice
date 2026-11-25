#ifndef _divoom_memorial_json_h_
#define _divoom_memorial_json_h_

typedef struct _divoom_net_memorial_info_
{
	//int image_id;
	//int device_id[2];
	int memorial_id;
	int  update_time;
	//文件名长度最大51
	char image_data[64];
	char name[256];
	int min;
    int mon;
    int day;
}divoom_net_memorial_info, *Pdivoom_net_memorial_info;

typedef struct _divoom_get_memorial_item_info
{
	uint8 memorial_num;
	Pdivoom_net_memorial_info memorial_list;
}divoom_get_memorial_item_info, *Pdivoom_get_memorial_item_info;

typedef struct _divoom_net_memorial_get_list_info
{
    int count;
}divoom_net_memorial_get_list_info, *Pdivoom_net_memorial_get_list_info;

typedef struct _divoom_net_memorial_get_item_request_info
{
    int id;
}divoom_net_memorial_get_item_request_info, *Pdivoom_net_memorial_get_item_request_info;

typedef struct _divoom_net_memorial_set_memorial_respone_info
{
    int last_update_time;
    int memorial_id;
}divoom_net_memorial_set_memorial_respone_info, *Pdivoom_net_memorial_set_memorial_respone_info;

typedef struct _divoom_net_memorial_delet_memorial_request_info
{
	int device_id[2];
	int memorial_id;
}divoom_net_memorial_delet_memorial_request_info, *Pdivoom_net_memorial_delet_memorial_request_info;

typedef struct _divoom_net_memorial_del_memorial_info_
{
	int memorial_id;
}divoom_net_memorial_del_memorial_info, *Pdivoom_net_memorial_del_memorial_info;


typedef struct _divoom_net_memorial_get_update_list_info
{
	int num;
	int memorial_id[256];
}divoom_net_memorial_get_update_list_info, *Pdivoom_net_memorial_get_update_list_info;


#if 0
void divoom_memorial_json_get_list_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_memorial_json_get_list_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_memorial_json_get_item_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_memorial_json_get_item_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_memorial_json_delet_memorial_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_memorial_json_delet_memorial_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
#else
void divoom_memorial_json_get_item_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_memorial_json_set_item_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_memorial_json_del_item_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_memorial_json_del_all_item_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_memorial_json_set_item_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_memorial_json_get_item_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_memorial_json_del_item_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
#endif


void divoom_memorial_json_get_update_list_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_memorial_json_get_update_list_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);


#endif



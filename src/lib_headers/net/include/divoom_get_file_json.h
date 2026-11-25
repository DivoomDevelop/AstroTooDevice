#ifndef _DIVOOM_GET_FILE_JSON_
#define _DIVOOM_GET_FILE_JSON_

typedef struct _divoom_get_file_hot_list
{
	int vendor_id;
	uint8 file_id[128];
	int version;
}divoom_get_file_hot_list, *Pdivoom_get_file_hot_list;

typedef struct _divoom_get_file_json_hot_info
{
	uint32 file_num;
	Pdivoom_get_file_hot_list hot_list;		
}divoom_get_file_json_hot_info, *Pdivoom_get_file_json_hot_info;

void divoom_get_file_json_hot_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_get_file_json_hot_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

#endif

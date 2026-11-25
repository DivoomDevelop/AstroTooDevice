#ifndef _divoom_voice_json_h_
#define _divoom_voice_json_h_

typedef struct _divoom_voice_json_commit_info
{
    int start_time;
    int total_time;
    int host_id;
    char path[256];
}divoom_voice_json_commit_info, *Pdivoom_voice_json_commit_info;

typedef struct _divoom_voice_json_voice_item_info
{
	char path[128];
	char UserName[64];
	int device_id;
	int record_time;
}divoom_voice_json_voice_item_info, *Pdivoom_voice_json_voice_item_info;
typedef struct _divoom_voice_json_voice_list_info
{
    int num;
    Pdivoom_voice_json_voice_item_info plist; //????????????
}divoom_voice_json_voice_list_info, *Pdivoom_voice_json_voice_list_info;

void divoom_voice_json_commit_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_voice_json_commit_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_voice_json_marked_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_voice_json_marked_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_voice_json_get_list_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_voice_json_get_list_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);


#endif



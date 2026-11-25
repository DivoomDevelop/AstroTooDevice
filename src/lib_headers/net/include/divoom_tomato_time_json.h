#ifndef _divoom_tomato_time_json_
#define _divoom_tomato_time_json_

typedef struct _divoom_tomato_time_info_
{
	uint32 id;
	uint8 name[64];
//	uint8 explain[64]; //说明
	int8 work_time; //工作时间长度  
	int8 long_rest_time; //长休息时长
	int8 short_rest_time; //短休息时长
	uint8 roll_times; //0:仅工作 1~4: 1~4番茄
	uint8 image_file_id[64];
	uint8 sound_on_off; //提示音开关
	uint8 sound_type; //提示音类型  0：系统提示音	1：录音提示音	2：音乐提示音(备用,对于设备和1一样)
	uint8 sound_volume; //音量
	uint8 sound_system_index; //系统提示音编号
	uint8 sound_voice_file_id[64]; //录音文件id
	uint32 target_date; //目标日期那天的UTC
	uint32 estimate_time; //预估时长
	uint32 focus_total_time; //总专注时长
	uint8 focus_total_count; //总专注次数
	uint8 finish_flag; //0没有完成 1完成
	uint8 exist_flag; //0不存在，1存在
	uint8 edit_item; //修改哪个项
	uint8 update_flag; //1有更新 
}divoom_tomato_time_info, *Pdivoom_tomato_time_info;

typedef struct _divoom_tomato_time_get_list_json
{
	uint8 tomato_time_num; //多少个番茄时间
	Pdivoom_tomato_time_info tomato_time_list;
}divoom_tomato_time_get_list_json, *Pdivoom_tomato_time_get_list_json;

typedef struct _divoom_tomato_time_set_json
{
	Pdivoom_tomato_time_info tomato_time;
}divoom_tomato_time_set_json, *Pdivoom_tomato_time_set_json;

typedef struct _divoom_tomato_time_start_json
{
	uint32 tomato_id;
}divoom_tomato_time_start_json, *Pdivoom_tomato_time_start_json;

typedef struct _divoom_tomato_time_focus_action_json
{
	uint32 tomato_id;
	uint32 start_time; //utc
	uint32 end_time; //utc
	uint32 focus_time; //这次专注时长， 分钟为单位
}divoom_tomato_time_focus_action_json, *Pdivoom_tomato_time_focus_action_json;

typedef struct _divoom_tomato_time_focus_done_json
{
	uint32 tomato_id; 
	uint32 start_time; //utc
}divoom_tomato_time_focus_done_json, *Pdivoom_tomato_time_focus_done_json;

typedef struct _divoom_tomato_time_listen_info_json
{
	int start_flag;
	int sound_type;
	int volume;
}divoom_tomato_time_listen_info_json, *Pdivoom_tomato_time_listen_info_json;

void divoom_tomato_time_get_list_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_tomato_time_get_list_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_tomato_time_set_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_tomato_time_set_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_tomato_time_start_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_tomato_time_start_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_tomato_time_focus_action_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_tomato_time_focus_action_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_tomato_time_focus_done_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_tomato_time_focus_done_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_tomato_time_delete_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_tomato_time_delete_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_tomato_time_listen_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_tomato_time_listen_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_tomato_time_set_listen_volume_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_tomato_time_set_listen_volume_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_tomato_time_send_one_by_one_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_tomato_time_send_listcount_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_tomato_time_dev_update_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

#endif

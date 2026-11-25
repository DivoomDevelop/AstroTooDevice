#ifndef _divoom_mixer_music_json_h_
#define _divoom_mixer_music_json_h_

typedef struct _divoom_mixer_music_json_info
{
    int classify;
	int mixer_id;
	int sub_id;
	int on_off;
}divoom_mixer_music_json_info, *Pdivoom_mixer_music_json_info;


typedef struct _divoom_ctrl_mixer_json_info
{
    int classify;
	int is_enter;
	int IsConnectAudio;		//是否重连蓝牙
}divoom_ctrl_mixer_json_info, *Pdivoom_ctrl_mixer_json_info;

typedef struct _divoom_mixer_set_json_info_
{
	uint8 mode;
	uint8 button_select;
	uint8 on_off;
}divoom_mixer_set_json_info, *Pdivoom_mixer_set_json_info;

typedef struct _divoom_mixer_set_new_json_info_
{
	uint8 mode;
	uint8 music_index;
}divoom_mixer_set_new_json_info, *Pdivoom_mixer_set_new_json_info;


int divoom_mixer_send_json_send_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_mixer_send_json_send_respone_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);


int divoom_mixer_ctrl_json_send_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_mixer_send_ctrl_send_respone_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_mixer_set_json_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_mixer_set_new_json_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

#endif


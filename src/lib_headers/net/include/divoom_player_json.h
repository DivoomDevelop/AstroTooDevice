#ifndef _divoom_player_json_h_
#define _divoom_player_json_h_
typedef struct _divoom_player_json_all_info
{
	int status;
	int cur_time;
	int total_time;
	int info_time;
	int volume;
	int type;
	int page;
	char music_url[256];
	char media_id[48];
	int music_index;
	char music_album_url[256];
	int sd_index;
	int music_id;       		//收藏的ID，只在收藏时有用
	char blue_mac[256];
	char sd_name[256];
	int play_mode;
	int is_favorite;
	int favorite_index;
}divoom_player_json_all_info, *Pdivoom_player_json_all_info;

typedef struct _divoom_player_json_set_blue_music_info
{
    int auto_connect;
	char bluc_mac[256];
}divoom_player_json_set_blue_music_info,*Pdivoom_player_json_set_blue_music_info;


typedef struct _divoom_player_json_file_list_info
{
	int start_index;
	int end_index;
	void *file_list_queue;
}divoom_player_json_file_list_info,*Pdivoom_player_json_file_list_info;

typedef struct _divoom_player_json_get_list_info
{
	int last_update_time;
	char file_id[256];
}divoom_player_json_get_list_info, *Pdivoom_player_json_get_list_info;

typedef struct _divoom_player_json_play_ctrl
{
    int type;
    int time;
}divoom_player_json_play_ctrl, *Pdivoom_player_json_play_ctrl;

typedef struct _divoom_player_json_set_favorite_ctrl
{
    int isFavorite;
    int Type;
    int page;
    int music_id;
    int total_time;
    char music_url[256];
    char music_album_url[256];
    char name[256];    
}divoom_player_json_set_favorite_ctrl, *Pdivoom_player_json_set_favorite_ctrl;

typedef struct _divoom_player_json_get_favorite_request
{
    int start;
    int end;
    char music_id[10];
}divoom_player_json_set_favorite_request, *Pdivoom_player_json_set_favorite_request;


typedef struct _divoom_player_json_get_favorite_item
{
    int musicID;
    int type;
    int page;
    char music_url[256];
    char music_album_url[256];
    char name[256];
}divoom_player_json_set_favorite_item, *Pdivoom_player_json_set_favorite_item;


typedef struct _divoom_player_json_get_favorite_respone
{
    int total_count;
    Pdivoom_player_json_set_favorite_item item_list;
}divoom_player_json_set_favorite_respone, *Pdivoom_player_json_set_favorite_respone;

typedef struct
{
	int last_update_time;
	int song_count; 
}divoom_player_json_get_favorite_info_respone, *Pdivoom_player_json_get_favorite_info_respone;


void divoom_player_json_get_info_respone_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_player_json_get_info_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_player_json_music_list_update_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_player_json_music_list_update_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_player_json_music_list_get_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_player_json_music_list_get_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

char *divoom_player_json_music_list_pack(void *queue_handle, uint32 checksum);

uint32 divoom_player_json_music_list_unpack(uint8 *music_string);


int divoom_player_json_set_info_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_player_json_set_info_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);


void divoom_player_json_set_url_respone_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_player_json_set_url_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_player_json_set_status_respone_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_player_json_set_status_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_player_json_set_playe_ctrl_respone_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_player_json_set_playe_ctrl_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_player_json_set_play_mode_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_player_json_set_play_mode_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);



void divoom_player_json_set_vol_respone_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_player_json_set_vol_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_player_json_set_blue_music_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_player_json_set_blue_music_respone_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_player_json_set_favorite_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_player_json_set_favorite_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_player_json_get_favorite_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_player_json_get_favorite_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_player_json_get_favorite_info_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_player_json_get_favorite_info_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

#endif



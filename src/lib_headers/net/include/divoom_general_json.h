#ifndef _divoom_general_json_h_
#define _divoom_general_json_h_

typedef struct _divoom_general_json_version_info
{
    int type;
    int level;
    char version[128];
    char path[256];
    char md5[256];
}divoom_general_json_version_info, *Pdivoom_general_json_version_info;


typedef struct _divoom_general_json_expert_pic_user_info
{
	//尺寸
	int filesize;
    int expert_user_id;
    int start;
    int end;
}divoom_general_json_expert_pic_user_info, *Pdivoom_general_json_expert_pic_user_info;

typedef struct _divoom_general_json_category_list_info
{
	//尺寸
	int filesize;
	int classify;
	int start;
	int end;
}divoom_general_json_category_list_info, *Pdivoom_general_json_category_list_info;

typedef struct _divoom_general_json_expert_pic_item_info
{
    char user_name[64];
    char file_name[64];
    char file_path[64];
    int like_cnt;
    int watch_cnt;
    int user_id;
    int pic_id;
    int image_len;
    uint8 *image_data;
}divoom_general_json_expert_pic_item_info, *Pdivoom_general_json_expert_pic_item_info;

typedef struct _divoom_general_json_expert_pic_list_info
{
    int total;
    int pic_num;
    Pdivoom_general_json_expert_pic_item_info pic_list;
}divoom_general_json_expert_pic_list_info, *Pdivoom_general_json_expert_pic_list_info;

typedef struct
{
	char file_list[64];
}divoom_general_json_get_hot_files_list, *Pdivoom_general_json_get_hot_files_list;

typedef struct
{
	int user_id;
	int version;
	//char file_addr[128];
	//char sha1_buf[64];
	int url_num;
	Pdivoom_general_json_get_hot_files_list file_list;
}divoom_general_json_get_hot_file_item_info, *Pdivoom_general_json_get_hot_file_item_info;

typedef struct
{
	int	all_hot_num;
	int update_time;
	Pdivoom_general_json_get_hot_file_item_info hot_file_list;
}divoom_general_json_get_hot_file_list_info, *Pdivoom_general_json_get_hot_file_list_info;

typedef struct _divoom_general_json_get_update_file_info_request
{
    int hardware;
    int test_flag;
}divoom_general_json_get_update_file_info_request, *Pdivoom_general_json_get_update_file_info_request;



typedef struct _divoom_general_json_get_update_file_info_respone
{
    int type;// 1-自动升级，2-app升级
    int version;
    int hardware; /* Sys/PushUpdate 等下发的 Hardware；0 表示未带 */
    char file_id[256];
    char sha1_sum[64];
	char update_info[512];
}divoom_general_json_get_update_file_info_respone, *Pdivoom_general_json_get_update_file_info_respone;

typedef struct _divoom_general_json_notice_expert_ctrl_
{
	int user_id;
	//可能用不到
	int file_type;
	int update_time;
	//１：增加；０：删除
	uint8 operate_type;
	char file_addr[64];
}divoom_general_json_notice_expert_ctrl, *Pdivoom_general_json_notice_expert_ctrl;

typedef struct _divoom_get_expert_update_list_ctrl_
{
	int expert_id;
	int start_pic_num;
	int end_pic_num;
	int file_type;
	int update_time;
}divoom_get_expert_update_list_ctrl, *Pdivoom_get_expert_update_list_ctrl;

typedef struct _divoom_general_expert_info_ctrl_
{
	int user_id;
	int file_type;
	char file_addr[64];
}divoom_genreal_expert_info_ctrl, *Pdivoom_genreal_expert_info_ctrl;

typedef struct _divoom_get_all_expert_update_list_ctrl_
{
	int									all_add_num;
	int									cur_add_num;
	Pdivoom_genreal_expert_info_ctrl	add_list;
	int									all_del_num;
	int									cur_del_num;
	Pdivoom_genreal_expert_info_ctrl	del_list;
	int									update_time;
}divoom_get_all_expert_update_list_ctrl, *Pdivoom_get_all_expert_update_list_ctrl;

int divoom_general_json_get_version_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_general_json_get_version_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);


int divoom_general_json_get_srv_time_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_general_json_get_srv_time_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);


int divoom_general_json_get_user_info_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_general_json_get_user_info_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_general_json_get_expert_pic_list_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_general_json_get_expert_pic_list_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_general_json_get_user_list_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_general_json_get_user_list_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_general_json_get_update_file_info_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_general_json_get_update_file_info_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_general_json_notice_update_ok_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_general_json_get_category_file_list_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_general_json_get_hot_file_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_general_json_get_hot_file_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_general_json_get_category_file_list_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_general_json_get_custom_file_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_general_json_get_custom_file_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_general_json_get_one_hot_file_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_general_json_get_one_hot_file_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_general_json_del_one_hot_file_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_general_json_del_one_hot_file_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_general_json_notice_expert_gallery_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_general_json_get_expert_update_list_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_general_json_get_expert_update_list_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

#endif




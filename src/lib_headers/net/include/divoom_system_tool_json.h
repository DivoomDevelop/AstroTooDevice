#ifndef _divoom_system_tool_json_h_
#define _divoom_system_tool_json_h_

typedef struct _divoom_system_tool_json_close_time_plan
{
	int plan_id;
}divoom_system_tool_json_close_time_plan, *Pdivoom_system_tool_json_close_time_plan;

typedef struct _divoom_system_tool_json_plan_list
{
	uint8 enable;//这个字项是关闭状态，还是开启状态
	uint8 plan_id;
}divoom_system_tool_json_plan_list, *Pdivoom_system_tool_json_plan_list;

typedef struct _divoom_system_tool_json_get_plan
{
	uint32 plan_num;
	Pdivoom_system_tool_json_plan_list plan_list;		
}divoom_system_tool_json_get_plan, *Pdivoom_system_tool_json_get_plan;

typedef struct _divoom_system_tool_json_time_info
{
    int  status;
    int  hour;
    int  min;
    int  voice_id;
}divoom_system_tool_json_time_info, *Pdivoom_system_tool_json_time_info;


typedef struct _divoom_system_tool_json_score_board
{
    int blue_score;
    int red_score;
}divoom_system_tool_json_score_board, *Pdivoom_system_tool_json_score_board;

typedef struct _divoom_system_tool_json_get_sleep
{
	int status;
	int time;
	uint8 color_flag;
	uint8 color[3];
	int light;
	int vol;
	int mode;
	int scene;
}divoom_system_tool_json_get_sleep, *Pdivoom_system_tool_json_get_sleep;


typedef struct _divoom_system_tool_json_test_sleep
{
    uint8 light;
    uint8 color_flag;
    uint8 color[3];
	 int vol;
	 int scene;
}divoom_system_tool_json_test_sleep, *Pdivoom_system_tool_json_test_sleep;


typedef struct _divoom_system_tool_json_get_plan_item_request_info
{
    int plan_id;
    int plan_pos;
}divoom_system_tool_json_get_plan_item_request_info, *Pdivoom_system_tool_json_get_plan_item_request_info;

typedef struct _divoom_system_tool_json_get_plan_item_info
{
    uint8  voice_switch;
    uint8  cyc_attr;  //如果为0表示非循环，为其它为循环星期
    uint32 start_time;
    uint32 end_time;
    uint8  disp_mode; //0:表盘, 1:云图
    uint8  cyc_flag[7];
    int    clock_id;
    char   plan_name[64];
    char   file_id[56];
}divoom_system_tool_json_get_plan_item_info, *Pdivoom_system_tool_json_get_plan_item_info;

typedef struct _divoom_system_tool_json_get_plan_info
{
	uint8 enable;//这个字项是关闭状态，还是开启状态
	int plan_item_num;
    uint32 update_time;
	Pdivoom_system_tool_json_get_plan_item_info plan_item_list;
}divoom_system_tool_json_get_plan_info, *Pdivoom_system_tool_json_get_plan_info;

typedef struct _divoom_system_tool_json_del_plan_info
{
	int plan_id;
    int update_time;
}divoom_system_tool_json_del_plan_info, *Pdivoom_system_tool_json_del_plan_info;


typedef struct _divoom_system_tool_json_timer_info
{
    int sec;
    int min;
    int status;
}divoom_system_tool_json_timer_info, *Pdivoom_system_tool_json_timer_info;


void divoom_system_tool_json_get_plan_info_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_system_tool_json_get_plan_info_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_system_tool_json_get_plan_item_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_system_tool_json_get_plan_item_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_system_tool_json_modify_plan_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_system_tool_json_modify_plan_respone_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_system_tool_json_del_plan_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_system_tool_json_del_plan_respone_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_system_tool_json_close_plan_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_system_tool_json_close_plan_respone_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_system_tool_json_get_plan_time_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_system_tool_json_get_plan_time_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_system_tool_json_test_sleep_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_system_tool_json_test_sleep_respone_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_system_tool_json_exit_test_sleep_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_system_tool_json_exit_test_sleep_respone_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_system_tool_json_set_sleep_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_system_tool_json_set_sleep_respone_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_system_tool_json_get_sleep_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_system_tool_json_get_sleep_respone_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_system_tool_json_ctrl_sleep_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_system_tool_json_ctrl_sleep_respone_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_system_tool_json_test_url_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_system_tool_json_cancel_plan_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_system_tool_json_cancel_plan_respone_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_system_tool_json_cancel_sleep_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_system_tool_json_cancel_sleep_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);


int divoom_system_tool_json_ctrl_get_timer_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_system_tool_json_ctrl_get_timer_respone_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_system_tool_json_ctrl_set_timer_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_system_tool_json_ctrl_set_stop_watch_respone_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_system_tool_json_ctrl_get_stop_watch_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_system_tool_json_ctrl_get_stop_watch_respone_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_system_tool_json_ctrl_get_noise_respone_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_system_tool_json_ctrl_get_noise_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_system_tool_json_ctrl_set_noise_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);


int divoom_system_tool_json_ctrl_set_score_board_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);


int divoom_system_tool_json_ctrl_get_score_board_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_system_tool_json_ctrl_get_score_board_respone_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_system_tool_json_ctrl_set_stop_watch_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);


#endif


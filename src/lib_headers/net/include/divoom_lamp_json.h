#ifndef _DIVOOM_LAMP_JSON_H_
#define _DIVOOM_LAMP_JSON_H_

typedef struct _divoom_lamp_json_clock_info_
{
	//第几个显示模式
	uint8 disp_mode;
	uint8 clock_light;
	//RGB颜色值
	uint8 color[3];
}divoom_lamp_json_clock_info, *Pdivoom_lamp_json_clock_info;

typedef struct _divoom_lamp_json_add_eq_data_
{
	//最后一次更新的时间
	int update_time;
	//文件服务器最大长度不超过64
	char file_addr[64];
}divoom_lamp_json_add_eq_data, *Pdivoom_lamp_json_add_eq_data;

typedef struct _divoom_lamp_json_del_eq_data_
{
	//最后一次更新的时间
	int update_time;
	uint8 eq_num;
}divoom_lamp_json_del_eq_data, *Pdivoom_lamp_json_del_eq_data;

typedef struct
{
	char eq_file_addr[64];
}divoom_lamp_eq_addr_struct, *Pdivoom_lamp_eq_addr_struct;

typedef struct _divoom_lamp_json_get_eq_list_
{
	//最后一次更新的时间
	int update_time;
	int all_eq_num;
	//id从12开始
	Pdivoom_lamp_eq_addr_struct all_eq_addr;
}divoom_lamp_json_get_eq_list, *Pdivoom_lamp_json_get_eq_list;

typedef struct _divoom_lamp_json_frame_list_
{
	char frame_addr[64];
}divoom_lamp_json_frame_list, *Pdivoom_lamp_json_frame_list;

typedef struct _divoom_lamp_json_frame_info_
{
	//最后一次更新的时间
	int update_time;
	uint8 is_change;
	uint8 list_num;
	Pdivoom_lamp_json_frame_list frame_list;
}divoom_lamp_json_frame_info, *Pdivoom_lamp_json_frame_info;

typedef union
{
	struct{
		uint32	user_id;
		//0-->最新，1-->>全部
		uint8	user_type;
		//达人最后更新时间
		//uint32	expert_update_time;
	}user;
	struct{
		//应该不会超过256个频道吧。。。。。
		uint32	reserve_bits1;
		//uint32	reserve_bits2;
		uint8	channel_id;
		//20191108不需要了
		//char	file_path[64];
	}channel;
}divoom_lamp_expect_ctrl, *Pdivoom_lamp_expect_ctrl;

typedef struct _divoom_lamp_json_expect_sub_
{
	//uint32						update_time;
	//用户信息列表
	//0-->channel, 1-->达人
	uint8						type;
	divoom_lamp_expect_ctrl		expect_info;
}divoom_lamp_json_expect_sub, *Pdivoom_lamp_json_expect_sub;

typedef struct _divoom_lamp_json_set_lamp_
{
	//RGB颜色值
	uint8 color[3];
	//亮度
	uint8 light;
	//是否打开
	uint8 open_flag;
	//模式
	uint8 mode;
}divoom_lamp_json_set_lamp, *Pdivoom_lamp_json_set_lamp;

typedef struct _divoom_lamp_json_get_dev_cfg_
{
	//表的显示模式,从1开始
	divoom_lamp_json_clock_info clock_info;
	//eq激活的id，从1开始
	uint8 eq_effect_id;
	divoom_lamp_json_set_lamp	lamp_info;
}divoom_lamp_json_get_dev_cfg, *Pdivoom_lamp_json_get_dev_cfg;

typedef struct _divoom_lamp_json_get_all_cfg_
{
	divoom_lamp_json_expect_sub		expect_data;
	divoom_lamp_json_frame_info		frame_data;
	divoom_lamp_json_get_eq_list	eq_data;
}divoom_lamp_json_get_all_cfg, *Pdivoom_lamp_json_get_all_cfg;

int divoom_lamp_json_set_clock_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_lamp_json_set_eq_info_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_lamp_json_add_eq_data_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_lamp_json_del_eq_data_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_lamp_json_get_eq_list_requet_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_lamp_json_get_eq_list_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_lamp_json_set_frame_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_lamp_json_get_frame_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_lamp_json_get_frame_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_lamp_json_set_sub_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_lamp_json_get_sub_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_lamp_json_get_sub_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_lamp_json_set_hot_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_lamp_json_set_lamp_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_lamp_json_get_dev_cfg_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_lamp_json_get_dev_cfg_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_lamp_json_get_all_cfg_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_lamp_json_get_all_cfg_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

#endif




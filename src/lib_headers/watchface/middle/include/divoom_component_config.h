#ifndef _divoom_component_config_
#define _divoom_component_config_
#define DIVOOM_COMPONENT_NAME_LEN	20

typedef struct _divoom_options_info
{
	uint16 option_id; //选项id
	uint8 group_name[20]; //组名
	uint8 option_id_vaule[4]; //id字符
	uint8 option_name[40]; //选项名称
	uint8 icon_addr[56]; //icon下载地址
}divoom_options_info,*Pdivoom_options_info;

typedef struct _divoom_component_sys_config
{
	uint32 update_time; //最后更新时间
	uint8 component_name[20]; //组件名称
	int disp_item_num; //组件里面有多少个选项
	Pdivoom_options_info pdisp_item_list; //选项列表
}divoom_component_sys_config,*Pdivoom_component_sys_config;

typedef struct _divoom_component_config_request_component_update_time_item
{
    int update_time;
	uint8 component_name[DIVOOM_COMPONENT_NAME_LEN];
}divoom_component_config_request_component_update_time_item, *Pdivoom_component_config_request_component_update_time_item;

typedef struct _divoom_component_config_request_component_update_time_list
{
    int num;
    Pdivoom_component_config_request_component_update_time_item pcomponent_list;
}divoom_component_config_request_component_update_time_list, *Pdivoom_component_config_request_component_update_time_list;

void divoom_component_config_init(void);
Pdivoom_component_sys_config divoom_component_config_load_sys_component_config(uint8 *component_name);
void divoom_component_config_save_component_update_time(uint8 *component_name, int update_time);
void divoom_component_config_save_component_update_file_from_queue(char *file_name, void *component_queue);
void divoom_component_config_create_sys_component_update_time_file(void);
Pdivoom_component_config_request_component_update_time_list divoom_component_config_get_component_update_time(void);
void divoom_component_free_sys_config(Pdivoom_component_sys_config pconfig_info);
void divoom_component_config_save_sys_component_to_file(Pdivoom_component_sys_config pconfig_info);

Pdivoom_component_sys_config divoom_component_config_read_sys_component_file(uint8 *componet_name);

#endif

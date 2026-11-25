#ifndef _divoom_component_manage_
#define _divoom_component_manage_
#include "divoom_component_config.h"

enum{
	DIVOOM_COMPONET_ID1,
	DIVOOM_COMPONET_ID2,
	DIVOOM_COMPONET_ID3,
	DIVOOM_COMPONET_ID4,
};

typedef struct _divoom_component_table_info
{
    int component_id;
    const char *string;
}divoom_component_table_info;

int divoom_component_manage_get_options_id(uint8 *component_name, uint8 *option_name);

int divoom_component_manage_get_component_id(uint8 *component_name);

void divoom_component_manage_init(void);

void divoom_component_manage_update_component_sys_conf(Pdivoom_component_sys_config pcomponent_info);

void divoom_component_manage_start_request_update(void);

void divoom_component_manage_update_component_list_time(Pdivoom_component_config_request_component_update_time_list pcomponent_list);

void divoom_component_manage_update_loop(void);

int divoom_component_manage_get_disp_item_image(uint8 *component_name, int option_id);

int divoom_component_manage_load_all_pic_from_sys_config(Pdivoom_component_sys_config pcomponent_info);

#endif

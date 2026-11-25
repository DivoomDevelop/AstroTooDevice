#ifndef _divoom_clock_play_plan_json_h_
#define _divoom_clock_play_plan_json_h_

#include "divoom_clock_play_plan.h"

typedef struct _divoom_clock_play_plan_json_switch_info
{
    uint8 is_enable;
}divoom_clock_play_plan_json_switch_info, *Pdivoom_clock_play_plan_json_switch_info;

typedef struct _divoom_clock_play_plan_json_cycle_info
{
    uint32 item_num;
    Pdivoom_clock_play_plan_cycle_item item_list;
}divoom_clock_play_plan_json_cycle_info, *Pdivoom_clock_play_plan_json_cycle_info;

typedef struct _divoom_clock_play_plan_json_date_info
{
    uint8  has_date_rule_list;
    uint32 item_num;
    Pdivoom_clock_play_plan_date_item item_list;
}divoom_clock_play_plan_json_date_info, *Pdivoom_clock_play_plan_json_date_info;

void divoom_clock_play_plan_json_get_switch_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_clock_play_plan_json_get_switch_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_clock_play_plan_json_get_cycle_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_clock_play_plan_json_get_cycle_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_clock_play_plan_json_get_date_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_clock_play_plan_json_get_date_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_clock_play_plan_json_set_date_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_clock_play_plan_json_set_cycle_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

#endif

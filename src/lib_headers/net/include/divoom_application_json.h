#ifndef _divoom_application_json_h_
#define _divoom_application_json_h_

typedef struct _divoom_application_json_realtimeinfo_item
{
    int  item_type;
    char item_id[64];
    char item_value[64];

}divoom_application_json_realtimeinfo_item, *Pdivoom_application_json_realtimeinfo_item;

int divoom_application_json_realinfo_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_application_json_getselect_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_application_json_getselect_respone_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
#endif

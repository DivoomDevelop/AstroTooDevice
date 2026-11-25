#ifndef _divoom_lamda_json_
#define _divoom_lamda_json_

int divoom_lamda_screen_ctrl_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_lamda_display_pic_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_lamda_light_ctrl_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_lamda_switch_clock_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_lamda_recommend_pic_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_indo);
int divoom_lamda_favorite_pic_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_indo);
int divoom_lamda_subscribe_pic_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_indo);

#endif

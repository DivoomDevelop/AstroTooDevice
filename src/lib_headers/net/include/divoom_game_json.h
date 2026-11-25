#ifndef _divoom_game_json_h_
#define _divoom_game_json_h_

void divoom_game_json_enter_respone_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_game_json_enter_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_game_json_exit_respone_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_game_json_exit_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_game_json_play_respone_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_game_json_play_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

#endif



#ifndef _divoom_net_command_json_h_
#define _divoom_net_command_json_h_
Pdivoom_net_command divoom_net_command_json_unpacket(char * cmd_string, uint8 need_free);
char * divoom_net_command_json_packet(Pdivoom_net_command pcmd_info);
char *divoom_net_command_json_packet_get_cmd_string(Pdivoom_net_command pcmd_info);
Pdivoom_net_command divoom_net_command_json_http_unpacket(divoom_cJSON *root, char *cmd_string);
#endif


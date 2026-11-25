#ifndef _divoom_ifft_json_h_
#define _divoom_ifft_json_h_

void divoom_ifft_alarm_start_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_ifft_time_plan_start_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_ifft_memorial_stop_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_ifft_alarm_stop_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_ifft_sleep_start_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_ifft_memorial_start_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_ifft_countdown_start_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_ifft_clock_start_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_ifft_screen_ctrl_start_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_ifft_play_user_define_start_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_ifft_play_net_gif_start_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_ifft_channel_start_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_ifft_white_start_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_ifft_back_color_start_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_ifft_voice_start_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_ifft_sleep_stop_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_ifft_switch_clock_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_ifft_set_brightness_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_ifft_play_hot_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

#endif



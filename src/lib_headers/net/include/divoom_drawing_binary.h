#ifndef _divoom_drawing_binary_
#define _divoom_drawing_binary_

int divoom_drawing_local_binary_get_image_flag(uint8 *data, int data_len, uint32 *user_id, uint32 *packet_id);
int divoom_drawing_local_binary_send_request_unpack(uint8 *data, int data_len, Pdivoom_net_command pcmd_info); //本地动画
int divoom_drawing_local_ble_binary_send_request_unpack(uint8 *data, int data_len, Pdivoom_net_command pcmd_info); //本地动画
int divoom_drawing_sync_binary_send_request_unpack(uint8 *data, int data_len, Pdivoom_net_command pcmd_info);

#endif

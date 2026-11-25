#ifndef _divoom_net_command_binary_
#define _divoom_net_command_binary_

int divoom_net_command_bin_get_cmd(int cmd);
Pdivoom_net_command divoom_net_command_bin_unpacket(uint8 *data, int data_len, uint8 need_free, uint32 device_id);

#endif

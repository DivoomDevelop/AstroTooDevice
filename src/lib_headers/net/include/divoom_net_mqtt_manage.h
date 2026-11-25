#ifndef _divoom_net_mqtt_manage_h_
#define _divoom_net_mqtt_manage_h_

#include "divoom_wifi_con.h"
#include "divoom_mqtt_model.h"
#include "divoom_server_msg_manage.h"

void divoom_net_mqtt_manage_recv_callback(MQTT_STATUS status, void *handle, uint8_t *topic, uint8_t *cmd_string, uint32_t data_len);
void *divoom_net_mqtt_manage_send(Pdivoom_net_command pcmd_info);
int divoom_net_mqtt_manage_send_checked(Pdivoom_net_command pcmd_info);
void divoom_net_mqtt_manage_init(int device_id);
void divoom_net_mqtt_update_srv_time(int time);
int divoom_net_mqtt_get_net_status(void);
void divoom_net_mqtt_manage_update_device_id(int device_id);

//调用此函数进行所有的MQTT的断开,资源释放
//server部分,手机部分
void divoom_net_mqtt_manage_exit(void);
void divoom_net_mqtt_manage_disconnect_server(void);
#endif

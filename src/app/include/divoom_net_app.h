#ifndef _divoom_net_app_h_
#define _divoom_net_app_h_

enum
{
    DIVOOM_NET_APP_COMMAND_DIRECT_RET,
    DIVOOM_NET_APP_COMMAND_NEED_RESPONE,
};

typedef enum _divoom_net_app_user_info_source
{
    DIVOOM_NET_APP_USER_INFO_SOURCE_NONE = 0,
    DIVOOM_NET_APP_USER_INFO_SOURCE_HOT,
    DIVOOM_NET_APP_USER_INFO_SOURCE_FAVORITE,
    DIVOOM_NET_APP_USER_INFO_SOURCE_ALBUM,
    DIVOOM_NET_APP_USER_INFO_SOURCE_EXPERT,
    DIVOOM_NET_APP_USER_INFO_SOURCE_USER_DEFINE,
    DIVOOM_NET_APP_USER_INFO_SOURCE_USER_IMAGE_LIST,
    DIVOOM_NET_APP_USER_INFO_SOURCE_USER_HEAD,
}divoom_net_app_user_info_source;

void divoom_net_app_init(void);

void divoom_net_app_send_packet(uint16 com, uint8 type, void *data);
int divoom_net_app_send_packet_checked(uint16 com, uint8 type, void *data);
/* 异步入队成功返回包号，失败返回 0。调用方可用包号核对回包。 */
uint32 divoom_net_app_send_packet_tracked(uint16 com, uint8 type, void *data);
void divoom_net_app_request_user_info_list(divoom_net_app_user_info_source source, void *request_info);
void divoom_net_app_free_packet(Pdivoom_net_command pcmd_info);
void divoom_net_app_callback(Pdivoom_net_command pnet_info);	//请求设备的命令
uint8 divoom_net_app_net_is_ok(void);
bool divoom_net_app_command_ok(Pdivoom_net_command pcmd_info);
void divoom_net_app_send_sd_card_status(bool statu);
void divoom_net_app_send_music_list(void *file_list_queue, int start_index, int end_index);
void *divoom_net_app_do_comm_malloc(void *msg_data);
void *divoom_net_app_do_comm_free(void *msg_data);

/*
  Function:       divoom_net_app_send_packet
  Description:	   发送命令数据包
  Input:        com 命令
  				type 发给服务器或者app
  				data 命令所带的数据, 谁分配谁释放
  				wait_flag 1等待返回数据 0不用等待数据
  Return:         ret_data 不为空时外部要释放
  Author:         wangft
  Data:           2020-12-18
*/
void *divoom_net_app_send_packet_wait(uint16 com, uint8 type, void *data);

/*
  Function:       divoom_net_app_push_cmd
  Description:	   把命令压入队列
  Input:          pnet_info 命令信息，外部分配，处理完之后内部释放
  Return:         NONE
  Author:         wangft
  Data:           2020-12-18
*/
void divoom_net_app_push_cmd(Pdivoom_net_command pnet_info);
/*
  Function:       divoom_net_app_push_quick_cmd
  Description:	   把命令压入队列头，加速处理
  Input:          pnet_info 命令信息，外部分配，处理完之后内部释放
  Return:         NONE
  Author:         aijun
  Data:           2020-12-18
*/
void divoom_net_app_push_quick_cmd(Pdivoom_net_command pnet_info);

void *divoom_net_app_send_packet_http(uint16 com, void *data, uint8 wait_flag);
void *divoom_net_app_send_packet_http_guarded(uint16 com, void *data, uint8 wait_flag, int (*allow)(void *, int), void *user);
void divoom_net_app_send_packet_lwt(uint16 com, uint8 type, void *data);

/*
  Function:       divoom_net_app_clear_cmd
  Description:	   把命令全部清除
  Input:          cmd_type
  Return:         NONE
*/

void divoom_net_app_clear_cmd(T_U16 cmd_type);

#endif

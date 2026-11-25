#ifndef  _divoom_bt_ctrl_h_
#define _divoom_bt_ctrl_h_


typedef enum
{
	DIVOOM_BT_EVENT_INIT,
	DIVOOM_BT_EVENT_ENABLE_BLE,
	DIVOOM_BT_EVENT_SW_BLE_NAME,

}DIVOOM_BT_EVENT_ID;

typedef struct _divoom_bt_msg_
{
	uint8 msg_id;
	int value;
}divoom_bt_msg,*Pdivoom_bt_msg;

void divoom_bt_ctrl_put_msg(divoom_bt_msg *pmsg);
void divoom_bt_ctrl_init(DIVOOM_BT_STATUS_CB callback);
void divoom_bt_ctrl_music_play_ctrl(uint8 play);
void divoom_bt_ctrl_music_play_next(void);
void divoom_bt_ctrl_music_play_prev(void);
void divoom_bt_ctrl_start_ble(void);
void divoom_bt_ctrl_ble_send_data(uint8_t *data, uint8_t len);
void divoom_bt_ctrl_stop_ble(void);
void divoom_bt_ctrl_reset_ble(void);
//进入退出配网模式，蓝牙会重启
void divoom_bt_ctrl_set_wifi_config(int wifi_config_flag);

void divoom_bt_ctrl_clear_config(void);

int divoom_bt_ctrl_get_music_status(void);

//同步设置手机音量
void divoom_bt_ctrl_set_sync_volume(int volume);


#endif 



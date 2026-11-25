#include "divoom_build_config.h"
#if !DIVOOM_BUILD_RTOS_DEVICE
#error "RTOS device-only stub; exclude from simulator VS project."
#endif

#include "divoom.h"
#include "divoom_bt_ctrl.h"
#include "divoom_com.h"
#include "btcore_uart.h"

#define DIVOOM_BTCORE_APP_TX_HDR_LEN  3u
#define DIVOOM_BTCORE_APP_TX_BLE      0u

void divoom_bt_ctrl_put_msg(divoom_bt_msg *pmsg) { (void)pmsg; }
void divoom_bt_ctrl_init(DIVOOM_BT_STATUS_CB callback) { (void)callback; }
void divoom_bt_ctrl_music_play_ctrl(uint8 play) { (void)play; }
void divoom_bt_ctrl_music_play_next(void) {}
void divoom_bt_ctrl_music_play_prev(void) {}

void divoom_bt_ctrl_start_ble(void)
{
	if (divoom_btcore_uart_cmd_send_ble_ctrl(DIVOOM_BTCORE_BLE_CTRL_ON) != 0)
		divoom_print("bt_ctrl: enable BLE adv failed\n");
}

void divoom_bt_ctrl_ble_send_data(uint8_t *data, uint8_t len)
{
	uint8_t *frame;
	uint16_t frame_len;

	if (!data || len == 0)
		return;
	if (!divoom_btcore_uart_cmd_is_ready())
		return;

	frame_len = (uint16_t)(DIVOOM_BTCORE_APP_TX_HDR_LEN + len);
	if (frame_len > BTCORE_MAX_CMD_DATA_LEN)
	{
		divoom_print("bt_ctrl ble send too large: %u\n", (unsigned)len);
		return;
	}

	frame = (uint8_t *)divoom_system_malloc(frame_len);
	if (!frame)
		return;

	frame[0] = DIVOOM_BTCORE_APP_TX_BLE;
	divoom_com_pack_uint16(&frame[1], len);
	memcpy(&frame[DIVOOM_BTCORE_APP_TX_HDR_LEN], data, len);
	divoom_btcore_uart_cmd_send(DM_BTCORE_APP_DATA_TRANSMIT, frame, frame_len);
	divoom_system_free(frame);
}

void divoom_bt_ctrl_stop_ble(void)
{
	if (divoom_btcore_uart_cmd_send_ble_ctrl(DIVOOM_BTCORE_BLE_CTRL_OFF) != 0)
		divoom_print("bt_ctrl: disable BLE adv failed\n");
}

void divoom_bt_ctrl_reset_ble(void) {}
void divoom_bt_ctrl_set_wifi_config(int wifi_config_flag) { (void)wifi_config_flag; }
void divoom_bt_ctrl_clear_config(void) {}
int divoom_bt_ctrl_get_music_status(void) { return 0; }
void divoom_bt_ctrl_set_sync_volume(int volume) { (void)volume; }

#ifndef _LBH_BLE_H_
#define _LBH_BLE_H_

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief     Prerequisite of terminating LBH
 *
 * @attention Must be invoked from thread other than LBH Client thread
 *
*/
int lbh_open(void);
//void lbh_terminate();
void client_queue_msg_callback(uint8_t *data, uint16_t length);
bool lbh_ble_is_initialized(void);
bool lbh_ble_is_deinitialized(void);
void lbh_ble_print_status(void);
int lbh_ble_deinit(void);
int lbh_ble_reinit(void);
int lbh_ble_get_local_addr(void);
int lbh_ble_set_advertising(bool enable);
int lbh_ble_set_name(const char *name);
int lbh_ble_disconnect(int conidx);
int lbh_ble_send_notification(const uint8_t *data, uint16_t len);
int lbh_ble_send_indication(const uint8_t *data, uint16_t len);
int lbh_ble_update_read_value(const uint8_t *data, uint16_t len);
int lbh_ble_send_notification_uuid(const uint8_t *uuid, const uint8_t *data,
                                   uint16_t len);
int lbh_ble_send_indication_uuid(const uint8_t *uuid, const uint8_t *data,
                                 uint16_t len);
int lbh_ble_update_read_value_uuid(const uint8_t *uuid, const uint8_t *data,
                                   uint16_t len);

#endif//_LBH_BLE_H_

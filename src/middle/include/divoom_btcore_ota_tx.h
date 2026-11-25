#ifndef _DIVOOM_BTCORE_OTA_TX_H_
#define _DIVOOM_BTCORE_OTA_TX_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*divoom_btcore_ota_tx_progress_fn)(void *user_data, uint32_t sent, uint32_t total);

/*
 * T113 → JL 固件 OTA（0x59 START → 0x5a DATA 循环 → 0x5b END）：
 *   每步等待 JL 应答 err==0 后再继续。
 *   fw_version：目标固件版本（小端，与云端 Version 一致）。
 *   flags：bit0=FORCE_FULL 强制全量升级。
 */
int divoom_btcore_ota_tx_send_firmware(const char *local_path, uint32_t fw_version, uint8_t flags,
				       divoom_btcore_ota_tx_progress_fn progress_cb, void *user_data);

int divoom_btcore_ota_tx_is_busy(void);

#ifdef __cplusplus
}
#endif

#endif /* _DIVOOM_BTCORE_OTA_TX_H_ */

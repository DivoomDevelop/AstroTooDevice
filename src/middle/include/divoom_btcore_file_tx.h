#ifndef _DIVOOM_BTCORE_FILE_TX_H_
#define _DIVOOM_BTCORE_FILE_TX_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*divoom_btcore_file_tx_progress_fn)(void *user_data, uint32_t sent, uint32_t total);

/*
 * T113 → JL 固件/文件传输（与 divoom_btcore_file_rx 方向相反）：
 *   0x17 文件头 → JL 应答 1 字节 status(0=OK)
 *   0x18 数据包 → JL 应答 1 字节 result(0=OK)
 *   0x3D JL 请求补包（传输过程中）
 *   0x3E 传输结束
 */
int divoom_btcore_file_tx_send_file(const char *local_path, const char *jl_dest_path,
				    uint16_t block_len,
				    divoom_btcore_file_tx_progress_fn progress_cb, void *user_data);

int divoom_btcore_file_tx_is_busy(void);

void divoom_btcore_file_tx_on_missed_packets(const uint8_t *cmd_data, uint16_t cmd_len);

#ifdef __cplusplus
}
#endif

#endif /* _DIVOOM_BTCORE_FILE_TX_H_ */

#ifndef _DIVOOM_BTCORE_FILE_RX_H_
#define _DIVOOM_BTCORE_FILE_RX_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DIVOOM_BTCORE_FILE_RX_MAX_PATH  256

typedef void (*divoom_btcore_file_rx_saved_fn)(const char *path, int ok);
typedef void (*divoom_btcore_file_rx_progress_fn)(uint8_t percent);

/* 第一步：记录 T113 侧期望保存的歌单文件路径与本地 CRC16 */
int divoom_btcore_file_rx_set_save_path(const char *path, uint16_t expect_crc16);

const char *divoom_btcore_file_rx_get_save_path(void);

uint16_t divoom_btcore_file_rx_get_expect_crc16(void);

void divoom_btcore_file_rx_set_saved_callback(divoom_btcore_file_rx_saved_fn cb);

/* dest 为 CHECK_LINK fw_path（tf_update.zip）时回调；与歌单 saved_cb 独立 */
void divoom_btcore_file_rx_set_fw_saved_callback(divoom_btcore_file_rx_saved_fn cb);

/* dest 为 tf_update.zip 时按收包比例回调 percent */
void divoom_btcore_file_rx_set_fw_progress_callback(divoom_btcore_file_rx_progress_fn cb);

void divoom_btcore_file_rx_reset(void);

/* CTX_SLOW 空闲时轮询：收包停滞则发 0x3D，避免丢掉尾包后永不补传 */
void divoom_btcore_file_rx_poll(void);

/* 计算文件 CRC16（与杰里 divoom_com_crc16 / CCITT 查表一致，初值 0） */
uint16_t divoom_btcore_file_rx_crc16_file(const char *path);

uint16_t divoom_btcore_file_rx_crc16_data(const uint8_t *data, uint32_t len);

/* 链式 CRC16（与杰里 divoom_com_crc16(data, len, crc) 一致，初值 0） */
uint16_t divoom_btcore_file_rx_crc16_chain(const uint8_t *data, uint32_t len, uint16_t crc);

/* 第二步：JL 发送文件头 DM_BTCORE_FILE_HEAD_TRANSMIT
 * 载荷：size(4 LE) + path_len(1) + dest_path(path_len) + block_len(2 LE)
 * dest 必须非空且在 /usr 或 /userdata 下；按 dest 落盘。set_save_path 只给 0x42 回路径用
 */
void divoom_btcore_file_rx_on_head(const uint8_t *cmd_data, uint16_t cmd_len);

/* 第三步：JL 连续发送文件包；按 packet_id 边收边写，位图记缺包 */
void divoom_btcore_file_rx_on_block(const uint8_t *cmd_data, uint16_t cmd_len);

/* 第四步：JL 发送结束 DM_BTCORE_FILE_END */
void divoom_btcore_file_rx_on_end(void);

/*
 * JL 查询 DM_BTCORE_SONGLIST_INFO(0x42，cmd_len=0)：
 * T113 回复 songlist_crc16(2) + timestamp(4) + songlist_path_len(1) + songlist_path
 */
void divoom_btcore_file_rx_on_songlist_reply(const uint8_t *cmd_data, uint16_t cmd_len);

#ifdef __cplusplus
}
#endif

#endif /* _DIVOOM_BTCORE_FILE_RX_H_ */

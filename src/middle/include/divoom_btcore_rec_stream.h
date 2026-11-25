#ifndef _DIVOOM_BTCORE_REC_STREAM_H_
#define _DIVOOM_BTCORE_REC_STREAM_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 固定录音参数（与 JL 写死一致，协议不传） */
#define DIVOOM_BTCORE_REC_STREAM_SAMPLE_RATE  16000u
#define DIVOOM_BTCORE_REC_STREAM_BIT_RATE     32000u
#define DIVOOM_BTCORE_REC_STREAM_CHANNELS     1u
#define DIVOOM_BTCORE_REC_STREAM_MAX_DUR_SEC  60u

/* 0x64 reason：0～5 由 JL 发送，见 JL divoom/docs/F101_MIC录音互斥联调说明.md */
#define DIVOOM_BTCORE_REC_STREAM_END_OK              0u
#define DIVOOM_BTCORE_REC_STREAM_END_ENC_ERR         1u
#define DIVOOM_BTCORE_REC_STREAM_END_TX_ERR          2u
#define DIVOOM_BTCORE_REC_STREAM_END_TIMEOUT         3u
#define DIVOOM_BTCORE_REC_STREAM_END_PREEMPT_CALL    4u
#define DIVOOM_BTCORE_REC_STREAM_END_PREEMPT_UAC     5u
/* F101 本地合成（JL 不会发送） */
#define DIVOOM_BTCORE_REC_STREAM_END_START_TIMEOUT   0x80u
#define DIVOOM_BTCORE_REC_STREAM_END_STOP_TIMEOUT    0x81u

#define DIVOOM_BTCORE_REC_STREAM_MAX_PATH     256u

#ifndef DIVOOM_BTCORE_REC_STREAM_DEFAULT_PATH
#define DIVOOM_BTCORE_REC_STREAM_DEFAULT_PATH DIVOOM_SD_SYSTEM_DIR "/jl_mic_rec.mp3"
#endif

typedef enum {
	DIVOOM_BTCORE_REC_STREAM_STATE_IDLE = 0,
	DIVOOM_BTCORE_REC_STREAM_STATE_WAIT_FIRST_PKT,
	DIVOOM_BTCORE_REC_STREAM_STATE_RECORDING,
	DIVOOM_BTCORE_REC_STREAM_STATE_STOPPING,
	DIVOOM_BTCORE_REC_STREAM_STATE_END,
} divoom_btcore_rec_stream_state_e;

typedef struct {
	uint32_t total_bytes;
	uint16_t total_packets;
	uint16_t payload_crc;
	uint8_t  reason;
	uint8_t  crc_ok;
	uint8_t  bytes_ok;
	uint8_t  packets_ok;
} divoom_btcore_rec_stream_end_t;

typedef void (*divoom_btcore_rec_stream_done_fn)(const char *path,
						 const divoom_btcore_rec_stream_end_t *end,
						 int ok);

static inline int divoom_btcore_rec_stream_reason_is_preempt(uint8_t reason)
{
	return reason == DIVOOM_BTCORE_REC_STREAM_END_PREEMPT_CALL
	       || reason == DIVOOM_BTCORE_REC_STREAM_END_PREEMPT_UAC;
}

static inline int divoom_btcore_rec_stream_reason_is_jl(uint8_t reason)
{
	return reason <= DIVOOM_BTCORE_REC_STREAM_END_PREEMPT_UAC;
}

/* F101→JL：0x20 RECORD，data[0]=1 开始 / 0 停止；data[1]=0 MIC */
int divoom_btcore_rec_stream_send_record(uint8_t start, uint8_t mode);

int divoom_btcore_rec_stream_set_save_path(const char *path);
const char *divoom_btcore_rec_stream_get_save_path(void);

void divoom_btcore_rec_stream_set_done_callback(divoom_btcore_rec_stream_done_fn cb);
void divoom_btcore_rec_stream_reset(void);

divoom_btcore_rec_stream_state_e divoom_btcore_rec_stream_get_state(void);
int divoom_btcore_rec_stream_is_active(void);

const divoom_btcore_rec_stream_end_t *divoom_btcore_rec_stream_get_end(void);

/* JL→F101 收包入口（由 divoom_btcore_uart_cmd 分发） */
void divoom_btcore_rec_stream_on_data(const uint8_t *data, uint16_t len);
void divoom_btcore_rec_stream_on_end(const uint8_t *data, uint16_t len);

/* UART RX 快速入队 + worker 写盘（btcore_uart_rtos 调用） */
int divoom_btcore_rec_stream_init(void);
int divoom_btcore_rec_stream_rx_enqueue(uint8_t cmd, const uint8_t *data, uint16_t len);

/* 发送 0x20 RECORD 前同步本地会话；资源不足时阻止远端开始录音。 */
int divoom_btcore_rec_stream_on_tx_record(uint8_t action, uint8_t mode);

#ifdef __cplusplus
}
#endif

#endif /* _DIVOOM_BTCORE_REC_STREAM_H_ */

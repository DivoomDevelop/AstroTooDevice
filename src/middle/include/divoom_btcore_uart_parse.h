#ifndef _DIVOOM_BTCORE_UART_PARSE_H_
#define _DIVOOM_BTCORE_UART_PARSE_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * BtCore UART 帧组包（0xAA / L / cmd / data / chk / 0xBB），见 btcore_uart.h。
 * RTOS btcore_rx 线程专用；整帧收齐后回调 on_frame，校验与入队由调用方处理。
 * on_frame 返回 0 表示帧已接受；非 0 时解析器滑 1 字节重同步。
 */

typedef int (*divoom_btcore_uart_parse_on_frame_fn)(const uint8_t *frame, int frame_len, void *ctx);

void divoom_btcore_uart_parse_init(divoom_btcore_uart_parse_on_frame_fn on_frame, void *ctx);

void divoom_btcore_uart_parse_reset(void);

/* 唯一解析入口，对标 divoom_app_protocol_AcceptData(handle, buf, size) */
void divoom_btcore_uart_parse_feed(const uint8_t *buf, uint32_t size);

/* ring_get → parse_feed；解析器不依赖 HAL */
void divoom_btcore_uart_parse_drain_ring(void *ring);

#ifdef __cplusplus
}
#endif

#endif /* _DIVOOM_BTCORE_UART_PARSE_H_ */

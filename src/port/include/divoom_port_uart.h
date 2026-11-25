#ifndef DIVOOM_PORT_UART_H
#define DIVOOM_PORT_UART_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 逻辑端口号。真机实现映射到 HAL UART_3（[uart3] PD0/PD1）。
 * middle 只使用这个常量，不要再引用 UART_3。
 */
#define DIVOOM_PORT_UART_BTCORE  3

int  divoom_port_uart_open(int port, int baud);
void divoom_port_uart_close(int port);
int  divoom_port_uart_write(int port, const uint8_t *buf, size_t len);
/*
 * timeout_ms==0：不等待，最多读 len。
 * timeout_ms>0：最多等这么久才有第一批数据；无环缓冲的实现最多等 1 字节，
 * 避免为凑满 len 逐字节阻塞。
 * 返回读到的字节数；0 表示超时无数据；<0 失败。
 */
int  divoom_port_uart_read(int port, uint8_t *buf, size_t len, int timeout_ms);

typedef struct {
	int      buffered;    /* 1=有 RX 环，0=逐字节 mailbox */
	uint32_t cap;
	uint32_t pending;
	uint32_t high_water;
	uint32_t drop;
} divoom_port_uart_rx_info_t;

int divoom_port_uart_rx_info(int port, divoom_port_uart_rx_info_t *out);

#ifdef __cplusplus
}
#endif

#endif /* DIVOOM_PORT_UART_H */

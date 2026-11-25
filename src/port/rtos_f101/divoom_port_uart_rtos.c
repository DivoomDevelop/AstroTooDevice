#include "divoom_build_config.h"
#if !DIVOOM_BUILD_RTOS_DEVICE
#error "divoom_port_uart_rtos.c is RTOS device-only."
#endif

#include "divoom_port_uart.h"

#include <hal_uart.h>
#include <stdio.h>
#include <string.h>

/*
 * UART3 为 2 线无流控、JL 侧主动推送，接收端唯一的防线是
 * 「一线缓冲深度 × 排空速度」。1.5Mbps 8N1 折 150KB/s，32KB 约合 218ms，
 * 足以覆盖 LVGL(prio 10)/audio_sink(prio 11) 对 btcore_rx 的抢占。
 */
#define DIVOOM_PORT_UART_RX_RING_SIZE  32768u

static int s_open;
static int s_hal_port = -1;
static int s_ring_on;

static int divoom_port_uart_to_hal_port(int port)
{
	if (port != DIVOOM_PORT_UART_BTCORE)
		return -1;
	return (int)UART_3;
}

static int divoom_port_uart_baud_to_hal(int baudrate)
{
	switch (baudrate) {
	case 9600:    return UART_BAUDRATE_9600;
	case 19200:   return UART_BAUDRATE_19200;
	case 38400:   return UART_BAUDRATE_38400;
	case 57600:   return UART_BAUDRATE_57600;
	case 115200:  return UART_BAUDRATE_115200;
	case 230400:  return UART_BAUDRATE_230400;
	case 460800:  return UART_BAUDRATE_460800;
	case 921600:  return UART_BAUDRATE_921600;
	case 1500000: return UART_BAUDRATE_1500000;
	default:
		printf("divoom_port_uart unsupported baud=%d, fallback to 115200\n",
		       baudrate);
		return UART_BAUDRATE_115200;
	}
}

int divoom_port_uart_open(int port, int baud)
{
	_uart_config_t uart_config;
	int hal_port;

	if (s_open)
		return 0;

	hal_port = divoom_port_uart_to_hal_port(port);
	if (hal_port < 0)
		return -1;

	memset(&uart_config, 0, sizeof(uart_config));
	uart_config.baudrate    = divoom_port_uart_baud_to_hal(baud);
	uart_config.word_length = UART_WORD_LENGTH_8;
	uart_config.stop_bit    = UART_STOP_BIT_1;
	uart_config.parity      = UART_PARITY_NONE;

	if (hal_uart_init(hal_port) != 0)
		return -1;
	if (hal_uart_control(hal_port, 0, &uart_config) != 0) {
		hal_uart_deinit(hal_port);
		return -1;
	}

	if (hal_uart_rx_ring_enable(hal_port, DIVOOM_PORT_UART_RX_RING_SIZE) != 0) {
		printf("divoom_port_uart rx ring enable fail, fallback to mailbox\n");
		s_ring_on = 0;
	} else {
		s_ring_on = 1;
	}

	s_hal_port = hal_port;
	s_open = 1;
	return 0;
}

void divoom_port_uart_close(int port)
{
	(void)port;
	if (!s_open)
		return;
	if (s_ring_on) {
		hal_uart_rx_ring_disable(s_hal_port);
		s_ring_on = 0;
	}
	hal_uart_deinit(s_hal_port);
	s_hal_port = -1;
	s_open = 0;
}

int divoom_port_uart_write(int port, const uint8_t *buf, size_t len)
{
	int32_t n;

	(void)port;
	if (!s_open || !buf || len == 0)
		return -1;
	n = hal_uart_send(s_hal_port, buf, (uint32_t)len);
	if (n < 0)
		return -1;
	return (int)n;
}

int divoom_port_uart_read(int port, uint8_t *buf, size_t len, int timeout_ms)
{
	int32_t n;
	uint32_t want;

	(void)port;
	if (!s_open || !buf || len == 0)
		return -1;
	if (timeout_ms < 0)
		timeout_ms = 0;

	if (s_ring_on) {
		n = hal_uart_rx_ring_read(s_hal_port, buf, (uint32_t)len, timeout_ms);
	} else {
		want = (timeout_ms > 0) ? 1u : (uint32_t)len;
		n = hal_uart_receive_no_block(s_hal_port, buf, want, timeout_ms);
	}
	if (n < 0)
		return -1;
	return (int)n;
}

int divoom_port_uart_rx_info(int port, divoom_port_uart_rx_info_t *out)
{
	uint32_t hw = 0;
	uint32_t drop = 0;

	(void)port;
	if (!out)
		return -1;
	memset(out, 0, sizeof(*out));
	if (!s_open)
		return -1;

	out->buffered = s_ring_on;
	if (!s_ring_on)
		return 0;

	out->cap = DIVOOM_PORT_UART_RX_RING_SIZE;
	out->pending = (uint32_t)hal_uart_rx_ring_datasize(s_hal_port);
	if (hal_uart_rx_ring_stats(s_hal_port, &hw, &drop) == 0) {
		out->high_water = hw;
		out->drop = drop;
	}
	return 0;
}

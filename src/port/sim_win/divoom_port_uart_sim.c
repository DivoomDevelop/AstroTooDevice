#include "divoom_build_config.h"
#if !DIVOOM_BUILD_SIMULATOR
#error "divoom_port_uart_sim.c is simulator-only; exclude from RTOS Kbuild."
#endif

#include "divoom_port_uart.h"

#include <string.h>

/*
 * 模拟器替换的是整份 btcore_uart_*（JL 回包），不会走这条字节流。
 * 空实现只保证链接完整，open 失败以免被误当成真串口。
 */
int divoom_port_uart_open(int port, int baud)
{
	(void)port;
	(void)baud;
	return -1;
}

void divoom_port_uart_close(int port)
{
	(void)port;
}

int divoom_port_uart_write(int port, const uint8_t *buf, size_t len)
{
	(void)port;
	(void)buf;
	(void)len;
	return -1;
}

int divoom_port_uart_read(int port, uint8_t *buf, size_t len, int timeout_ms)
{
	(void)port;
	(void)buf;
	(void)len;
	(void)timeout_ms;
	return 0;
}

int divoom_port_uart_rx_info(int port, divoom_port_uart_rx_info_t *out)
{
	(void)port;
	if (!out)
		return -1;
	memset(out, 0, sizeof(*out));
	return -1;
}

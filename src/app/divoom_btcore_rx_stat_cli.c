/*
 * BtCore RX 常驻诊断：串口环 / 队列 / 帧池计数。
 * 不挂在 CONFIG_DIVOOM_FINSH_AUTOTEST 或 DIVOOM_BTCORE_UART_CLI 下，量产固件也可查。
 */
#include "divoom_build_config.h"
#if DIVOOM_BUILD_RTOS_DEVICE

#include "btcore_uart.h"
#include "divoom_port_shell.h"

static int btcore_uart_rx_stat_cli(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	btcore_uart_rx_stats_dump();
	return 0;
}

DIVOOM_PORT_SHELL_CMD(btcore_uart_rx_stat_cli, btcore_rx_stat,
		      BtCore UART RX ring / queue / frame pool counters);

#endif /* DIVOOM_BUILD_RTOS_DEVICE */

/*
 * BtCore 串口通信（与 doc/BtCore串口通信协议 一致）
 *
 * 指令格式：包头 + 包长度 + 命令类型 + 命令数据(可选) + 校验和 + 包尾
 *   [0]     包头 1 字节：0xAA
 *   [1][2]  包长度 2 字节，小端：长度 = 命令数据字节数 N + 校验和 2 字节 = N + 2
 *   [3]     命令类型 1 字节
 *   [4 .. 4+N-1] 命令数据 N 字节（无数据时 N=0）
 *   [4+N][5+N] 校验和 2 字节，小端 uint16：对「包长度两字节 + 命令类型 + 命令数据」
 *              各字节无符号相加后取低 16 位（累加和）
 *   [6+N]   包尾 1 字节：0xBB
 * 整帧长度 = 1 + 2 + 1 + N + 2 + 1 = 7 + N = 5 + (N+2) = 5 + L，其中 L = N+2。
 */
#ifndef BTCORE_UART_H
#define BTCORE_UART_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BTCORE_UART_DEVICE_DEFAULT
#define BTCORE_UART_DEVICE_DEFAULT  "/dev/ttyS1"
#endif

#ifndef BTCORE_UART_BAUD_DEFAULT
/* 与 BtCore 对端约定的链路波特率。
 *
 * T113 上 bus-uart1 直接挂 apb1（默认 24MHz），sunxi-uart 在该 uartclk 下：
 *   - 1.5Mbps 分频 = 1（24M/16/1），精度 100%，是唯一精确的高速档；
 *   - 1Mbps / 2Mbps / 921600 等都对不上整数分频，会被驱动 baud_set 拒绝
 *     或产生 > 10% 误差。
 * 想再往上推必须把 apb1 切到 ≥ 32MHz（动 ccu 默认配置），副作用波及所有
 * apb1 外设 (其它 UART/I2C/I2S BUS)，权衡后维持 1.5M。 */
#define BTCORE_UART_BAUD_DEFAULT     1500000
#endif

/* 命令类型枚举见 app/include/divoom_btcore_uart_cmd.h（DM_BTCORE_*） */

/* 命令数据区最大长度。文件块 0x18 载荷 = 6 + block_len(+2 CRC)，需覆盖 JL 侧 1792 字节块。 */
#define BTCORE_MAX_CMD_DATA_LEN  4096

typedef void (*btcore_uart_on_cmd_fn)(uint8_t cmd, const uint8_t *cmd_data, uint16_t cmd_len);

typedef struct btcore_uart_config {
	const char *device;   /* NULL 则 BTCORE_UART_DEVICE_DEFAULT */
	int         baudrate; /* 如 115200；0 则 BTCORE_UART_BAUD_DEFAULT */
} btcore_uart_config_t;

void btcore_uart_set_rx_callback(btcore_uart_on_cmd_fn cb);

int btcore_uart_init(const btcore_uart_config_t *cfg);

void btcore_uart_deinit(void);

int btcore_uart_is_ready(void);

/* cmd_len 为命令数据 N 字节（不含命令字节与校验） */
int btcore_uart_send(uint8_t cmd, const uint8_t *cmd_data, uint16_t cmd_len);

int btcore_uart_parse_frame(const uint8_t *frame, int frame_len,
			    uint8_t *out_cmd, const uint8_t **out_cmd_data, uint16_t *out_cmd_len);

/* 计算协议规定的 16 位累加和（与组包/校验共用） */
uint16_t btcore_uart_checksum16(const uint8_t len_le[2], uint8_t cmd,
				const uint8_t *cmd_data, uint16_t cmd_len);

/* 小端 32 位：实现见 btcore_uart.c */
void     btcore_put_le32(uint8_t *p, int32_t v);
int32_t  btcore_get_le32(const uint8_t *p);

/*
 * RX 帧数据池（实现见 btcore_uart_rtos.c）。所有权约定：投递方 alloc，消费方 free。
 * 让 RX 侧各消费者共用同一套静态块，队列里只搬指针，不搬定长大结构。
 */
uint8_t *btcore_uart_frame_alloc(uint16_t len);
void     btcore_uart_frame_free(uint8_t *data);
void     btcore_uart_frame_pool_dump(void);
/* RX 诊断快照：字节环水位 / 丢弃、两条命令队列深度、帧池余量。只读，可随时调 */
void     btcore_uart_rx_stats_dump(void);

#ifdef __cplusplus
}
#endif

#endif /* BTCORE_UART_H */

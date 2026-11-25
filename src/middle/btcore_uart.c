#include "btcore_uart.h"
#include "divoom.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#define BTCORE_HEADER     0xAA
#define BTCORE_TAIL       0xBB

/* 1.5Mbps 下单帧最大约 4KB+7；16KB 可缓冲多帧并避免 poll 周期内溢出。 */
#define BTCORE_RX_BUF     16384

/* 包长度域 L = N + 2，N 为命令数据长度；L 最大 = BTCORE_MAX_CMD_DATA_LEN + 2 */
#define BTCORE_MAX_L_BODY  (BTCORE_MAX_CMD_DATA_LEN + 2)

static int btcore_uart_set_attribs(int fd, int baudrate)
{
	struct termios t;

	if (tcgetattr(fd, &t) != 0)
		return -1;
	memset(&t, 0, sizeof(t));
	t.c_cflag |= CLOCAL | CREAD;
	t.c_cflag &= ~CSIZE;
	t.c_cflag |= CS8;
	t.c_cflag &= ~PARENB;
	t.c_cflag &= ~CSTOPB;
	t.c_cflag &= ~CRTSCTS;
	t.c_cc[VMIN] = 1;
	t.c_cc[VTIME] = 0;

	speed_t spd = B115200;
	switch (baudrate)
	{
		case 9600:    spd = B9600;    break;
		case 19200:   spd = B19200;   break;
		case 38400:   spd = B38400;   break;
		case 57600:   spd = B57600;   break;
		case 115200:  spd = B115200;  break;
		case 230400:  spd = B230400;  break;
		case 460800:  spd = B460800;  break;
		case 921600:  spd = B921600;  break;
		/* 1.5Mbps 是 T113 默认 24MHz apb1 下唯一精确（0 误差）的高速档：
		 *   24M / 16 / 1 = 1.5M, 分频整数。
		 * 1M / 2M / 2.5M / 3M 需要先把 apb1 拉到 ≥ 32MHz，会改动 ccu 默认
		 * 配置并波及所有 apb1 外设，故暂不开放；需要时再单独评估。 */
		case 1500000: spd = B1500000; break;
		default:
			divoom_print("btcore_uart unsupported baud=%d, fallback to 115200\n",
				     baudrate);
			break;
	}
	cfsetispeed(&t, spd);
	cfsetospeed(&t, spd);
	tcflush(fd, TCIOFLUSH);
	return tcsetattr(fd, TCSANOW, &t);
}

static int              s_fd = -1;
static pthread_t        s_rx_thread;
static bool             s_thread_running;
static bool             s_inited;
static pthread_mutex_t  s_tx_mutex = PTHREAD_MUTEX_INITIALIZER;

static btcore_uart_on_cmd_fn s_rx_cb;
static void                 *s_rx_user;

void btcore_uart_set_rx_callback(btcore_uart_on_cmd_fn cb)
{
	s_rx_cb = cb;
}

uint16_t btcore_uart_checksum16(const uint8_t len_le[2], uint8_t cmd,
				const uint8_t *cmd_data, uint16_t cmd_len)
{
	uint32_t s = (uint32_t)len_le[0] + (uint32_t)len_le[1] + (uint32_t)cmd;

	for (uint16_t i = 0; i < cmd_len; i++)
		s += cmd_data[i];
	return (uint16_t)(s & 0xFFFFu);
}

void btcore_put_le32(uint8_t *p, int32_t v)
{
	uint32_t u;

	u = (uint32_t)v;
	p[0] = (uint8_t)(u & 0xFFu);
	p[1] = (uint8_t)((u >> 8) & 0xFFu);
	p[2] = (uint8_t)((u >> 16) & 0xFFu);
	p[3] = (uint8_t)((u >> 24) & 0xFFu);
}

int32_t btcore_get_le32(const uint8_t *p)
{
	uint32_t u;

	u = (uint32_t)p[0];
	u |= (uint32_t)p[1] << 8;
	u |= (uint32_t)p[2] << 16;
	u |= (uint32_t)p[3] << 24;
	return (int32_t)u;
}

int btcore_uart_parse_frame(const uint8_t *frame, int frame_len,
			    uint8_t *out_cmd, const uint8_t **out_cmd_data, uint16_t *out_cmd_len)
{
	uint16_t L;
	uint16_t N;
	uint16_t expect;
	uint16_t chk_wire;

	if (!frame || frame_len < 7 || !out_cmd || !out_cmd_data || !out_cmd_len)
		return -EINVAL;
	if (frame[0] != BTCORE_HEADER)
		return -EINVAL;

	L = (uint16_t)frame[1] | ((uint16_t)frame[2] << 8);
	if (L < 2 || L > BTCORE_MAX_L_BODY)
		return -EINVAL;
	N = (uint16_t)(L - 2u);
	expect = (uint16_t)(5 + L);
	if (frame_len != expect)
		return -EINVAL;
	if (frame[4 + N + 2] != BTCORE_TAIL)
		return -EINVAL;

	*out_cmd = frame[3];
	*out_cmd_data = &frame[4];
	*out_cmd_len = N;

	chk_wire = (uint16_t)frame[4 + N] | ((uint16_t)frame[5 + N] << 8);
	if (chk_wire != btcore_uart_checksum16(&frame[1], frame[3], &frame[4], N))
		return -EINVAL;
	return 0;
}

#define BTCORE_FRAME_LOG_HEX_MAX  128//48

/* 文件块 0x18 延后处理：先尽量把后续 UART 帧读入/解析，再统一回调，避免大包回调阻塞导致丢包。 */
#define BTCORE_DEFER_FILE_BLOCK_CMD  0x18u
#define BTCORE_DEFER_BLOCK_MAX       32

typedef struct {
	uint8_t *data;
	uint16_t len;
} btcore_deferred_block_t;

static btcore_deferred_block_t s_deferred_blocks[BTCORE_DEFER_BLOCK_MAX];
static int s_deferred_block_count;

static void btcore_flush_deferred_file_blocks(void)
{
	int i;

	for (i = 0; i < s_deferred_block_count; i++)
	{
		if (s_rx_cb && s_deferred_blocks[i].data)
			s_rx_cb(BTCORE_DEFER_FILE_BLOCK_CMD, s_deferred_blocks[i].data,
				s_deferred_blocks[i].len);
		divoom_system_free(s_deferred_blocks[i].data);
		s_deferred_blocks[i].data = NULL;
		s_deferred_blocks[i].len = 0;
	}
	s_deferred_block_count = 0;
}

static int btcore_defer_file_block(const uint8_t *cmd_data, uint16_t cmd_len)
{
	uint8_t *copy;

	if (cmd_len == 0u || s_deferred_block_count >= BTCORE_DEFER_BLOCK_MAX)
		return -ENOMEM;

	copy = (uint8_t *)divoom_system_malloc(cmd_len);
	if (!copy)
		return -ENOMEM;

	memcpy(copy, cmd_data, cmd_len);
	s_deferred_blocks[s_deferred_block_count].data = copy;
	s_deferred_blocks[s_deferred_block_count].len = cmd_len;
	s_deferred_block_count++;
	return 0;
}

static void btcore_uart_rx_pull_kernel(uint8_t *buf, size_t *used)
{
	int avail = 0;
	ssize_t n;

	while (ioctl(s_fd, FIONREAD, &avail) == 0 && avail > 0)
	{
		if (*used >= BTCORE_RX_BUF)
			break;

		n = read(s_fd, buf + *used, BTCORE_RX_BUF - *used);
		if (n < 0)
		{
			if (errno == EINTR)
				continue;
			break;
		}
		if (n == 0)
			break;
		*used += (size_t)n;
	}
}

static void btcore_uart_log_frame(const char *dir, const uint8_t *frame, int frame_len,
				  uint8_t cmd, uint16_t cmd_len)
{
#if 0	
	char line[320];
	int pos;
	int dump_n;
	int i;

	pos = snprintf(line, sizeof(line),
		       "btcore_uart %s cmd=0x%02x data_len=%u frame_len=%d:",
		       dir ? dir : "rx", cmd, (unsigned)cmd_len, frame_len);
	dump_n = frame_len;
	if (dump_n > BTCORE_FRAME_LOG_HEX_MAX)
		dump_n = BTCORE_FRAME_LOG_HEX_MAX;
	for (i = 0; i < dump_n && pos < (int)sizeof(line) - 4; i++)
		pos += snprintf(line + pos, (size_t)sizeof(line) - (size_t)pos,
				" %02x", frame[i]);
	if (frame_len > dump_n && pos < (int)sizeof(line) - 12)
		pos += snprintf(line + pos, (size_t)sizeof(line) - (size_t)pos,
				" ...+%d", frame_len - dump_n);
	divoom_print("%s\n", line);
#endif
}

/* 0=解析并回调成功，<0=失败（不消费缓冲，由上层 resync） */
static int btcore_dispatch_frame(const uint8_t *frame, int frame_len);
static void btcore_process_rx_buf(uint8_t *buf, size_t *used);

static int btcore_dispatch_frame(const uint8_t *frame, int frame_len)
{
	uint8_t cmd;
	const uint8_t *cmd_data;
	uint16_t cmd_len;
	uint8_t *cmd_copy = NULL;
	int r;

	r = btcore_uart_parse_frame(frame, frame_len, &cmd, &cmd_data, &cmd_len);
	if (r != 0)
		return r;

#ifdef DIVOOM_TIMES_VIBE
	if (divoom_btcore_uart_cmd_is_rx_suppressed())
		return 0;
#endif

	if (cmd == BTCORE_DEFER_FILE_BLOCK_CMD && cmd_len > 256u)
		return btcore_defer_file_block(cmd_data, cmd_len);

	if (cmd != DM_BTCORE_BT_BIP_DATA /*&& cmd != DM_BTCORE_BAT_VOLT_LEVE*/ && cmd != DM_BTCORE_BAT_VOLT_AVR && cmd != DM_BTCORE_GET_FFT_FREQ_DATA)
		btcore_uart_log_frame("rx", frame, frame_len, cmd, cmd_len);

	/* 0x3E 等命令必须在文件块入库后再处理 */
	btcore_flush_deferred_file_blocks();

	if (s_rx_cb)
	{
		if (cmd_len > 0u)
		{
			cmd_copy = (uint8_t *)divoom_system_malloc(cmd_len);
			if (!cmd_copy)
				return -ENOMEM;
			memcpy(cmd_copy, cmd_data, cmd_len);
			s_rx_cb(cmd, cmd_copy, cmd_len);
			divoom_system_free(cmd_copy);
		}
		else
		{
			s_rx_cb(cmd, cmd_data, cmd_len);
		}
	}
	return 0;
}

static void btcore_uart_rx_drain_and_process(uint8_t *buf, size_t *used)
{
	int avail = 0;
	ssize_t n;

	for (;;)
	{
		btcore_process_rx_buf(buf, used);

		if (ioctl(s_fd, FIONREAD, &avail) != 0 || avail <= 0)
			break;

		if (*used >= BTCORE_RX_BUF)
		{
			divoom_print("btcore_uart rx buf full used=%u avail=%d\n",
				     (unsigned)*used, avail);
			btcore_process_rx_buf(buf, used);
			if (*used >= BTCORE_RX_BUF)
			{
				divoom_print("btcore_uart rx overflow, drop %u bytes\n",
					     (unsigned)*used);
				*used = 0;
				break;
			}
		}

		n = read(s_fd, buf + *used, BTCORE_RX_BUF - *used);
		if (n < 0)
		{
			if (errno == EINTR)
				continue;
			divoom_print("btcore_uart drain read err errno=%d\n", errno);
			break;
		}
		if (n == 0)
			break;
		*used += (size_t)n;
	}
}

static void btcore_process_rx_buf(uint8_t *buf, size_t *used)
{
	while (*used >= 4)
	{
		size_t j;
		uint16_t L;
		size_t need;

		for (j = 0; j < *used; j++)
		{
			if (buf[j] == BTCORE_HEADER)
				break;
		}
		if (j > 0)
		{
			memmove(buf, buf + j, *used - j);
			*used -= j;
		}
		if (*used < 4)
			return;

		L = (uint16_t)buf[1] | ((uint16_t)buf[2] << 8);
		if (L < 2 || L > BTCORE_MAX_L_BODY)
		{
			if (L > BTCORE_MAX_L_BODY && buf[3] == 0x18)
			{
				divoom_print("btcore_uart drop 0x18 frame L=%u > max=%u\n",
					     (unsigned)L, (unsigned)BTCORE_MAX_L_BODY);
			}
			memmove(buf, buf + 1, *used - 1);
			(*used)--;
			continue;
		}
		need = (size_t)5 + (size_t)L;
		if (*used < need)
			return;

		/* 尾字节不对：多半未收齐或 0xAA 假同步，丢 1 字节重找头 */
		if (buf[need - 1] != BTCORE_TAIL)
		{
			memmove(buf, buf + 1, *used - 1);
			(*used)--;
			continue;
		}

		if (btcore_dispatch_frame(buf, (int)need) != 0)
		{
			/* 长度/校验失败：不要按 need 整段丢弃，否则会把未收齐的真帧吃掉 */
			memmove(buf, buf + 1, *used - 1);
			(*used)--;
			continue;
		}

		memmove(buf, buf + need, *used - need);
		*used -= need;
		btcore_uart_rx_pull_kernel(buf, used);
	}

	btcore_flush_deferred_file_blocks();
}

static void *btcore_rx_thread_main(void *arg)
{
	uint8_t buf[BTCORE_RX_BUF];
	size_t used = 0;
	(void)arg;

	divoom_print("btcore_uart rx thread enter, fd=%d\n", s_fd);

	while (s_thread_running)
	{
		struct pollfd pfd;
		int pr;
		ssize_t n;

		if (s_fd < 0)
		{
			divoom_print("btcore_uart fd<0, exit thread\n");
			break;
		}

		pfd.fd = s_fd;
		pfd.events = POLLIN;
		pfd.revents = 0;
		pr = poll(&pfd, 1, 1000);
		if (pr < 0)
		{
			if (errno == EINTR)
				continue;
			divoom_print("btcore_uart poll err errno=%d\n", errno);
			break;
		}
		if (pr == 0)
			continue;
		if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL))
		{
			divoom_print("btcore_uart poll revents=0x%x, exit\n",
				     (unsigned)pfd.revents);
			break;
		}
		if (!(pfd.revents & POLLIN))
			continue;

		n = read(s_fd, buf + used, sizeof(buf) - used);
		if (n < 0)
		{
			if (errno == EINTR)
				continue;
			divoom_print("btcore_uart read err errno=%d\n", errno);
			break;
		}
		if (n == 0)
			continue;
		used += (size_t)n;
		btcore_uart_rx_drain_and_process(buf, &used);
	}
	divoom_print("btcore_uart rx thread exit\n");
	return NULL;
}

int btcore_uart_is_ready(void)
{
	return s_inited && s_fd >= 0 && s_thread_running;
}

int btcore_uart_init(const btcore_uart_config_t *cfg)
{
	const char *dev = BTCORE_UART_DEVICE_DEFAULT;
	int baud = BTCORE_UART_BAUD_DEFAULT;
	int fd;

	if (s_inited)
		return 0;
	if (cfg)
	{
		if (cfg->device && cfg->device[0])
			dev = cfg->device;
		if (cfg->baudrate > 0)
			baud = cfg->baudrate;
	}

	fd = open(dev, O_RDWR | O_NOCTTY);
	if (fd < 0)
	{
		divoom_print("btcore_uart open %s fail errno=%d\n", dev, errno);
		return -1;
	}
	if (btcore_uart_set_attribs(fd, baud) != 0)
	{
		divoom_print("btcore_uart set_attribs fail errno=%d\n", errno);
		close(fd);
		return -1;
	}

	s_fd = fd;
	s_thread_running = true;
	if (pthread_create(&s_rx_thread, NULL, btcore_rx_thread_main, NULL) != 0)
	{
		divoom_print("btcore_uart pthread_create fail errno=%d\n", errno);
		close(s_fd);
		s_fd = -1;
		s_thread_running = false;
		return -1;
	}
	s_inited = true;
	divoom_system_delay_ms(10);
	divoom_print("btcore_uart init ok dev=%s baud=%d fd=%d\n",
		     dev, baud, s_fd);
	return 0;
}

void btcore_uart_deinit(void)
{
	int fd_join;

	if (!s_inited)
		return;
	s_thread_running = false;
	fd_join = s_fd;
	if (fd_join >= 0)
	{
		s_fd = -1;
		close(fd_join);
	}
	pthread_join(s_rx_thread, NULL);
	s_inited = false;
	btcore_flush_deferred_file_blocks();
}

int btcore_uart_send(uint8_t cmd, const uint8_t *cmd_data, uint16_t cmd_len)
{
	uint8_t frame[7 + BTCORE_MAX_CMD_DATA_LEN];
	uint16_t L;
	uint16_t chk;
	size_t total;

	if (!s_inited || s_fd < 0)
		return -EINVAL;
	if (cmd_len > BTCORE_MAX_CMD_DATA_LEN)
		return -EINVAL;

	L = (uint16_t)(cmd_len + 2u);
	frame[0] = BTCORE_HEADER;
	frame[1] = (uint8_t)(L & 0xFF);
	frame[2] = (uint8_t)((L >> 8) & 0xFF);
	frame[3] = cmd;
	if (cmd_len && cmd_data)
		memcpy(frame + 4, cmd_data, cmd_len);

	chk = btcore_uart_checksum16(&frame[1], cmd, cmd_len ? cmd_data : NULL, cmd_len);
	frame[4 + cmd_len] = (uint8_t)(chk & 0xFF);
	frame[5 + cmd_len] = (uint8_t)((chk >> 8) & 0xFF);
	frame[6 + cmd_len] = BTCORE_TAIL;

	total = (size_t)7 + (size_t)cmd_len;

	btcore_uart_log_frame("tx", frame, (int)total, cmd, cmd_len);

	pthread_mutex_lock(&s_tx_mutex);
	(void)write(s_fd, frame, total);
	pthread_mutex_unlock(&s_tx_mutex);
	return 0;
}

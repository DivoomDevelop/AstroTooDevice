#include "divoom.h"

#ifdef DIVOOM_TIMES_VIBE

#include "divoom_btcore_file_rx.h"
#include "btcore_uart.h"

#include <string.h>

#define DIVOOM_BTCORE_FILE_RX_MAX_FILE_SIZE  (4u * 1024u * 1024u)
#define DIVOOM_BTCORE_FILE_RX_MAX_PACKETS    65536u
#define DIVOOM_BTCORE_SONGLIST_DEFAULT_PATH  DIVOOM_SD_SYSTEM_DIR"/tf_songlist.bin"
/* 逐包日志会抢全局日志锁；抽样保留进度 */
#define DIVOOM_BTCORE_FILE_RX_LOG_EVERY      256u
#define DIVOOM_BTCORE_FILE_RX_GAP_LOG_EVERY  32u
#define DIVOOM_BTCORE_FILE_RX_CRC_CHUNK      4096u
/* 与杰理 MISSED_BLOCK_ID_MAX_SIZE 一致，超出放到下一轮 0x3D */
#define DIVOOM_BTCORE_FILE_RX_MISS_CAP       64u
#define DIVOOM_BTCORE_FILE_RX_STALL_MS       1500u
#define DIVOOM_BTCORE_FILE_RX_MISS_RETRY_MS  200u

typedef struct _divoom_btcore_file_rx_ctx {
	char     save_path[DIVOOM_BTCORE_FILE_RX_MAX_PATH];
	uint16_t expect_crc16;
	uint32_t file_size;
	uint16_t block_len;
	uint32_t expected_packets;
	uint16_t wire_crc16;
	int      receiving;
	int      tail_received;
	uint32_t recv_count;
	uint8_t *recv_bits;
	void    *fp;
	uint32_t last_rx_ms;
	uint32_t last_miss_req_ms;
	uint32_t gap_log_count;
	divoom_btcore_file_rx_saved_fn saved_cb;
	divoom_btcore_file_rx_saved_fn fw_saved_cb;
	divoom_btcore_file_rx_progress_fn fw_progress_cb;
} divoom_btcore_file_rx_ctx, *Pdivoom_btcore_file_rx_ctx;

static divoom_btcore_file_rx_ctx s_file_rx;
static uint32_t s_file_rx_last_packet_id = 0xFFFFFFFFu;

static uint16_t divoom_btcore_file_rx_get_le16(const uint8_t *p)
{
	return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t divoom_btcore_file_rx_get_le32(const uint8_t *p)
{
	return (uint32_t)p[0]
	       | ((uint32_t)p[1] << 8)
	       | ((uint32_t)p[2] << 16)
	       | ((uint32_t)p[3] << 24);
}

static void divoom_btcore_file_rx_put_le16(uint8_t *p, uint16_t v)
{
	p[0] = (uint8_t)(v & 0xFFu);
	p[1] = (uint8_t)((v >> 8) & 0xFFu);
}

static void divoom_btcore_file_rx_put_le32(uint8_t *p, uint32_t v)
{
	p[0] = (uint8_t)(v & 0xFFu);
	p[1] = (uint8_t)((v >> 8) & 0xFFu);
	p[2] = (uint8_t)((v >> 16) & 0xFFu);
	p[3] = (uint8_t)((v >> 24) & 0xFFu);
}

/* 与杰里 divoom_com_crc16 / divoom_UartCrcCCITTTable 一致（CRC-16/CCITT 查表，初值 0） */
static const uint16_t divoom_uart_crc_ccitt_table[256] = {
	0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
	0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
	0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
	0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
	0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
	0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
	0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
	0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
	0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
	0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
	0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
	0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
	0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
	0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
	0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
	0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
	0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
	0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
	0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
	0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
	0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
	0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
	0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
	0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
	0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
	0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
	0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
	0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
	0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
	0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
	0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
	0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};

static uint16_t divoom_btcore_file_rx_crc16_ccitt(const uint8_t *buf, uint32_t len, uint16_t crc)
{
	uint32_t i;

	if (!buf || len == 0u)
		return crc;

	for (i = 0; i < len; i++)
		crc = (uint16_t)((crc << 8) ^
				 divoom_uart_crc_ccitt_table[((crc >> 8) ^ buf[i]) & 0xFFu]);
	return crc;
}

uint16_t divoom_btcore_file_rx_crc16_data(const uint8_t *data, uint32_t len)
{
	return divoom_btcore_file_rx_crc16_ccitt(data, len, 0u);
}

uint16_t divoom_btcore_file_rx_crc16_chain(const uint8_t *data, uint32_t len, uint16_t crc)
{
	return divoom_btcore_file_rx_crc16_ccitt(data, len, crc);
}

uint16_t divoom_btcore_file_rx_crc16_file(const char *path)
{
	void *fp;
	uint8_t buf[1024];
	size_t n;
	uint16_t crc = 0u;

	if (!path || !path[0])
		return 0u;

	fp = divoom_port_file_open(path, "rb");
	if (!fp)
		return 0u;

	while ((n = divoom_port_file_read(buf, 1, sizeof(buf), fp)) > 0u)
		crc = divoom_btcore_file_rx_crc16_ccitt(buf, (uint32_t)n, crc);

	divoom_port_file_close(fp);
	return crc;
}

static int divoom_btcore_file_rx_is_songlist_path(const char *path)
{
	return (path && path[0] &&
		strcmp(path, DIVOOM_BTCORE_SONGLIST_DEFAULT_PATH) == 0);
}

static int divoom_btcore_file_rx_is_fw_path(const char *path)
{
	return (path && path[0] &&
		strcmp(path, DIVOOM_BTCORE_JL_FW_UPDATE_PATH) == 0);
}

static void divoom_btcore_file_rx_report_fw_percent(void)
{
	uint32_t done;
	uint8_t percent;

	if (!divoom_btcore_file_rx_is_fw_path(s_file_rx.save_path) ||
	    s_file_rx.file_size == 0u)
		return;

	done = s_file_rx.recv_count * (uint32_t)s_file_rx.block_len;
	if (done > s_file_rx.file_size)
		done = s_file_rx.file_size;
	percent = (uint8_t)(done * 100u / s_file_rx.file_size);
	if (s_file_rx.fw_progress_cb)
		s_file_rx.fw_progress_cb(percent);
}

static int divoom_btcore_file_rx_prefix_ok(const char *path, const char *root)
{
	size_t n;

	if (!path || !root)
		return 0;
	n = strlen(root);
	if (n == 0u)
		return 0;
	if (strcmp(path, root) == 0)
		return 1;
	if (strncmp(path, root, n) == 0 && path[n] == '/')
		return 1;
	return 0;
}

/* JL TF 升级 / 搬文件：dest 只允许 /usr 与 /userdata，禁止 .. */
static int divoom_btcore_file_rx_path_allowed(const char *path)
{
	if (!path || path[0] != '/' || strstr(path, "..") != NULL)
		return 0;
	if (divoom_btcore_file_rx_prefix_ok(path, "/usr"))
		return 1;
	if (divoom_btcore_file_rx_prefix_ok(path, DIVOOM_USER_ROOT))
		return 1;
	return 0;
}

static int divoom_btcore_file_rx_reply_head(uint8_t status)
{
	return divoom_btcore_uart_cmd_send(DM_BTCORE_FILE_HEAD_TRANSMIT, &status, 1);
}

static int divoom_btcore_file_rx_reply_block(uint8_t result)
{
	return divoom_btcore_uart_cmd_send(DM_BTCORE_FILE_BLOCK_TRANSMIT, &result, 1);
}

static int divoom_btcore_file_rx_send_missed_packets(const uint32_t *ids, uint16_t count)
{
	uint8_t *buf;
	uint16_t i;
	uint16_t pos;
	int ret;

	if (!ids || count == 0u)
		return -1;

	buf = (uint8_t *)divoom_system_malloc((size_t)2 + (size_t)count * 4u);
	if (!buf)
		return -1;

	divoom_btcore_file_rx_put_le16(buf, count);
	pos = 2;
	for (i = 0; i < count; i++)
	{
		divoom_btcore_file_rx_put_le32(buf + pos, ids[i]);
		pos = (uint16_t)(pos + 4u);
	}
	ret = divoom_btcore_uart_cmd_send(DM_BTCORE_FILE_MISSED_PACKETS, buf, pos);
	divoom_system_free(buf);
	return ret;
}

static int divoom_btcore_file_rx_bit_test(uint32_t pkt)
{
	if (!s_file_rx.recv_bits || pkt >= s_file_rx.expected_packets)
		return 0;
	return (int)((s_file_rx.recv_bits[pkt >> 3] >> (pkt & 7u)) & 1u);
}

static int divoom_btcore_file_rx_bit_set(uint32_t pkt)
{
	uint8_t mask;

	if (!s_file_rx.recv_bits || pkt >= s_file_rx.expected_packets)
		return 0;
	mask = (uint8_t)(1u << (pkt & 7u));
	if (s_file_rx.recv_bits[pkt >> 3] & mask)
		return 0;
	s_file_rx.recv_bits[pkt >> 3] = (uint8_t)(s_file_rx.recv_bits[pkt >> 3] | mask);
	return 1;
}

static int divoom_btcore_file_rx_has_missing_packets(void)
{
	if (!s_file_rx.receiving)
		return 0;
	return (s_file_rx.recv_count < s_file_rx.expected_packets) ? 1 : 0;
}

static void divoom_btcore_file_rx_close_fp(void)
{
	if (s_file_rx.fp)
	{
		divoom_port_file_close(s_file_rx.fp);
		s_file_rx.fp = NULL;
	}
}

static void divoom_btcore_file_rx_free_bits(void)
{
	if (s_file_rx.recv_bits)
	{
		divoom_system_free(s_file_rx.recv_bits);
		s_file_rx.recv_bits = NULL;
	}
}

static void divoom_btcore_file_rx_ensure_dir(const char *path)
{
	char dir_path[DIVOOM_BTCORE_FILE_RX_MAX_PATH];
	const char *slash;
	size_t dir_len;

	if (!path || !path[0])
		return;

	strncpy(dir_path, path, sizeof(dir_path) - 1u);
	dir_path[sizeof(dir_path) - 1u] = '\0';
	slash = strrchr(dir_path, '/');
	if (!slash || slash == dir_path)
		return;
	dir_len = (size_t)(slash - dir_path);
	dir_path[dir_len] = '\0';
	divoom_app_com_check_and_mk_dir(dir_path);
}

static void divoom_btcore_file_rx_reset_receive_state(int remove_file)
{
	char path[DIVOOM_BTCORE_FILE_RX_MAX_PATH];

	path[0] = '\0';
	if (remove_file && s_file_rx.save_path[0])
		strncpy(path, s_file_rx.save_path, sizeof(path) - 1u);

	divoom_btcore_file_rx_close_fp();
	divoom_btcore_file_rx_free_bits();
	s_file_rx.receiving = 0;
	s_file_rx.tail_received = 0;
	s_file_rx.file_size = 0;
	s_file_rx.block_len = 0;
	s_file_rx.expected_packets = 0;
	s_file_rx.wire_crc16 = 0;
	s_file_rx.recv_count = 0;
	s_file_rx.last_rx_ms = 0;
	s_file_rx.last_miss_req_ms = 0;
	s_file_rx.gap_log_count = 0;
	s_file_rx_last_packet_id = 0xFFFFFFFFu;

	if (path[0])
		divoom_port_remove_file(path);
}

/* 扫描缺包，最多 64 个 id 发 0x3D。返回 1=已请求，0=无缺包/节流，-1=错误 */
static int divoom_btcore_file_rx_request_missed_packets(void)
{
	uint32_t expected_packets;
	uint32_t miss_ids[DIVOOM_BTCORE_FILE_RX_MISS_CAP];
	uint16_t miss_count = 0;
	uint32_t pkt;
	uint32_t now_ms;

	if (!s_file_rx.receiving || !s_file_rx.recv_bits)
		return 0;

	expected_packets = s_file_rx.expected_packets;
	if (expected_packets == 0u || s_file_rx.recv_count >= expected_packets)
		return 0;

	now_ms = divoom_system_get_tickms();
	if (s_file_rx.last_miss_req_ms != 0u &&
	    (now_ms - s_file_rx.last_miss_req_ms) < DIVOOM_BTCORE_FILE_RX_MISS_RETRY_MS)
		return 0;

	for (pkt = 0; pkt < expected_packets && miss_count < DIVOOM_BTCORE_FILE_RX_MISS_CAP; pkt++)
	{
		if (!divoom_btcore_file_rx_bit_test(pkt))
			miss_ids[miss_count++] = pkt;
	}

	if (miss_count == 0u)
		return 0;

	s_file_rx.last_miss_req_ms = now_ms;
	divoom_print("btcore file rx miss packets=%u first=%u recv=%u/%u\n",
		     (unsigned)miss_count, (unsigned)miss_ids[0],
		     (unsigned)s_file_rx.recv_count, (unsigned)expected_packets);

	if (divoom_btcore_file_rx_send_missed_packets(miss_ids, miss_count) == 0)
		return 1;
	return -1;
}

static int divoom_btcore_file_rx_store_block(uint32_t packet_id, const uint8_t *data, uint16_t data_len)
{
	uint32_t blk_off;

	if (!data || data_len == 0u || !s_file_rx.fp || !s_file_rx.recv_bits)
		return 0;
	if (packet_id >= s_file_rx.expected_packets || s_file_rx.block_len == 0u)
		return 0;

	blk_off = packet_id * (uint32_t)s_file_rx.block_len;
	if (blk_off >= s_file_rx.file_size)
		return 0;
	if (blk_off + (uint32_t)data_len > s_file_rx.file_size)
		data_len = (uint16_t)(s_file_rx.file_size - blk_off);

	if (divoom_port_file_seek(s_file_rx.fp, (long)blk_off, DIVOOM_PORT_SEEK_SET) != 0)
		return 0;
	if (divoom_port_file_write(data, 1, (size_t)data_len, s_file_rx.fp) != (size_t)data_len)
		return 0;

	if (divoom_btcore_file_rx_bit_set(packet_id))
		s_file_rx.recv_count++;
	return 1;
}

static uint16_t divoom_btcore_file_rx_crc16_open_file(void)
{
	uint8_t buf[DIVOOM_BTCORE_FILE_RX_CRC_CHUNK];
	size_t n;
	uint32_t remain;
	uint16_t crc = 0u;

	if (!s_file_rx.fp || s_file_rx.file_size == 0u)
		return 0u;

	if (divoom_port_file_seek(s_file_rx.fp, 0, DIVOOM_PORT_SEEK_SET) != 0)
		return 0u;

	remain = s_file_rx.file_size;
	while (remain > 0u)
	{
		size_t want = sizeof(buf);

		if ((uint32_t)want > remain)
			want = (size_t)remain;
		n = divoom_port_file_read(buf, 1, want, s_file_rx.fp);
		if (n == 0u)
			break;
		crc = divoom_btcore_file_rx_crc16_ccitt(buf, (uint32_t)n, crc);
		remain -= (uint32_t)n;
	}
	return crc;
}

static void divoom_btcore_file_rx_notify_saved(int ok)
{
	if (s_file_rx.saved_cb &&
	    divoom_btcore_file_rx_is_songlist_path(s_file_rx.save_path))
		s_file_rx.saved_cb(s_file_rx.save_path, ok);
	if (s_file_rx.fw_saved_cb &&
	    divoom_btcore_file_rx_is_fw_path(s_file_rx.save_path))
		s_file_rx.fw_saved_cb(s_file_rx.save_path, ok);
}

/* 无缺包且已有尾包 CRC：读盘校验，OK 则 0x18 回复 result=0。返回 1=已完成 */
static int divoom_btcore_file_rx_try_complete_and_reply(void)
{
	uint16_t calc_crc;
	uint8_t result = 1;
	int ok = 0;

	if (!s_file_rx.receiving || !s_file_rx.tail_received)
		return 0;
	if (divoom_btcore_file_rx_has_missing_packets())
		return 0;
	if (s_file_rx.file_size == 0u || s_file_rx.expected_packets == 0u)
		return 0;

	calc_crc = divoom_btcore_file_rx_crc16_open_file();
	if (calc_crc != s_file_rx.wire_crc16)
	{
		divoom_print("btcore file rx crc fail wire=0x%04x calc=0x%04x\n",
			     (unsigned)s_file_rx.wire_crc16, (unsigned)calc_crc);
		divoom_btcore_file_rx_reply_block(result);
		divoom_btcore_file_rx_notify_saved(0);
		divoom_btcore_file_rx_reset_receive_state(1);
		return 0;
	}

	divoom_btcore_file_rx_close_fp();
	ok = 1;
	result = 0;
	divoom_btcore_file_rx_reply_block(result);
	divoom_print("btcore file rx saved ok: %s size=%u crc=0x%04x\n",
		     s_file_rx.save_path, (unsigned)s_file_rx.file_size,
		     (unsigned)s_file_rx.wire_crc16);
	divoom_print("btcore file rx block reply result=%u\n", (unsigned)result);
	divoom_btcore_file_rx_notify_saved(1);
	divoom_btcore_file_rx_reset_receive_state(0);
	return ok ? 1 : 0;
}

static int divoom_btcore_file_rx_finish_transfer(void)
{
	int ok;

	if (!s_file_rx.receiving)
		return 0;

	if (!s_file_rx.tail_received)
	{
		divoom_print("btcore file rx end but tail crc missing recv=%u/%u\n",
			     (unsigned)s_file_rx.recv_count,
			     (unsigned)s_file_rx.expected_packets);
		if (divoom_btcore_file_rx_has_missing_packets())
			divoom_btcore_file_rx_request_missed_packets();
		return 0;
	}

	ok = divoom_btcore_file_rx_try_complete_and_reply();
	if (ok)
		return 1;

	if (s_file_rx.receiving)
	{
		divoom_print("btcore file rx end but transfer incomplete recv=%u/%u\n",
			     (unsigned)s_file_rx.recv_count,
			     (unsigned)s_file_rx.expected_packets);
		divoom_btcore_file_rx_request_missed_packets();
	}
	return 0;
}

int divoom_btcore_file_rx_set_save_path(const char *path, uint16_t expect_crc16)
{
	if (!path)
		return -1;

	strncpy(s_file_rx.save_path, path, sizeof(s_file_rx.save_path) - 1u);
	s_file_rx.save_path[sizeof(s_file_rx.save_path) - 1u] = '\0';
	s_file_rx.expect_crc16 = expect_crc16;
	return 0;
}

const char *divoom_btcore_file_rx_get_save_path(void)
{
	return s_file_rx.save_path;
}

uint16_t divoom_btcore_file_rx_get_expect_crc16(void)
{
	return s_file_rx.expect_crc16;
}

void divoom_btcore_file_rx_set_saved_callback(divoom_btcore_file_rx_saved_fn cb)
{
	s_file_rx.saved_cb = cb;
}

void divoom_btcore_file_rx_set_fw_saved_callback(divoom_btcore_file_rx_saved_fn cb)
{
	s_file_rx.fw_saved_cb = cb;
}

void divoom_btcore_file_rx_set_fw_progress_callback(divoom_btcore_file_rx_progress_fn cb)
{
	s_file_rx.fw_progress_cb = cb;
}

void divoom_btcore_file_rx_reset(void)
{
	divoom_btcore_file_rx_saved_fn saved_cb = s_file_rx.saved_cb;
	divoom_btcore_file_rx_saved_fn fw_saved_cb = s_file_rx.fw_saved_cb;
	divoom_btcore_file_rx_progress_fn fw_progress_cb = s_file_rx.fw_progress_cb;
	int remove_file = s_file_rx.receiving;

	divoom_btcore_file_rx_reset_receive_state(remove_file);
	memset(&s_file_rx, 0, sizeof(s_file_rx));
	s_file_rx.saved_cb = saved_cb;
	s_file_rx.fw_saved_cb = fw_saved_cb;
	s_file_rx.fw_progress_cb = fw_progress_cb;
}

void divoom_btcore_file_rx_poll(void)
{
	uint32_t now_ms;

	if (!s_file_rx.receiving || !s_file_rx.recv_bits)
		return;
	if (!divoom_btcore_file_rx_has_missing_packets())
	{
		if (s_file_rx.tail_received)
			divoom_btcore_file_rx_try_complete_and_reply();
		return;
	}

	now_ms = divoom_system_get_tickms();
	if (s_file_rx.last_rx_ms == 0u)
		return;
	if ((now_ms - s_file_rx.last_rx_ms) < DIVOOM_BTCORE_FILE_RX_STALL_MS)
		return;
	if (s_file_rx.last_miss_req_ms != 0u &&
	    (now_ms - s_file_rx.last_miss_req_ms) < DIVOOM_BTCORE_FILE_RX_STALL_MS)
		return;

	if (divoom_btcore_file_rx_request_missed_packets() == 1)
		divoom_print("btcore file rx stall %ums recv=%u/%u tail=%d\n",
			     (unsigned)(now_ms - s_file_rx.last_rx_ms),
			     (unsigned)s_file_rx.recv_count,
			     (unsigned)s_file_rx.expected_packets,
			     s_file_rx.tail_received);
}

static uint32_t divoom_btcore_file_rx_read_songlist_timestamp(const char *path)
{
	void *fp;
	uint8_t hdr[8];

	if (!path || !path[0])
		return 0u;

	fp = divoom_port_file_open(path, "rb");
	if (!fp)
		return 0u;

	if (divoom_port_file_read(hdr, 1, sizeof(hdr), fp) < 8u)
	{
		divoom_port_file_close(fp);
		return 0u;
	}
	divoom_port_file_close(fp);
	return divoom_btcore_file_rx_get_le32(hdr + 4);
}

void divoom_btcore_file_rx_on_songlist_reply(const uint8_t *cmd_data, uint16_t cmd_len)
{
	const char *path;
	uint16_t crc16;
	uint32_t timestamp;
	uint8_t path_len;
	uint8_t payload[2 + 4 + 1 + DIVOOM_BTCORE_FILE_RX_MAX_PATH];
	int pos;

	(void)cmd_data;

	/* JL→T113 DM_BTCORE_SONGLIST_INFO(0x42) 查询：无载荷；T113 回复 crc16 + timestamp + path */
	if (cmd_len != 0u)
	{
		divoom_print("btcore songlist query ignore unexpected len=%u\n",
			     (unsigned)cmd_len);
		return;
	}

	if (!divoom_btcore_uart_cmd_is_ready())
		return;

	path = s_file_rx.save_path;
	if (path[0] == '\0')
		path = DIVOOM_BTCORE_SONGLIST_DEFAULT_PATH;

	crc16 = divoom_btcore_file_rx_crc16_file(path);
	timestamp = divoom_btcore_file_rx_read_songlist_timestamp(path);

	path_len = (uint8_t)strlen(path);
	if (path_len == 0u || path_len > 250u)
		return;

	strncpy(s_file_rx.save_path, path, sizeof(s_file_rx.save_path) - 1u);
	s_file_rx.save_path[sizeof(s_file_rx.save_path) - 1u] = '\0';
	s_file_rx.expect_crc16 = crc16;

	divoom_btcore_file_rx_put_le16(payload, crc16);
	pos = 2;
	divoom_btcore_file_rx_put_le32(payload + pos, timestamp);
	pos += 4;
	payload[pos++] = path_len;
	memcpy(payload + pos, path, path_len);
	pos += (int)path_len;

	divoom_btcore_uart_cmd_send(DM_BTCORE_SONGLIST_INFO, payload, (uint16_t)pos);
	divoom_print("btcore songlist reply crc=0x%04x ts=%u path=%s\n",
		     (unsigned)crc16, (unsigned)timestamp, path);
	divoom_tf_music_on_songlist_info_replied();
}

void divoom_btcore_file_rx_on_head(const uint8_t *cmd_data, uint16_t cmd_len)
{
	uint32_t file_size;
	uint8_t path_len;
	uint16_t block_len;
	uint32_t expected_packets;
	uint32_t bits_bytes;
	uint8_t status = 1;

	if (!cmd_data || cmd_len < 7u)
	{
		divoom_btcore_file_rx_reply_head(status);
		return;
	}

	file_size = divoom_btcore_file_rx_get_le32(cmd_data);
	path_len = cmd_data[4];
	if ((uint16_t)(7u + path_len) > cmd_len)
	{
		divoom_btcore_file_rx_reply_head(status);
		return;
	}

	/* JL TF 搬文件：FILE_HEAD 带 dest；歌单路径也可由 0x42 预先 set_save_path */
	if (path_len > 0u)
	{
		char dest[DIVOOM_BTCORE_FILE_RX_MAX_PATH];
		size_t n;

		if ((size_t)path_len >= sizeof(dest))
		{
			divoom_print("btcore file rx head dest too long len=%u\n",
				     (unsigned)path_len);
			divoom_btcore_file_rx_reply_head(status);
			return;
		}
		memcpy(dest, cmd_data + 5, path_len);
		dest[path_len] = '\0';
		n = strlen(dest);
		if (n > 0u && dest[n - 1u] == '/')
		{
			if (n + 10u >= sizeof(dest))
			{
				divoom_btcore_file_rx_reply_head(status);
				return;
			}
			memcpy(dest + n, "update.bin", 11);
		}
		if (!divoom_btcore_file_rx_path_allowed(dest))
		{
			divoom_print("btcore file rx head dest reject: %s\n", dest);
			divoom_btcore_file_rx_reply_head(status);
			return;
		}
		if (s_file_rx.receiving)
			divoom_btcore_file_rx_reset_receive_state(1);
		else
			divoom_btcore_file_rx_reset_receive_state(0);
		divoom_btcore_file_rx_set_save_path(dest, 0);
	}
	else
	{
		divoom_print("btcore file rx head dest empty\n");
		divoom_btcore_file_rx_reply_head(status);
		return;
	}

	block_len = divoom_btcore_file_rx_get_le16(cmd_data + 5 + path_len);

	if (file_size == 0u || block_len == 0u || file_size > DIVOOM_BTCORE_FILE_RX_MAX_FILE_SIZE)
	{
		divoom_print("btcore file rx head invalid size=%u block=%u\n",
			     (unsigned)file_size, (unsigned)block_len);
		divoom_btcore_file_rx_reply_head(status);
		return;
	}

	expected_packets = (file_size + (uint32_t)block_len - 1u) / (uint32_t)block_len;
	if (expected_packets == 0u || expected_packets > DIVOOM_BTCORE_FILE_RX_MAX_PACKETS)
	{
		divoom_btcore_file_rx_reply_head(status);
		return;
	}

	divoom_btcore_file_rx_ensure_dir(s_file_rx.save_path);
	s_file_rx.fp = divoom_port_file_open(s_file_rx.save_path, "wb+");
	if (!s_file_rx.fp)
	{
		divoom_print("btcore file rx head open fail: %s\n", s_file_rx.save_path);
		divoom_btcore_file_rx_reply_head(status);
		return;
	}

	bits_bytes = (expected_packets + 7u) / 8u;
	s_file_rx.recv_bits = (uint8_t *)divoom_system_malloc(bits_bytes);
	if (!s_file_rx.recv_bits)
	{
		divoom_btcore_file_rx_close_fp();
		divoom_port_remove_file(s_file_rx.save_path);
		divoom_btcore_file_rx_reply_head(status);
		return;
	}
	memset(s_file_rx.recv_bits, 0, bits_bytes);

	s_file_rx.file_size = file_size;
	s_file_rx.block_len = block_len;
	s_file_rx.expected_packets = expected_packets;
	s_file_rx.wire_crc16 = 0;
	s_file_rx.tail_received = 0;
	s_file_rx.receiving = 1;
	s_file_rx.recv_count = 0;
	s_file_rx.last_rx_ms = divoom_system_get_tickms();
	s_file_rx.last_miss_req_ms = 0;
	s_file_rx.gap_log_count = 0;
	s_file_rx_last_packet_id = 0xFFFFFFFFu;
	if (divoom_btcore_file_rx_is_songlist_path(s_file_rx.save_path))
		divoom_tf_music_on_songlist_transfer_begin();

	status = 0;
	divoom_btcore_file_rx_reply_head(status);

	divoom_print("btcore file rx head size=%u block=%u packets=%u jl_path_len=%u save=%s\n",
		     (unsigned)file_size, (unsigned)block_len, (unsigned)expected_packets,
		     (unsigned)path_len, s_file_rx.save_path);
}

void divoom_btcore_file_rx_on_block(const uint8_t *cmd_data, uint16_t cmd_len)
{
	uint32_t packet_id;
	uint16_t data_len;
	const uint8_t *payload;
	uint16_t min_len;
	int has_crc;

	if (!s_file_rx.receiving || !cmd_data || cmd_len < 6u)
		return;

	packet_id = divoom_btcore_file_rx_get_le32(cmd_data);
	data_len = divoom_btcore_file_rx_get_le16(cmd_data + 4);
	payload = cmd_data + 6;
	min_len = (uint16_t)(6u + data_len);
	if (cmd_len < min_len)
	{
		divoom_print("btcore file rx block short id=%u cmd_len=%u need=%u\n",
			     (unsigned)packet_id, (unsigned)cmd_len, (unsigned)min_len);
		return;
	}

	has_crc = (cmd_len >= (uint16_t)(min_len + 2u));
	if (has_crc)
	{
		s_file_rx.wire_crc16 = divoom_btcore_file_rx_get_le16(cmd_data + min_len);
		s_file_rx.tail_received = 1;
	}

	if (s_file_rx_last_packet_id != 0xFFFFFFFFu &&
	    packet_id > s_file_rx_last_packet_id + 1u)
	{
		s_file_rx.gap_log_count++;
		if (s_file_rx.gap_log_count <= 4u ||
		    (s_file_rx.gap_log_count % DIVOOM_BTCORE_FILE_RX_GAP_LOG_EVERY) == 0u)
		{
			divoom_print("btcore file rx gap last=%u cur=%u count=%u\n",
				     (unsigned)s_file_rx_last_packet_id, (unsigned)packet_id,
				     (unsigned)s_file_rx.gap_log_count);
		}
	}

	if (!divoom_btcore_file_rx_store_block(packet_id, payload, data_len))
	{
		divoom_print("btcore file rx store block fail id=%u len=%u\n",
			     (unsigned)packet_id, (unsigned)data_len);
		return;
	}

	divoom_btcore_file_rx_report_fw_percent();
	s_file_rx.last_rx_ms = divoom_system_get_tickms();

	if (packet_id == 0u || has_crc
	    || (packet_id % DIVOOM_BTCORE_FILE_RX_LOG_EVERY) == 0u)
	{
		divoom_print("btcore file rx block id=%u len=%u recv=%u/%u%s\n",
			     (unsigned)packet_id, (unsigned)data_len,
			     (unsigned)s_file_rx.recv_count,
			     (unsigned)s_file_rx.expected_packets,
			     has_crc ? " crc_tail" : "");
	}
	s_file_rx_last_packet_id = packet_id;

	if (!s_file_rx.tail_received)
		return;

	if (divoom_btcore_file_rx_try_complete_and_reply())
		return;

	/* 已有尾包但仍缺包：0x3D 补传（含补发批次到达后继续要） */
	divoom_btcore_file_rx_request_missed_packets();
}

void divoom_btcore_file_rx_on_end(void)
{
	divoom_btcore_file_rx_finish_transfer();
}

#else /* !DIVOOM_TIMES_VIBE */

void divoom_btcore_file_rx_poll(void)
{
}

#endif /* DIVOOM_TIMES_VIBE */

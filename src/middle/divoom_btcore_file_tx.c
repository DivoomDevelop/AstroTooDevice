#include "divoom.h"

#ifdef DIVOOM_TIMES_VIBE

#include "divoom_btcore_file_tx.h"
#include "divoom_btcore_file_rx.h"

#include <stdio.h>
#include <string.h>

#define DIVOOM_BTCORE_FILE_TX_BLOCK_DEFAULT  1024u
#define DIVOOM_BTCORE_FILE_TX_HEAD_BUF       280u
#define DIVOOM_BTCORE_FILE_TX_BLOCK_BUF      (DIVOOM_BTCORE_FILE_TX_BLOCK_DEFAULT + 16u)

typedef struct _divoom_btcore_file_tx_ctx {
	char     local_path[256];
	uint8_t *file_data;
	uint32_t file_size;
	uint16_t block_len;
	uint32_t expected_packets;
	uint16_t file_crc16;
	int      busy;
	int      last_packet_has_crc;
	divoom_btcore_file_tx_progress_fn progress_cb;
	void    *progress_user;
} divoom_btcore_file_tx_ctx;

static divoom_btcore_file_tx_ctx s_file_tx;

static uint16_t divoom_btcore_file_tx_get_le16(const uint8_t *p)
{
	return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t divoom_btcore_file_tx_get_le32(const uint8_t *p)
{
	return (uint32_t)p[0]
	       | ((uint32_t)p[1] << 8)
	       | ((uint32_t)p[2] << 16)
	       | ((uint32_t)p[3] << 24);
}

static void divoom_btcore_file_tx_put_le16(uint8_t *p, uint16_t v)
{
	p[0] = (uint8_t)(v & 0xFFu);
	p[1] = (uint8_t)((v >> 8) & 0xFFu);
}

static void divoom_btcore_file_tx_put_le32(uint8_t *p, uint32_t v)
{
	p[0] = (uint8_t)(v & 0xFFu);
	p[1] = (uint8_t)((v >> 8) & 0xFFu);
	p[2] = (uint8_t)((v >> 16) & 0xFFu);
	p[3] = (uint8_t)((v >> 24) & 0xFFu);
}

static void divoom_btcore_file_tx_report_progress(uint32_t sent)
{
	if (s_file_tx.progress_cb)
		s_file_tx.progress_cb(s_file_tx.progress_user, sent, s_file_tx.file_size);
}

static int divoom_btcore_file_tx_load_file(const char *local_path)
{
	FILE *fp;
	long fsize;

	fp = divoom_port_file_open(local_path, "rb");
	if (!fp)
	{
		divoom_print("btcore file tx open fail: %s\n", local_path);
		return -1;
	}
	if (divoom_port_file_seek(fp, 0, SEEK_END) != 0)
	{
		divoom_port_file_close(fp);
		return -1;
	}
	fsize = divoom_port_file_tell(fp);
	if (fsize <= 0 || fsize > (long)(4u * 1024u * 1024u))
	{
		divoom_port_file_close(fp);
		divoom_print("btcore file tx size invalid: %ld\n", fsize);
		return -1;
	}
	rewind(fp);
	s_file_tx.file_data = (uint8_t *)divoom_system_malloc((size_t)fsize);
	if (!s_file_tx.file_data)
	{
		divoom_port_file_close(fp);
		return -1;
	}
	if (divoom_port_file_read(s_file_tx.file_data, 1, (size_t)fsize, fp) != (size_t)fsize)
	{
		divoom_port_file_close(fp);
		divoom_system_free(s_file_tx.file_data);
		s_file_tx.file_data = NULL;
		return -1;
	}
	divoom_port_file_close(fp);
	s_file_tx.file_size = (uint32_t)fsize;
	s_file_tx.file_crc16 = divoom_btcore_file_rx_crc16_data(s_file_tx.file_data, s_file_tx.file_size);
	return 0;
}

static void divoom_btcore_file_tx_cleanup(void)
{
	if (s_file_tx.file_data)
	{
		divoom_system_free(s_file_tx.file_data);
		s_file_tx.file_data = NULL;
	}
	s_file_tx.busy = 0;
	s_file_tx.file_size = 0;
	s_file_tx.expected_packets = 0;
}

static int divoom_btcore_file_tx_send_head(const char *jl_dest_path, uint16_t block_len)
{
	uint8_t buf[DIVOOM_BTCORE_FILE_TX_HEAD_BUF];
	uint8_t reply = 0xFFu;
	uint16_t reply_len = 0;
	size_t jl_len;
	uint16_t pos;
	int ret;

	jl_len = jl_dest_path ? strlen(jl_dest_path) : 0u;
	if (jl_len > 200u)
		jl_len = 200u;
	if ((uint16_t)(7u + jl_len) > DIVOOM_BTCORE_FILE_TX_HEAD_BUF)
		return -1;

	pos = 0;
	divoom_btcore_file_tx_put_le32(buf + pos, s_file_tx.file_size);
	pos = (uint16_t)(pos + 4u);
	buf[pos++] = (uint8_t)jl_len;
	if (jl_len > 0u)
	{
		memcpy(buf + pos, jl_dest_path, jl_len);
		pos = (uint16_t)(pos + (uint16_t)jl_len);
	}
	divoom_btcore_file_tx_put_le16(buf + pos, block_len);
	pos = (uint16_t)(pos + 2u);

	ret = divoom_btcore_uart_cmd_send_wait(DM_BTCORE_FILE_HEAD_TRANSMIT, buf, pos,
					       &reply, 1, &reply_len);
	if (ret != 0 || reply_len < 1u || reply != 0u)
	{
		divoom_print("btcore file tx head fail ret=%d status=%u\n", ret, (unsigned)reply);
		return -1;
	}
	divoom_print("btcore file tx head ok size=%u block=%u jl_path=%.*s\n",
		     (unsigned)s_file_tx.file_size, (unsigned)block_len,
		     (int)jl_len, jl_dest_path ? jl_dest_path : "");
	return 0;
}

static int divoom_btcore_file_tx_send_one_packet(uint32_t packet_id, int with_crc_tail)
{
	uint8_t buf[DIVOOM_BTCORE_FILE_TX_BLOCK_BUF];
	uint8_t reply = 0xFFu;
	uint16_t reply_len = 0;
	uint32_t off;
	uint16_t data_len;
	uint16_t frame_len;
	int ret;

	off = packet_id * (uint32_t)s_file_tx.block_len;
	if (off >= s_file_tx.file_size)
		return -1;

	data_len = s_file_tx.block_len;
	if (off + (uint32_t)data_len > s_file_tx.file_size)
		data_len = (uint16_t)(s_file_tx.file_size - off);

	divoom_btcore_file_tx_put_le32(buf, packet_id);
	divoom_btcore_file_tx_put_le16(buf + 4, data_len);
	memcpy(buf + 6, s_file_tx.file_data + off, data_len);
	frame_len = (uint16_t)(6u + data_len);

	if (with_crc_tail)
	{
		divoom_btcore_file_tx_put_le16(buf + frame_len, s_file_tx.file_crc16);
		frame_len = (uint16_t)(frame_len + 2u);
	}

	ret = divoom_btcore_uart_cmd_send_wait(DM_BTCORE_FILE_BLOCK_TRANSMIT, buf, frame_len,
					       &reply, 1, &reply_len);
	if (ret != 0 || reply_len < 1u || reply != 0u)
	{
		divoom_print("btcore file tx block fail id=%u ret=%d result=%u\n",
			     (unsigned)packet_id, ret, (unsigned)reply);
		return -1;
	}
	divoom_btcore_file_tx_report_progress(off + (uint32_t)data_len);
	return 0;
}

static int divoom_btcore_file_tx_send_all_packets(void)
{
	uint32_t pkt;
	uint32_t last_id;

	if (s_file_tx.expected_packets == 0u)
		return -1;

	last_id = s_file_tx.expected_packets - 1u;
	s_file_tx.last_packet_has_crc = 1;

	for (pkt = 0; pkt < s_file_tx.expected_packets; pkt++)
	{
		int with_crc = (pkt == last_id) ? 1 : 0;

		if (divoom_btcore_file_tx_send_one_packet(pkt, with_crc) != 0)
			return -1;
	}
	return 0;
}

static int divoom_btcore_file_tx_send_end(void)
{
	return divoom_btcore_uart_cmd_send(DM_BTCORE_FILE_END, NULL, 0);
}

int divoom_btcore_file_tx_is_busy(void)
{
	return s_file_tx.busy;
}

void divoom_btcore_file_tx_on_missed_packets(const uint8_t *cmd_data, uint16_t cmd_len)
{
	uint16_t count;
	uint16_t i;
	uint16_t pos;

	if (!s_file_tx.busy || !cmd_data || cmd_len < 2u)
		return;

	count = divoom_btcore_file_tx_get_le16(cmd_data);
	pos = 2;
	divoom_print("btcore file tx jl miss count=%u\n", (unsigned)count);
	for (i = 0; i < count; i++)
	{
		uint32_t pkt_id;

		if ((uint16_t)(pos + 4u) > cmd_len)
			break;
		pkt_id = divoom_btcore_file_tx_get_le32(cmd_data + pos);
		pos = (uint16_t)(pos + 4u);
		if (pkt_id >= s_file_tx.expected_packets)
			continue;
		if (divoom_btcore_file_tx_send_one_packet(pkt_id, 0) != 0)
			divoom_print("btcore file tx resend fail id=%u\n", (unsigned)pkt_id);
	}

	if (s_file_tx.last_packet_has_crc)
	{
		if (divoom_btcore_file_tx_send_one_packet(s_file_tx.expected_packets - 1u, 1) != 0)
			divoom_print("btcore file tx resend last+crc fail\n");
	}
}

int divoom_btcore_file_tx_send_file(const char *local_path, const char *jl_dest_path,
				    uint16_t block_len,
				    divoom_btcore_file_tx_progress_fn progress_cb, void *user_data)
{
	int ret = -1;

	if (!local_path || !local_path[0] || !divoom_btcore_uart_cmd_is_ready())
		return -1;
	if (s_file_tx.busy)
		return -1;

	if (block_len == 0u)
		block_len = DIVOOM_BTCORE_FILE_TX_BLOCK_DEFAULT;

	memset(&s_file_tx, 0, sizeof(s_file_tx));
	strncpy(s_file_tx.local_path, local_path, sizeof(s_file_tx.local_path) - 1u);
	s_file_tx.block_len = block_len;
	s_file_tx.progress_cb = progress_cb;
	s_file_tx.progress_user = user_data;
	s_file_tx.busy = 1;

	if (divoom_btcore_file_tx_load_file(local_path) != 0)
		goto out;

	s_file_tx.expected_packets =
		(s_file_tx.file_size + (uint32_t)s_file_tx.block_len - 1u) / (uint32_t)s_file_tx.block_len;
	if (s_file_tx.expected_packets == 0u)
		goto out;

	if (!jl_dest_path)
		jl_dest_path = "update.bin";

	if (divoom_btcore_file_tx_send_head(jl_dest_path, block_len) != 0)
		goto out;
	if (divoom_btcore_file_tx_send_all_packets() != 0)
		goto out;
	if (divoom_btcore_file_tx_send_end() != 0)
		goto out;

	divoom_print("btcore file tx done: %s crc=0x%04x\n", local_path,
		     (unsigned)s_file_tx.file_crc16);
	ret = 0;

out:
	divoom_btcore_file_tx_cleanup();
	return ret;
}

#endif /* DIVOOM_TIMES_VIBE */

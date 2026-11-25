#include "divoom.h"

#ifdef DIVOOM_TIMES_VIBE

#include "divoom_btcore_ota_tx.h"

#include <stdio.h>
#include <string.h>

#define DIVOOM_BTCORE_OTA_DATA_BUF  (6u + DIVOOM_BTCORE_OTA_PACKET_SIZE)
#define DIVOOM_BTCORE_OTA_START_REPLY_LEN  9u
#define DIVOOM_BTCORE_OTA_DATA_REPLY_LEN   5u

typedef struct _divoom_btcore_ota_tx_ctx {
	uint8_t *file_data;
	uint32_t file_size;
	uint32_t fw_version;
	uint32_t fw_crc32;
	uint8_t  flags;
	int      busy;
	divoom_btcore_ota_tx_progress_fn progress_cb;
	void    *progress_user;
} divoom_btcore_ota_tx_ctx;

static divoom_btcore_ota_tx_ctx s_ota_tx;

static uint16_t divoom_btcore_ota_get_le16(const uint8_t *p)
{
	return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t divoom_btcore_ota_get_le32(const uint8_t *p)
{
	return (uint32_t)p[0]
	       | ((uint32_t)p[1] << 8)
	       | ((uint32_t)p[2] << 16)
	       | ((uint32_t)p[3] << 24);
}

static void divoom_btcore_ota_put_le16(uint8_t *p, uint16_t v)
{
	p[0] = (uint8_t)(v & 0xFFu);
	p[1] = (uint8_t)((v >> 8) & 0xFFu);
}

static void divoom_btcore_ota_put_le32(uint8_t *p, uint32_t v)
{
	p[0] = (uint8_t)(v & 0xFFu);
	p[1] = (uint8_t)((v >> 8) & 0xFFu);
	p[2] = (uint8_t)((v >> 16) & 0xFFu);
	p[3] = (uint8_t)((v >> 24) & 0xFFu);
}

static void divoom_btcore_ota_report_progress(uint32_t sent)
{
	if (s_ota_tx.progress_cb)
		s_ota_tx.progress_cb(s_ota_tx.progress_user, sent, s_ota_tx.file_size);
}

static void divoom_btcore_ota_cleanup(void)
{
	if (s_ota_tx.file_data)
	{
		divoom_system_free(s_ota_tx.file_data);
		s_ota_tx.file_data = NULL;
	}
	s_ota_tx.busy = 0;
	s_ota_tx.file_size = 0;
}

static int divoom_btcore_ota_load_file(const char *local_path)
{
	FILE *fp;
	long fsize;

	fp = divoom_port_file_open(local_path, "rb");
	if (!fp)
	{
		divoom_print("btcore ota open fail: %s\n", local_path);
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
		divoom_print("btcore ota size invalid: %ld\n", fsize);
		return -1;
	}
	rewind(fp);
	s_ota_tx.file_data = (uint8_t *)divoom_system_malloc((size_t)fsize);
	if (!s_ota_tx.file_data)
	{
		divoom_port_file_close(fp);
		return -1;
	}
	if (divoom_port_file_read(s_ota_tx.file_data, 1, (size_t)fsize, fp) != (size_t)fsize)
	{
		divoom_port_file_close(fp);
		divoom_system_free(s_ota_tx.file_data);
		s_ota_tx.file_data = NULL;
		return -1;
	}
	divoom_port_file_close(fp);
	s_ota_tx.file_size = (uint32_t)fsize;
	s_ota_tx.fw_crc32 = (uint32_t)divoom_btcore_file_rx_crc16_data(s_ota_tx.file_data, s_ota_tx.file_size);
	return 0;
}

static int divoom_btcore_ota_send_start(uint32_t *resume_offset)
{
	uint8_t req[13];
	uint8_t reply[DIVOOM_BTCORE_OTA_START_REPLY_LEN];
	uint16_t reply_len = 0;
	uint8_t err;
	uint32_t echo_size;
	int ret;

	if (!resume_offset)
		return -1;

	divoom_btcore_ota_put_le32(req, s_ota_tx.fw_version);
	divoom_btcore_ota_put_le32(req + 4, s_ota_tx.file_size);
	divoom_btcore_ota_put_le32(req + 8, s_ota_tx.fw_crc32);
	req[12] = s_ota_tx.flags;

	ret = divoom_btcore_uart_cmd_send_wait_timeout(DM_BTCORE_OTA_START, req, sizeof(req),
						       reply, sizeof(reply), &reply_len,
						       DIVOOM_BTCORE_OTA_START_TIMEOUT_S);
	if (ret != 0 || reply_len < DIVOOM_BTCORE_OTA_START_REPLY_LEN)
	{
		divoom_print("btcore ota start uart fail ret=%d len=%u (wait=%ds)\n",
			     ret, (unsigned)reply_len, DIVOOM_BTCORE_OTA_START_TIMEOUT_S);
		return -1;
	}

	err = reply[0];
	echo_size = divoom_btcore_ota_get_le32(reply + 5);
	*resume_offset = divoom_btcore_ota_get_le32(reply + 1);

	if (err != DIVOOM_OTA_HOST_ERR_OK)
	{
		divoom_print("btcore ota start err=%u\n", (unsigned)err);
		return -1;
	}
	if (echo_size != s_ota_tx.file_size)
	{
		divoom_print("btcore ota start size mismatch echo=%u local=%u\n",
			     (unsigned)echo_size, (unsigned)s_ota_tx.file_size);
		return -1;
	}
	if (*resume_offset > s_ota_tx.file_size)
	{
		divoom_print("btcore ota start bad resume=%u\n", (unsigned)*resume_offset);
		return -1;
	}

	divoom_print("btcore ota start ok ver=%u size=%u crc16=0x%04x resume=%u\n",
		     (unsigned)s_ota_tx.fw_version, (unsigned)s_ota_tx.file_size,
		     (unsigned)s_ota_tx.fw_crc32, (unsigned)*resume_offset);
	return 0;
}

static int divoom_btcore_ota_send_one_data(uint32_t offset, uint16_t data_len)
{
	uint8_t buf[DIVOOM_BTCORE_OTA_DATA_BUF];
	uint8_t reply[DIVOOM_BTCORE_OTA_DATA_REPLY_LEN];
	uint16_t reply_len = 0;
	uint8_t err;
	uint32_t write_offset;
	uint16_t frame_len;
	int ret;

	if (offset + (uint32_t)data_len > s_ota_tx.file_size)
		return -1;

	divoom_btcore_ota_put_le32(buf, offset);
	divoom_btcore_ota_put_le16(buf + 4, data_len);
	memcpy(buf + 6, s_ota_tx.file_data + offset, data_len);
	frame_len = (uint16_t)(6u + data_len);

	ret = divoom_btcore_uart_cmd_send_wait_timeout(DM_BTCORE_OTA_DATA, buf, frame_len,
						       reply, sizeof(reply), &reply_len,
						       DIVOOM_BTCORE_OTA_DATA_TIMEOUT_S);
	if (ret != 0 || reply_len < DIVOOM_BTCORE_OTA_DATA_REPLY_LEN)
	{
		divoom_print("btcore ota data uart fail off=%u ret=%d len=%u\n",
			     (unsigned)offset, ret, (unsigned)reply_len);
		return -1;
	}

	err = reply[0];
	write_offset = divoom_btcore_ota_get_le32(reply + 1);
	if (err != DIVOOM_OTA_HOST_ERR_OK)
	{
		divoom_print("btcore ota data err=%u off=%u\n", (unsigned)err, (unsigned)offset);
		return -1;
	}
	if (write_offset != offset + (uint32_t)data_len)
	{
		divoom_print("btcore ota data offset mismatch sent=%u len=%u write=%u\n",
			     (unsigned)offset, (unsigned)data_len, (unsigned)write_offset);
		return -1;
	}

	divoom_btcore_ota_report_progress(write_offset);
	return 0;
}

static int divoom_btcore_ota_send_all_data(uint32_t start_offset)
{
	uint32_t offset = start_offset;
	uint32_t last_log_off = start_offset;

	while (offset < s_ota_tx.file_size)
	{
		uint32_t remain = s_ota_tx.file_size - offset;
		uint16_t chunk = DIVOOM_BTCORE_OTA_PACKET_SIZE;

		if (remain < (uint32_t)chunk)
			chunk = (uint16_t)remain;

		if (divoom_btcore_ota_send_one_data(offset, chunk) != 0)
			return -1;

		offset += (uint32_t)chunk;
		/* 约每 64KB 打一次，避免刷屏；1MB 固件约十几条 */
		if (offset - last_log_off >= 64u * 1024u || offset >= s_ota_tx.file_size)
		{
			divoom_print("btcore ota data %u/%u (%u%%)\n",
				     (unsigned)offset, (unsigned)s_ota_tx.file_size,
				     (unsigned)((offset * 100u) / s_ota_tx.file_size));
			last_log_off = offset;
		}
	}
	return 0;
}

static int divoom_btcore_ota_send_end(void)
{
	uint8_t reply[1];
	uint16_t reply_len = 0;
	int ret;

	ret = divoom_btcore_uart_cmd_send_wait_timeout(DM_BTCORE_OTA_END, NULL, 0,
						       reply, sizeof(reply), &reply_len,
						       DIVOOM_BTCORE_OTA_END_TIMEOUT_S);
	if (ret != 0 || reply_len < 1u)
	{
		divoom_print("btcore ota end uart fail ret=%d len=%u\n",
			     ret, (unsigned)reply_len);
		return -1;
	}
	if (reply[0] != DIVOOM_OTA_HOST_ERR_OK)
	{
		divoom_print("btcore ota end err=%u\n", (unsigned)reply[0]);
		return -1;
	}
	divoom_print("btcore ota end ok\n");
	return 0;
}

int divoom_btcore_ota_tx_is_busy(void)
{
	return s_ota_tx.busy;
}

int divoom_btcore_ota_tx_send_firmware(const char *local_path, uint32_t fw_version, uint8_t flags,
				       divoom_btcore_ota_tx_progress_fn progress_cb, void *user_data)
{
	uint32_t resume_offset = 0;
	int ret = -1;

	if (!local_path || !local_path[0] || !divoom_btcore_uart_cmd_is_ready())
		return -1;
	if (s_ota_tx.busy)
		return -1;

	memset(&s_ota_tx, 0, sizeof(s_ota_tx));
	s_ota_tx.fw_version = fw_version;
	s_ota_tx.flags = flags;
	s_ota_tx.progress_cb = progress_cb;
	s_ota_tx.progress_user = user_data;
	s_ota_tx.busy = 1;

	if (divoom_btcore_ota_load_file(local_path) != 0)
		goto out;

	if (divoom_btcore_ota_send_start(&resume_offset) != 0)
		goto out;
	if (divoom_btcore_ota_send_all_data(resume_offset) != 0)
		goto out;
	if (divoom_btcore_ota_send_end() != 0)
		goto out;

	divoom_print("btcore ota done: %s ver=%u size=%u\n",
		     local_path, (unsigned)fw_version, (unsigned)s_ota_tx.file_size);
	ret = 0;

out:
	divoom_btcore_ota_cleanup();
	return ret;
}

#endif /* DIVOOM_TIMES_VIBE */

#include "divoom.h"
#include "divoom_mic_record.h"

#ifdef DIVOOM_TIMES_VIBE

#include "divoom_btcore_rec_stream.h"
#include "divoom_btcore_file_rx.h"
#include "btcore_uart.h"
#include "divoom_cyc_buf.h"
#include "divoom_system_task_conf.h"
#include "divoom_port_fs.h"

#include <stdio.h>
#include <string.h>

#define REC_STREAM_PKT_MAX          512u
#define REC_STREAM_PIPE_SIZE        8192u
#define REC_STREAM_PIPE_LOW_WATER   2048u
#define REC_STREAM_FLUSH_MAX_BYTES  REC_STREAM_PIPE_SIZE
#define REC_STREAM_END_LEN          9u
#define REC_STREAM_DATA_HDR_LEN     6u
#define REC_STREAM_RX_Q_DEPTH       16u
#define REC_STREAM_RX_ITEM_MAX      (REC_STREAM_DATA_HDR_LEN + REC_STREAM_PKT_MAX)
#define REC_STREAM_WORKER_PRIO      4
#define REC_STREAM_FIRST_PKT_TIMEOUT_MS  3000u
#define REC_STREAM_STOP_WAIT_MS          5000u
#define REC_STREAM_MAX_DUR_MS            ((DIVOOM_BTCORE_REC_STREAM_MAX_DUR_SEC * 1000u) + 2000u)

typedef struct {
	divoom_btcore_rec_stream_state_e state;
	char save_path[DIVOOM_BTCORE_REC_STREAM_MAX_PATH];
	divoom_btcore_rec_stream_end_t end;
	divoom_btcore_rec_stream_done_fn done_cb;
	void *pipe;
	uint8_t *flush_chunk;
	FILE *fp;
	uint32_t expect_seq;
	uint32_t rx_bytes;
	uint16_t rx_packets;
	uint16_t payload_crc;
	uint32_t last_seq_gap_got;
	uint32_t session_start_ms;
	uint32_t deadline_ms;
	uint8_t seq_gap_logged;
	uint8_t stop_sent;
} divoom_btcore_rec_stream_ctx;

/*
 * 队列里只放帧池块指针：整项内嵌 data[518] 时，每包要在队列上做两次 528 字节拷贝，
 * 且与 len 无关。改指针后队列项降到十余字节，投递侧只拷 len 字节一次。
 * 所有权：投递方 btcore_uart_frame_alloc，消费方（含丢弃分支）btcore_uart_frame_free。
 */
typedef struct {
	uint32_t gen;
	uint16_t len;
	uint8_t cmd;
	uint8_t *data;
} rec_stream_rx_item_t;

static divoom_btcore_rec_stream_ctx s_rec = {
	.save_path = DIVOOM_BTCORE_REC_STREAM_DEFAULT_PATH,
	.state = DIVOOM_BTCORE_REC_STREAM_STATE_IDLE,
};

static void *s_rec_rx_q;
static void *s_rec_lock;
static volatile bool s_rec_worker_running;
static volatile bool s_rec_worker_done;
static uint32_t s_rec_session_gen;

static int divoom_btcore_rec_stream_open_output(void);
static void divoom_btcore_rec_stream_on_tx_stop(void);
static void divoom_btcore_rec_stream_reset_locked(void);
static void divoom_btcore_rec_stream_remove_output_if_empty(void);
static int divoom_btcore_rec_stream_end_is_ok(uint8_t reason,
					      uint8_t bytes_ok,
					      uint8_t packets_ok,
					      uint8_t crc_ok);
static void divoom_btcore_rec_stream_abort_local(uint8_t reason, int ok);
static void divoom_btcore_rec_stream_check_timeouts(uint8_t *need_auto_stop);

static uint16_t divoom_btcore_rec_stream_get_le16(const uint8_t *p)
{
	return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t divoom_btcore_rec_stream_get_le32(const uint8_t *p)
{
	return (uint32_t)p[0]
	       | ((uint32_t)p[1] << 8)
	       | ((uint32_t)p[2] << 16)
	       | ((uint32_t)p[3] << 24);
}

static int divoom_btcore_rec_stream_state_accepts_data(divoom_btcore_rec_stream_state_e state)
{
	return state == DIVOOM_BTCORE_REC_STREAM_STATE_WAIT_FIRST_PKT
	       || state == DIVOOM_BTCORE_REC_STREAM_STATE_RECORDING
	       || state == DIVOOM_BTCORE_REC_STREAM_STATE_STOPPING;
}

static void divoom_btcore_rec_stream_free_buffers(void)
{
	if (s_rec.pipe) {
		divoom_cyc_buf_exit(s_rec.pipe);
		s_rec.pipe = NULL;
	}
	if (s_rec.flush_chunk) {
		divoom_system_free(s_rec.flush_chunk);
		s_rec.flush_chunk = NULL;
	}
}

static void divoom_btcore_rec_stream_close_file(void)
{
	if (s_rec.fp) {
		fflush(s_rec.fp);
		divoom_port_file_close(s_rec.fp);
		s_rec.fp = NULL;
	}
}

static void divoom_btcore_rec_stream_remove_output_if_empty(void)
{
	divoom_btcore_rec_stream_close_file();
	if (s_rec.rx_bytes == 0u && s_rec.save_path[0])
		divoom_port_remove_file(s_rec.save_path);
}

static int divoom_btcore_rec_stream_flush_pipe_to_file_bounded(int max_bytes)
{
	int flushed = 0;
	int pending;
	int n;
	const int chunk_max = REC_STREAM_FLUSH_MAX_BYTES;

	if (!s_rec.fp || !s_rec.pipe)
		return 0;

	for (;;) {
		if (max_bytes > 0 && flushed >= max_bytes)
			break;
		pending = divoom_cyc_buf_get_data_size(s_rec.pipe);
		if (pending <= 0)
			break;
		n = pending > chunk_max ? chunk_max : pending;
		if (max_bytes > 0 && flushed + n > max_bytes)
			n = max_bytes - flushed;
		n = divoom_cyc_buf_read(s_rec.pipe, s_rec.flush_chunk, n);
		if (n <= 0)
			break;
		if (divoom_port_file_write(s_rec.flush_chunk, 1, (size_t)n, s_rec.fp) != (size_t)n)
			return -1;
		flushed += n;
	}
	return 0;
}

static int divoom_btcore_rec_stream_flush_pipe_to_file(void)
{
	return divoom_btcore_rec_stream_flush_pipe_to_file_bounded(0);
}

static void divoom_btcore_rec_stream_maybe_flush(uint16_t plen)
{
	int free_sz;

	if (!s_rec.fp || !s_rec.pipe)
		return;

	free_sz = divoom_cyc_buf_get_free_size(s_rec.pipe);
	if (free_sz >= (int)(plen + REC_STREAM_PIPE_LOW_WATER))
		return;

	if (divoom_btcore_rec_stream_flush_pipe_to_file_bounded(REC_STREAM_FLUSH_MAX_BYTES) != 0)
		divoom_print("rec_stream: backpressure flush failed\n");
}

static void divoom_btcore_rec_stream_rx_queue_drain(void)
{
	rec_stream_rx_item_t item;

	if (!s_rec_rx_q)
		return;

	while (divoom_port_queue_recv(s_rec_rx_q, &item, 0) == 0)
		btcore_uart_frame_free(item.data);
}

static int divoom_btcore_rec_stream_open_output(void)
{
	divoom_btcore_rec_stream_close_file();

	s_rec.fp = divoom_port_file_open(s_rec.save_path, "wb");
	if (!s_rec.fp) {
		divoom_print("rec_stream: open %s failed\n", s_rec.save_path);
		return -1;
	}
	return 0;
}

static int divoom_btcore_rec_stream_begin_session(void)
{
	uint32_t now = divoom_system_get_tickms();

	divoom_btcore_rec_stream_close_file();
	divoom_btcore_rec_stream_free_buffers();
	s_rec.pipe = divoom_cyc_buf_init(REC_STREAM_PIPE_SIZE);
	if (s_rec.pipe)
		s_rec.flush_chunk = divoom_system_malloc(REC_STREAM_FLUSH_MAX_BYTES);
	if (!s_rec.pipe || !s_rec.flush_chunk) {
		divoom_btcore_rec_stream_free_buffers();
		divoom_print("rec_stream: session buffer allocation failed, free=%ldKB largest=%ldKB\n",
			     divoom_system_get_ram_free_size(),
			     divoom_system_get_ram_largest_free_block_size());
		s_rec.state = DIVOOM_BTCORE_REC_STREAM_STATE_IDLE;
		return -1;
	}

	s_rec.expect_seq = 0u;
	s_rec.rx_bytes = 0u;
	s_rec.rx_packets = 0u;
	s_rec.payload_crc = 0u;
	s_rec.seq_gap_logged = 0u;
	s_rec.last_seq_gap_got = 0xffffffffu;
	s_rec.stop_sent = 0u;
	s_rec.session_start_ms = now;
	s_rec.deadline_ms = now + REC_STREAM_FIRST_PKT_TIMEOUT_MS;
	memset(&s_rec.end, 0, sizeof(s_rec.end));
	s_rec.state = DIVOOM_BTCORE_REC_STREAM_STATE_WAIT_FIRST_PKT;

	if (divoom_btcore_rec_stream_open_output() != 0)
		divoom_print("rec_stream: pre-open %s failed\n", s_rec.save_path);
	return 0;
}

static void divoom_btcore_rec_stream_finish(int ok)
{
	divoom_btcore_rec_stream_done_fn cb = s_rec.done_cb;
	const char *path = s_rec.save_path;

	s_rec.deadline_ms = 0u;
	divoom_btcore_rec_stream_flush_pipe_to_file();
	divoom_btcore_rec_stream_close_file();
	divoom_btcore_rec_stream_free_buffers();
	s_rec.state = DIVOOM_BTCORE_REC_STREAM_STATE_END;

	if (cb)
		cb(path, &s_rec.end, ok);
}

static int divoom_btcore_rec_stream_end_is_ok(uint8_t reason,
					      uint8_t bytes_ok,
					      uint8_t packets_ok,
					      uint8_t crc_ok)
{
	if (reason == DIVOOM_BTCORE_REC_STREAM_END_ENC_ERR
	    || reason == DIVOOM_BTCORE_REC_STREAM_END_TX_ERR
	    || reason == DIVOOM_BTCORE_REC_STREAM_END_START_TIMEOUT
	    || reason == DIVOOM_BTCORE_REC_STREAM_END_STOP_TIMEOUT)
		return 0;
	if (!bytes_ok || !packets_ok || !crc_ok)
		return 0;
	return 1;
}

static void divoom_btcore_rec_stream_abort_local(uint8_t reason, int ok)
{
	s_rec.end.total_bytes = s_rec.rx_bytes;
	s_rec.end.total_packets = s_rec.rx_packets;
	s_rec.end.payload_crc = s_rec.payload_crc;
	s_rec.end.reason = reason;
	s_rec.end.bytes_ok = 1u;
	s_rec.end.packets_ok = 1u;
	s_rec.end.crc_ok = 1u;

	divoom_print("rec_stream: abort reason=%u bytes=%u ok=%d\n",
		     (unsigned)reason, (unsigned)s_rec.rx_bytes, ok);

	if (s_rec.rx_bytes == 0u) {
		divoom_btcore_rec_stream_remove_output_if_empty();
		divoom_btcore_rec_stream_free_buffers();
		s_rec.deadline_ms = 0u;
		s_rec.state = DIVOOM_BTCORE_REC_STREAM_STATE_END;
		if (s_rec.done_cb)
			s_rec.done_cb(s_rec.save_path, &s_rec.end, ok);
		return;
	}

	divoom_btcore_rec_stream_finish(ok);
}

static void divoom_btcore_rec_stream_on_tx_stop(void)
{
	if (s_rec.state == DIVOOM_BTCORE_REC_STREAM_STATE_IDLE
	    || s_rec.state == DIVOOM_BTCORE_REC_STREAM_STATE_END)
		return;

	if (s_rec.state == DIVOOM_BTCORE_REC_STREAM_STATE_WAIT_FIRST_PKT) {
		divoom_print("rec_stream: stop before first 0x63\n");
		divoom_btcore_rec_stream_abort_local(DIVOOM_BTCORE_REC_STREAM_END_OK, 0);
		return;
	}

	if (s_rec.state == DIVOOM_BTCORE_REC_STREAM_STATE_STOPPING)
		return;

	s_rec.state = DIVOOM_BTCORE_REC_STREAM_STATE_STOPPING;
	s_rec.deadline_ms = divoom_system_get_tickms() + REC_STREAM_STOP_WAIT_MS;
	s_rec.stop_sent = 1u;
}

static void divoom_btcore_rec_stream_check_timeouts(uint8_t *need_auto_stop)
{
	uint32_t now = divoom_system_get_tickms();

	if (need_auto_stop)
		*need_auto_stop = 0u;

	if (s_rec.state == DIVOOM_BTCORE_REC_STREAM_STATE_WAIT_FIRST_PKT
	    && s_rec.deadline_ms != 0u && now >= s_rec.deadline_ms) {
		divoom_print("rec_stream: start timeout, no 0x63 (JL busy or rejected)\n");
		divoom_btcore_rec_stream_abort_local(DIVOOM_BTCORE_REC_STREAM_END_START_TIMEOUT, 0);
		return;
	}

	if (s_rec.state == DIVOOM_BTCORE_REC_STREAM_STATE_STOPPING
	    && s_rec.deadline_ms != 0u && now >= s_rec.deadline_ms) {
		divoom_print("rec_stream: stop timeout, no 0x64\n");
		divoom_btcore_rec_stream_abort_local(DIVOOM_BTCORE_REC_STREAM_END_STOP_TIMEOUT, 0);
		return;
	}

	if (!s_rec.stop_sent
	    && (s_rec.state == DIVOOM_BTCORE_REC_STREAM_STATE_WAIT_FIRST_PKT
		|| s_rec.state == DIVOOM_BTCORE_REC_STREAM_STATE_RECORDING)
	    && s_rec.session_start_ms != 0u
	    && (now - s_rec.session_start_ms) >= REC_STREAM_MAX_DUR_MS
	    && need_auto_stop) {
		*need_auto_stop = 1u;
	}
}

int divoom_btcore_rec_stream_on_tx_record(uint8_t action, uint8_t mode)
{
	int rc = 0;
	(void)mode;

	if (divoom_btcore_rec_stream_init() != 0)
		return -1;
	divoom_system_get_lock(s_rec_lock);

	if (!action) {
		divoom_btcore_rec_stream_on_tx_stop();
	} else {
		if (divoom_btcore_rec_stream_state_accepts_data(s_rec.state))
			divoom_print("rec_stream: start while active, reset\n");
		if (s_rec.state != DIVOOM_BTCORE_REC_STREAM_STATE_IDLE)
			divoom_btcore_rec_stream_reset_locked();
		uint32_t gen = __atomic_add_fetch(&s_rec_session_gen, 1, __ATOMIC_RELAXED);
		rc = divoom_btcore_rec_stream_begin_session();
		if (!rc)
			divoom_print("rec_stream: start gen=%u wait 0x63\n", (unsigned)gen);
	}

	divoom_system_free_lock(s_rec_lock);
	return rc;
}

int divoom_btcore_rec_stream_send_record(uint8_t start, uint8_t mode)
{
	uint8_t payload[2];

	if (!divoom_btcore_uart_cmd_is_ready())
		return -1;

	payload[0] = start ? 1u : 0u;
	payload[1] = mode;
	return divoom_btcore_uart_cmd_send(DM_BTCORE_RECORD, payload, 2);
}

int divoom_btcore_rec_stream_set_save_path(const char *path)
{
	if (!path || !path[0])
		return -1;
	strncpy(s_rec.save_path, path, sizeof(s_rec.save_path) - 1u);
	s_rec.save_path[sizeof(s_rec.save_path) - 1u] = '\0';
	return 0;
}

const char *divoom_btcore_rec_stream_get_save_path(void)
{
	return s_rec.save_path;
}

void divoom_btcore_rec_stream_set_done_callback(divoom_btcore_rec_stream_done_fn cb)
{
	s_rec.done_cb = cb;
}

static void divoom_btcore_rec_stream_reset_locked(void)
{
	divoom_btcore_rec_stream_rx_queue_drain();
	divoom_btcore_rec_stream_flush_pipe_to_file();
	divoom_btcore_rec_stream_close_file();
	divoom_btcore_rec_stream_free_buffers();
	memset(&s_rec.end, 0, sizeof(s_rec.end));
	s_rec.expect_seq = 0u;
	s_rec.rx_bytes = 0u;
	s_rec.rx_packets = 0u;
	s_rec.payload_crc = 0u;
	s_rec.seq_gap_logged = 0u;
	s_rec.last_seq_gap_got = 0xffffffffu;
	s_rec.session_start_ms = 0u;
	s_rec.deadline_ms = 0u;
	s_rec.stop_sent = 0u;
	s_rec.state = DIVOOM_BTCORE_REC_STREAM_STATE_IDLE;
	if (!s_rec.save_path[0]) {
		strncpy(s_rec.save_path, DIVOOM_BTCORE_REC_STREAM_DEFAULT_PATH,
			sizeof(s_rec.save_path) - 1u);
	}
}

void divoom_btcore_rec_stream_reset(void)
{
	if (!s_rec_lock)
		return;
	divoom_system_get_lock(s_rec_lock);
	divoom_btcore_rec_stream_reset_locked();
	divoom_system_free_lock(s_rec_lock);
}

divoom_btcore_rec_stream_state_e divoom_btcore_rec_stream_get_state(void)
{
	return s_rec.state;
}

int divoom_btcore_rec_stream_is_active(void)
{
	return s_rec.state == DIVOOM_BTCORE_REC_STREAM_STATE_WAIT_FIRST_PKT
	       || s_rec.state == DIVOOM_BTCORE_REC_STREAM_STATE_RECORDING
	       || s_rec.state == DIVOOM_BTCORE_REC_STREAM_STATE_STOPPING ? 1 : 0;
}

const divoom_btcore_rec_stream_end_t *divoom_btcore_rec_stream_get_end(void)
{
	return &s_rec.end;
}

void divoom_btcore_rec_stream_on_data(const uint8_t *data, uint16_t len)
{
	uint32_t seq;
	uint16_t plen;
	const uint8_t *payload;
	int wlen;

	if (!data || len < REC_STREAM_DATA_HDR_LEN)
		return;

	if (s_rec.state == DIVOOM_BTCORE_REC_STREAM_STATE_END
	    || s_rec.state == DIVOOM_BTCORE_REC_STREAM_STATE_IDLE)
		return;

	seq = divoom_btcore_rec_stream_get_le32(data);
	plen = divoom_btcore_rec_stream_get_le16(data + 4);
	if (plen == 0u || (uint16_t)(REC_STREAM_DATA_HDR_LEN + plen) > len) {
		divoom_print("rec_stream: bad len seq=%u plen=%u frame=%u\n",
			     (unsigned)seq, (unsigned)plen, (unsigned)len);
		return;
	}
	if (plen > REC_STREAM_PKT_MAX) {
		divoom_print("rec_stream: plen %u > max\n", (unsigned)plen);
		return;
	}

	payload = data + REC_STREAM_DATA_HDR_LEN;

	if (!divoom_btcore_rec_stream_state_accepts_data(s_rec.state)) {
		if (!s_rec.seq_gap_logged) {
			divoom_print("rec_stream: data ignored seq=%u state=%d\n",
				     (unsigned)seq, (int)s_rec.state);
			s_rec.seq_gap_logged = 1u;
		}
		return;
	}

	if (s_rec.state == DIVOOM_BTCORE_REC_STREAM_STATE_WAIT_FIRST_PKT) {
		s_rec.state = DIVOOM_BTCORE_REC_STREAM_STATE_RECORDING;
		s_rec.deadline_ms = 0u;
		divoom_print("rec_stream: first 0x63 seq=%u\n", (unsigned)seq);
	}

	if (!s_rec.fp) {
		if (divoom_btcore_rec_stream_open_output() != 0) {
			divoom_print("rec_stream: open output failed seq=%u\n", (unsigned)seq);
			return;
		}
	}

	if (seq != s_rec.expect_seq) {
		if (!s_rec.seq_gap_logged || seq != s_rec.last_seq_gap_got) {
			divoom_print("rec_stream: seq gap expect=%u got=%u\n",
				     (unsigned)s_rec.expect_seq, (unsigned)seq);
			s_rec.seq_gap_logged = 1u;
			s_rec.last_seq_gap_got = seq;
		}
		return;
	}

	divoom_btcore_rec_stream_maybe_flush(plen);

	wlen = divoom_cyc_buf_write(s_rec.pipe, (uint8_t *)payload, (int)plen);
	if (wlen < (int)plen) {
		if (divoom_btcore_rec_stream_flush_pipe_to_file() != 0) {
			divoom_print("rec_stream: drain flush failed seq=%u\n", (unsigned)seq);
			return;
		}
		wlen = divoom_cyc_buf_write(s_rec.pipe, (uint8_t *)payload, (int)plen);
	}
	if (wlen < (int)plen) {
		divoom_print("rec_stream: pipe full seq=%u free=%d need=%u\n",
			     (unsigned)seq,
			     s_rec.pipe ? divoom_cyc_buf_get_free_size(s_rec.pipe) : 0,
			     (unsigned)plen);
		return;
	}

	s_rec.payload_crc = divoom_btcore_file_rx_crc16_chain(payload, plen, s_rec.payload_crc);
	s_rec.expect_seq++;
	s_rec.rx_packets++;
	s_rec.rx_bytes += plen;
	divoom_mic_record_stream_data(payload, plen, 1, 16000);
}

void divoom_btcore_rec_stream_on_end(const uint8_t *data, uint16_t len)
{
	uint32_t total_bytes;
	uint16_t total_packets;
	uint16_t payload_crc;
	uint8_t reason;
	int ok = 0;

	if (!data || len < REC_STREAM_END_LEN)
		return;

	if (s_rec.state == DIVOOM_BTCORE_REC_STREAM_STATE_IDLE) {
		divoom_print("rec_stream: end in idle, drop\n");
		return;
	}

	total_bytes = divoom_btcore_rec_stream_get_le32(data);
	total_packets = divoom_btcore_rec_stream_get_le16(data + 4);
	payload_crc = divoom_btcore_rec_stream_get_le16(data + 6);
	reason = data[8];

	s_rec.end.total_bytes = total_bytes;
	s_rec.end.total_packets = total_packets;
	s_rec.end.payload_crc = payload_crc;
	s_rec.end.reason = reason;
	s_rec.end.bytes_ok = (total_bytes == s_rec.rx_bytes) ? 1u : 0u;
	s_rec.end.packets_ok = (total_packets == s_rec.rx_packets) ? 1u : 0u;
	s_rec.end.crc_ok = (payload_crc == s_rec.payload_crc) ? 1u : 0u;

	ok = divoom_btcore_rec_stream_end_is_ok(reason,
					      s_rec.end.bytes_ok,
					      s_rec.end.packets_ok,
					      s_rec.end.crc_ok);

	if (!ok) {
		divoom_print("rec_stream: end fail reason=%u bytes=%u/%u pkt=%u/%u crc=%04x/%04x\n",
			     (unsigned)reason,
			     (unsigned)total_bytes, (unsigned)s_rec.rx_bytes,
			     (unsigned)total_packets, (unsigned)s_rec.rx_packets,
			     (unsigned)payload_crc, (unsigned)s_rec.payload_crc);
	} else if (divoom_btcore_rec_stream_reason_is_preempt(reason)) {
		divoom_print("rec_stream: end preempt reason=%u bytes=%u pkts=%u path=%s\n",
			     (unsigned)reason,
			     (unsigned)total_bytes,
			     (unsigned)total_packets,
			     s_rec.save_path);
	} else {
		divoom_print("rec_stream: end ok reason=%u bytes=%u pkts=%u path=%s\n",
			     (unsigned)reason,
			     (unsigned)total_bytes,
			     (unsigned)total_packets,
			     s_rec.save_path);
	}

	divoom_btcore_rec_stream_finish(ok);
}

static void divoom_btcore_rec_stream_worker_main(void *arg)
{
	(void)arg;

	divoom_print("rec_stream worker enter\n");
	while (s_rec_worker_running) {
		rec_stream_rx_item_t item;
		uint8_t need_auto_stop = 0u;

		if (divoom_port_queue_recv(s_rec_rx_q, &item, 50) != 0) {
			if (s_rec_lock)
				divoom_system_get_lock(s_rec_lock);
			divoom_btcore_rec_stream_check_timeouts(&need_auto_stop);
			if (s_rec_lock)
				divoom_system_free_lock(s_rec_lock);
			if (need_auto_stop)
				divoom_btcore_rec_stream_send_record(0, 0);
			continue;
		}

		if (s_rec_lock)
			divoom_system_get_lock(s_rec_lock);

		uint32_t gen = __atomic_load_n(&s_rec_session_gen, __ATOMIC_RELAXED);
		if (item.gen != gen) {
			divoom_print("rec_stream: drop stale gen=%u cur=%u cmd=0x%02x\n",
				     (unsigned)item.gen, (unsigned)gen,
				     (unsigned)item.cmd);
			if (s_rec_lock)
				divoom_system_free_lock(s_rec_lock);
			btcore_uart_frame_free(item.data);
			continue;
		}

		if (item.cmd == DM_BTCORE_REC_STREAM_DATA)
			divoom_btcore_rec_stream_on_data(item.data, item.len);
		else if (item.cmd == DM_BTCORE_REC_STREAM_END)
			divoom_btcore_rec_stream_on_end(item.data, item.len);

		divoom_btcore_rec_stream_check_timeouts(&need_auto_stop);

		if (s_rec_lock)
			divoom_system_free_lock(s_rec_lock);

		btcore_uart_frame_free(item.data);

		if (need_auto_stop)
			divoom_btcore_rec_stream_send_record(0, 0);
	}

	/* 归还队列里残留的池块，避免线程退出后块被永久占用 */
	divoom_btcore_rec_stream_rx_queue_drain();

	divoom_print("rec_stream worker exit\n");
	s_rec_worker_done = true;
}

int divoom_btcore_rec_stream_init(void)
{
	if (s_rec_rx_q)
		return 0;

	s_rec_lock = divoom_system_create_lock();
	if (!s_rec_lock) {
		divoom_print("rec_stream: init lock fail\n");
		return -1;
	}
	s_rec_rx_q = divoom_port_queue_create("rec_rx", sizeof(rec_stream_rx_item_t),
					      REC_STREAM_RX_Q_DEPTH);
	if (!s_rec_rx_q) {
		divoom_system_destroy_lock(s_rec_lock);
		s_rec_lock = NULL;
		divoom_print("rec_stream: init queue fail\n");
		return -1;
	}

	s_rec_worker_running = true;
	s_rec_worker_done = false;
	if (!divoom_system_create_task_prio(divoom_btcore_rec_stream_worker_main,
					    "rec_rx", NULL, DIVOOM_TASK_STACK_4K,
					    REC_STREAM_WORKER_PRIO)) {
		divoom_print("rec_stream: create worker fail\n");
		divoom_port_queue_destroy(s_rec_rx_q);
		s_rec_rx_q = NULL;
		divoom_system_destroy_lock(s_rec_lock);
		s_rec_lock = NULL;
		s_rec_worker_running = false;
		return -1;
	}

	divoom_print("rec_stream: worker ready q=%u\n", (unsigned)REC_STREAM_RX_Q_DEPTH);
	return 0;
}

int divoom_btcore_rec_stream_rx_enqueue(uint8_t cmd, const uint8_t *data, uint16_t len)
{
	rec_stream_rx_item_t item;

	if (!s_rec_rx_q) {
		if (divoom_btcore_rec_stream_init() != 0)
			return -1;
	}
	if (!data || len == 0u || len > REC_STREAM_RX_ITEM_MAX)
		return -1;

	item.data = btcore_uart_frame_alloc(len);
	if (!item.data) {
		static uint32_t alloc_fail;

		/* 帧池有堆兜底，走到这里是池与堆都拿不到；每包都打会刷屏，按次数抽样 */
		if ((alloc_fail++ % 32u) == 0u)
			divoom_print("rec_stream: frame alloc fail cmd=0x%02x len=%u fail=%u\n",
				     (unsigned)cmd, (unsigned)len, (unsigned)alloc_fail);
		return -1;
	}
	item.gen = __atomic_load_n(&s_rec_session_gen, __ATOMIC_RELAXED);
	item.cmd = cmd;
	item.len = len;
	memcpy(item.data, data, len);

	if (divoom_port_queue_send(s_rec_rx_q, &item, 0) != 0) {
		divoom_print("rec_stream: rx queue full cmd=0x%02x len=%u depth=%u\n",
			     (unsigned)cmd, (unsigned)len,
			     (unsigned)divoom_port_queue_len(s_rec_rx_q));
		btcore_uart_frame_free(item.data);
		return -1;
	}
	return 0;
}

#endif /* DIVOOM_TIMES_VIBE */

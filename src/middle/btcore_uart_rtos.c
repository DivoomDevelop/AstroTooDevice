/*
 * BtCore UART on F101 RTOS: divoom_port_uart on [uart3] PD0/PD1 (1.5Mbps).
 * Protocol/framing logic matches btcore_uart.c (Linux ttyS1 build).
 *
 * RX path（btcore_rx / btcore_work / btcore_slow 三线程）：
 *   1. btcore_rx_pump()          — port UART 块读 → FSM 组帧
 *   2. btcore_rx_route_frame()   — sync reply 优先，其余按 app 层声明的执行上下文分流：
 *                                  CTX_FAST 本线程同步跑完，CTX_STREAM 投录音流队列，
 *                                  CTX_WORK / CTX_SLOW 取帧池块投对应队列
 *   3. btcore_msg_thread_loop()  — work / slow 队列 → s_rx_cb (business)，消费后归还帧池
 *
 * 一线缓冲在 port UART 实现里（真机为 HAL RX 环）：中断整批写入，本线程块读。
 * app 层不再另建 ring 多做一次搬运。
 */
#include "btcore_uart.h"
#include "divoom.h"
#include "divoom_btcore_rec_stream.h"
#include "divoom_port_uart.h"
#include "divoom_system_task_conf.h"

#if !DIVOOM_BUILD_RTOS_DEVICE
#error "btcore_uart_rtos.c is RTOS device-only; use btcore_uart.c in simulator."
#endif

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/*
 * 三条 RX 线程的优先级按「谁被堵住会丢数据 / 谁被堵住只是慢」排序：
 *
 * btcore_rx(9)   唯一排空 HAL 环的线程，纯 CPU 短任务（解析 + 分类 + CTX_FAST），
 *                放在 LVGL(10) 之下、所有业务线程之上：不威胁 UI 与音频，又不会
 *                被 mic(8)/disp(7)/tone(5) 之类压住。环的 218ms 余量兜底剩余抖动。
 * btcore_work(6) 轻量业务与 UI 状态机，按键响应经此路径，高于 tone(5) 与默认(2)，
 *                低于 disp_ctrl(7) 与 mic(8)。
 * btcore_slow(3) 文件 IO 与 sqlite，单次可占用很久，压到交互路径之下。
 */
#define BTCORE_RX_PARSE_TASK_PRIO  9
#define BTCORE_RX_WORK_TASK_PRIO   6
#define BTCORE_RX_SLOW_TASK_PRIO   3
#define BTCORE_RX_WORK_QUEUE_DEPTH 128
/*
 * 慢队列除文件块外还有歌单条目等短命令（占 S 级块）。
 * TF zip 约 3MB/1024B 连发时，略加深以吸收写盘抖动。
 */
#define BTCORE_RX_SLOW_QUEUE_DEPTH 96
#define BTCORE_RX_CMD_POISON       0xFFu

#if BTCORE_RX_PARSE_TASK_PRIO >= DIVOOM_LVGL_HANDLER_TASK_PRIO
#error btcore_rx prio must stay below LVGL handler prio
#endif
#if BTCORE_RX_PARSE_TASK_PRIO <= BTCORE_RX_WORK_TASK_PRIO
#error btcore_rx prio must stay above btcore_work prio
#endif
#if BTCORE_RX_WORK_TASK_PRIO <= BTCORE_RX_SLOW_TASK_PRIO
#error btcore_work prio must stay above btcore_slow prio
#endif

#define BTCORE_HEADER     0xAA
#define BTCORE_TAIL       0xBB
/* 单次从 port UART 块读的上限；容纳最大帧（1807B）即可，取整为 2KB */
#define BTCORE_RX_CHUNK_SIZE   2048u
#define BTCORE_MAX_FRAME_SIZE  (7u + BTCORE_MAX_CMD_DATA_LEN)
#define BTCORE_MAX_L_BODY  (BTCORE_MAX_CMD_DATA_LEN + 2)
#define BTCORE_FRAME_LOG_HEX_MAX  128
/*
 * 帧数据池，替代每帧一次 divoom_system_malloc。
 *
 * 分三级覆盖命令数据长度分布：上限取 2KB，因为 BTCORE_MAX_CMD_DATA_LEN 名义 4096，
 * 但协议实际最大是 JL 侧 1792 字节文件块（整帧 1807）。
 * 分配发生在 btcore_rx 与录音流投递路径，归还发生在 btcore_work / btcore_slow /
 * rec_rx / btcore_rx 四处，故空闲链表用临界区保护；每次只摘挂一个指针，
 * 开销远低于 malloc 且不产生碎片。
 *
 * 池取不到时退到堆（见 BTCORE_FRAME_LEVEL_HEAP），因此容量估小的代价是内存短时
 * 上涨与热路径多一次 malloc，而不是丢帧。上涨幅度由 s_frame_heap_peak 反映。
 */
#define BTCORE_FRAME_POOL_LEVELS  3
#define BTCORE_FRAME_HDR_SIZE     4u
/* S：按键 / 电量 / EQ 等短命令 */
#define BTCORE_POOL_S_SIZE        64u
#define BTCORE_POOL_S_COUNT       48u
/* M：录音流包 6+512=518，取 640 留余量 */
#define BTCORE_POOL_M_SIZE        640u
#define BTCORE_POOL_M_COUNT       24u
/* L：文件块 6+1792=1798 */
#define BTCORE_POOL_L_SIZE        2048u
#define BTCORE_POOL_L_COUNT       16u
/*
 * 槽 = 4 字节头 + 数据，槽长向上取整到 8。数据区起点是槽首 +4，故实际为 4 字节对齐；
 * 协议只按字节与 le16/le32 存取，够用。将来若要在块内放 8 字节类型，需先扩头到 8。
 */
#define BTCORE_POOL_SLOT(size)    ((((size) + BTCORE_FRAME_HDR_SIZE) + 7u) & ~7u)
/*
 * 堆兜底的级别标记。三级全空、或长度超过最大级时改从堆取块而不是丢帧：
 * 短命令（按键、电量）没有重传可补，容量估小的代价不该是丢事件。
 * 常态下一个都不该走到这里，故 s_frame_heap_* 三个计数即「池容量是否真的够」的判据。
 */
#define BTCORE_FRAME_LEVEL_HEAP   0xFFu
/* idle wait for first byte only; do not use for bulk read (see rx thread). */
#define BTCORE_RX_IDLE_WAIT_MS       50

typedef struct {
	uint8_t  cmd;
	uint16_t len;
	uint8_t *data;
} btcore_rx_msg_t;

typedef enum {
	BTCORE_FSM_HUNT,
	BTCORE_FSM_COLLECT,
} btcore_rx_fsm_state_t;

typedef struct {
	btcore_rx_fsm_state_t state;
	uint16_t              need;
	uint16_t              pos;
	uint8_t               frame[BTCORE_MAX_FRAME_SIZE];
} btcore_rx_fsm_t;

static int                  s_uart = DIVOOM_PORT_UART_BTCORE;
static bool                 s_uart_open;
static bool                 s_thread_running;
static bool                 s_rx_thread_done;
static bool                 s_work_thread_running;
static bool                 s_work_thread_done;
static bool                 s_slow_thread_running;
static bool                 s_slow_thread_done;
static bool                 s_inited;
static void                *s_tx_mutex;
static void                *s_rx_work_queue;
static void                *s_rx_slow_queue;

static uint32_t             s_send_uart_len;

static uint8_t              *s_send_uart_buf = NULL;

static btcore_uart_on_cmd_fn s_rx_cb;
static btcore_rx_fsm_t      s_rx_fsm;
static uint8_t              s_rx_chunk[BTCORE_RX_CHUNK_SIZE];

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
	uint32_t u = (uint32_t)v;

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

static int btcore_enqueue_parsed_cmd(void *q, uint8_t cmd, uint8_t *cmd_data,
				     uint16_t cmd_len);

/* --- 帧数据池 --- */

typedef struct btcore_frame_blk {
	struct btcore_frame_blk *next;
} btcore_frame_blk_t;

typedef struct {
	uint8_t            *mem;
	uint32_t            blk_size;
	uint32_t            blk_count;
	btcore_frame_blk_t *free_list;
	uint32_t            in_use;
	uint32_t            peak;
	uint32_t            starve;   /* 本级取空次数（可能已向上级借到） */
} btcore_frame_pool_t;

static uint8_t s_pool_mem_s[BTCORE_POOL_S_COUNT * BTCORE_POOL_SLOT(BTCORE_POOL_S_SIZE)]
	__attribute__((aligned(8)));
static uint8_t s_pool_mem_m[BTCORE_POOL_M_COUNT * BTCORE_POOL_SLOT(BTCORE_POOL_M_SIZE)]
	__attribute__((aligned(8)));
static uint8_t s_pool_mem_l[BTCORE_POOL_L_COUNT * BTCORE_POOL_SLOT(BTCORE_POOL_L_SIZE)]
	__attribute__((aligned(8)));

static btcore_frame_pool_t s_frame_pools[BTCORE_FRAME_POOL_LEVELS];
static uint32_t s_frame_pool_fail;      /* 三级全空（随后转堆兜底，不等于丢帧） */
static uint32_t s_frame_pool_oversize;  /* 超过最大级，协议上不该出现 */
static uint32_t s_frame_heap_now;       /* 当前在途堆块数 */
static uint32_t s_frame_heap_peak;      /* 在途堆块峰值 */
static uint32_t s_frame_heap_total;     /* 累计走堆次数，常态应为 0 */
static uint32_t s_frame_heap_fail;      /* 堆也拿不到，此时才真丢帧 */
static bool     s_frame_pool_ready;

static void btcore_frame_pool_init_level(btcore_frame_pool_t *p, uint8_t *mem,
					 uint32_t blk_size, uint32_t blk_count,
					 uint8_t level)
{
	uint32_t slot = BTCORE_POOL_SLOT(blk_size);
	uint32_t i;

	p->mem = mem;
	p->blk_size = blk_size;
	p->blk_count = blk_count;
	p->free_list = NULL;
	p->in_use = 0;
	p->peak = 0;
	p->starve = 0;

	for (i = 0; i < blk_count; i++) {
		uint8_t *slot_p = mem + i * slot;
		btcore_frame_blk_t *blk;

		/* 槽头记所属级别，归还时无需按地址范围反查 */
		slot_p[0] = level;
		blk = (btcore_frame_blk_t *)(slot_p + BTCORE_FRAME_HDR_SIZE);
		blk->next = p->free_list;
		p->free_list = blk;
	}
}

/*
 * 可被 btcore_uart_init 与录音流投递路径分别首次触发，故建链必须原子完成：
 * 若两侧交错重建 free_list，已发出去的块会重新回到链上，被两个使用者同时持有。
 * 三级共 88 次指针挂接，临界区不足 2us。
 */
static void btcore_frame_pool_init(void)
{
	if (s_frame_pool_ready)
		return;

	divoom_port_critical_enter();
	if (!s_frame_pool_ready) {
		btcore_frame_pool_init_level(&s_frame_pools[0], s_pool_mem_s,
					     BTCORE_POOL_S_SIZE, BTCORE_POOL_S_COUNT, 0);
		btcore_frame_pool_init_level(&s_frame_pools[1], s_pool_mem_m,
					     BTCORE_POOL_M_SIZE, BTCORE_POOL_M_COUNT, 1);
		btcore_frame_pool_init_level(&s_frame_pools[2], s_pool_mem_l,
					     BTCORE_POOL_L_SIZE, BTCORE_POOL_L_COUNT, 2);
		s_frame_pool_fail = 0;
		s_frame_pool_oversize = 0;
		s_frame_heap_now = 0;
		s_frame_heap_peak = 0;
		s_frame_heap_total = 0;
		s_frame_heap_fail = 0;
		s_frame_pool_ready = true;
	}
	divoom_port_critical_exit();
}

/*
 * 池取不到时的后备路径。调用方必须已退出临界区：divoom_system_malloc 内部要加锁，
 * 在 divoom_port_critical_enter 里调用会破坏调度假设。
 */
static uint8_t *btcore_frame_alloc_heap(uint16_t len)
{
	uint8_t *slot;

	/* 协议上限之外一律不受理，防止长度域解析错误时申请超大块 */
	if (len > BTCORE_MAX_CMD_DATA_LEN) {
		s_frame_heap_fail++;
		return NULL;
	}

	/* 数据区随后整段 memcpy 覆盖，不必先清零 */
	slot = divoom_system_malloc_no_zero(BTCORE_FRAME_HDR_SIZE + len);
	if (!slot) {
		s_frame_heap_fail++;
		return NULL;
	}
	slot[0] = BTCORE_FRAME_LEVEL_HEAP;

	divoom_port_critical_enter();
	s_frame_heap_total++;
	s_frame_heap_now++;
	if (s_frame_heap_now > s_frame_heap_peak)
		s_frame_heap_peak = s_frame_heap_now;
	divoom_port_critical_exit();

	return slot + BTCORE_FRAME_HDR_SIZE;
}

static uint8_t *btcore_frame_alloc(uint16_t len)
{
	btcore_frame_blk_t *blk = NULL;
	int level;

	if (len == 0u || !s_frame_pool_ready)
		return NULL;
	if (len > BTCORE_POOL_L_SIZE) {
		s_frame_pool_oversize++;
		/* 协议上不该出现，转堆保住这帧，但仍抽样记录便于排查 */
		if ((s_frame_pool_oversize % 32u) == 1u)
			divoom_print("btcore_uart frame oversize len=%u max=%u count=%u\n",
				     (unsigned)len, (unsigned)BTCORE_POOL_L_SIZE,
				     (unsigned)s_frame_pool_oversize);
		return btcore_frame_alloc_heap(len);
	}

	for (level = 0; level < BTCORE_FRAME_POOL_LEVELS; level++)
		if (len <= s_frame_pools[level].blk_size)
			break;

	/*
	 * 本级空了就向上借。小命令（按键、电量）无重传可补，文件块有 0x3D 重传兜底，
	 * 所以优先保小命令不丢。
	 */
	divoom_port_critical_enter();
	for (; level < BTCORE_FRAME_POOL_LEVELS; level++) {
		btcore_frame_pool_t *p = &s_frame_pools[level];

		if (!p->free_list) {
			p->starve++;
			continue;
		}
		blk = p->free_list;
		p->free_list = blk->next;
		p->in_use++;
		if (p->in_use > p->peak)
			p->peak = p->in_use;
		break;
	}
	if (!blk)
		s_frame_pool_fail++;
	divoom_port_critical_exit();

	/* 三级全空。退到堆而不是丢帧，malloc 只能在临界区外做 */
	if (!blk)
		return btcore_frame_alloc_heap(len);

	return (uint8_t *)blk;
}

static void btcore_frame_free(uint8_t *data)
{
	btcore_frame_blk_t *blk;
	btcore_frame_pool_t *p;
	uint8_t level;

	if (!data)
		return;

	level = *(data - BTCORE_FRAME_HDR_SIZE);

	if (level == BTCORE_FRAME_LEVEL_HEAP) {
		divoom_port_critical_enter();
		if (s_frame_heap_now)
			s_frame_heap_now--;
		divoom_port_critical_exit();
		/* 与 alloc 对称：free 同样不能放进临界区 */
		divoom_system_free(data - BTCORE_FRAME_HDR_SIZE);
		return;
	}

	if (level >= BTCORE_FRAME_POOL_LEVELS) {
		divoom_print("btcore_uart frame free bad level=%u\n", (unsigned)level);
		return;
	}

	p = &s_frame_pools[level];
	blk = (btcore_frame_blk_t *)data;

	divoom_port_critical_enter();
	blk->next = p->free_list;
	p->free_list = blk;
	if (p->in_use)
		p->in_use--;
	divoom_port_critical_exit();
}

/* 供其他 RX 消费者（如录音流）共用同一套块，避免各自在队列里搬定长大结构 */
uint8_t *btcore_uart_frame_alloc(uint16_t len)
{
	btcore_frame_pool_init();
	return btcore_frame_alloc(len);
}

void btcore_uart_frame_free(uint8_t *data)
{
	btcore_frame_free(data);
}

void btcore_uart_frame_pool_dump(void)
{
	int i;

	for (i = 0; i < BTCORE_FRAME_POOL_LEVELS; i++)
		divoom_print("btcore_uart pool[%d] size=%u total=%u in_use=%u peak=%u starve=%u\n",
			     i, (unsigned)s_frame_pools[i].blk_size,
			     (unsigned)s_frame_pools[i].blk_count,
			     (unsigned)s_frame_pools[i].in_use,
			     (unsigned)s_frame_pools[i].peak,
			     (unsigned)s_frame_pools[i].starve);
	divoom_print("btcore_uart pool fail=%u oversize=%u\n",
		     (unsigned)s_frame_pool_fail, (unsigned)s_frame_pool_oversize);
	/* heap_total 非 0 即池容量不足的证据；heap_fail 才是真丢帧 */
	divoom_print("btcore_uart pool heap now=%u peak=%u total=%u fail=%u\n",
		     (unsigned)s_frame_heap_now, (unsigned)s_frame_heap_peak,
		     (unsigned)s_frame_heap_total, (unsigned)s_frame_heap_fail);
}

void btcore_uart_rx_stats_dump(void)
{
	divoom_port_uart_rx_info_t rx;

	if (divoom_port_uart_rx_info(s_uart, &rx) == 0) {
		divoom_print("btcore_uart rx_ring on=%d cap=%u\n",
			     rx.buffered, (unsigned)rx.cap);
		if (rx.buffered)
			divoom_print("btcore_uart rx_ring high_water=%u drop=%u pending=%u\n",
				     (unsigned)rx.high_water, (unsigned)rx.drop,
				     (unsigned)rx.pending);
	}

	divoom_print("btcore_uart queue work=%d slow=%d\n",
		     s_rx_work_queue ? divoom_port_queue_len(s_rx_work_queue) : -1,
		     s_rx_slow_queue ? divoom_port_queue_len(s_rx_slow_queue) : -1);

	btcore_uart_frame_pool_dump();
}

static int btcore_uart_rx_frame_log_enabled(uint8_t cmd)
{
	switch (cmd) {
	case DM_BTCORE_BT_BIP_DATA:
	case DM_BTCORE_BAT_VOLT_LEVE:
	case DM_BTCORE_SEND_KEY_SYSTEM_MSG:
	case DM_BTCORE_BAT_VOLT_AVR:
	case DM_BTCORE_GET_FFT_FREQ_DATA:
	case DM_BTCORE_MIC_NOISE_METER:
	case DM_BTCORE_SD_PLAY_INFO:
		return 0;
	default:
		return 1;
	}
}

static int btcore_enqueue_parsed_cmd(void *q, uint8_t cmd, uint8_t *cmd_data,
				     uint16_t cmd_len)
{
	btcore_rx_msg_t msg;

	if (!q) {
		btcore_frame_free(cmd_data);
		return -1;
	}

	msg.cmd = cmd;
	msg.len = cmd_len;
	msg.data = cmd_data;
	if (divoom_port_queue_send(q, &msg, 0) != 0) {
		static uint32_t qfull_fail;

		if ((qfull_fail++ % 32u) == 0u)
			divoom_print("btcore_uart queue full cmd=0x%02x len=%u fail=%u\n",
				     (unsigned)cmd, (unsigned)cmd_len,
				     (unsigned)qfull_fail);
		btcore_frame_free(cmd_data);
		return -ENOMEM;
	}
	return 0;
}

/*
 * 按 app 层声明的执行上下文路由一帧。CTX_FAST 在本线程（btcore_rx）同步跑完，
 * 其余取帧池块投到对应队列，由消费者归还。
 */
static int btcore_rx_route_frame(uint8_t cmd, const uint8_t *cmd_data, uint16_t cmd_len)
{
	void *q;
	uint8_t *copy = NULL;

	/* send_wait 的同步应答优先，避免排在业务后面等到超时 */
	if (divoom_btcore_uart_cmd_try_take_sync_reply(cmd, cmd_data, cmd_len))
		return 0;

	switch (divoom_btcore_uart_cmd_ctx(cmd)) {
	case DIVOOM_BTCORE_CTX_DROP:
		return 0;

	case DIVOOM_BTCORE_CTX_STREAM:
		if (divoom_btcore_rec_stream_rx_enqueue(cmd, cmd_data, cmd_len) != 0) {
			static uint32_t stream_fail;

			/* 录音流失速时每包都打会刷屏并拖慢本线程排空，按次数抽样 */
			if ((stream_fail++ % 32u) == 0u)
				divoom_print("btcore_uart rec_stream enqueue fail cmd=0x%02x len=%u fail=%u\n",
					     (unsigned)cmd, (unsigned)cmd_len,
					     (unsigned)stream_fail);
		}
		return 0;

	case DIVOOM_BTCORE_CTX_FAST:
		divoom_btcore_uart_cmd_rx_handle_inline(cmd, cmd_data, cmd_len);
		return 0;

	case DIVOOM_BTCORE_CTX_SLOW:
		q = s_rx_slow_queue;
		break;

	case DIVOOM_BTCORE_CTX_WORK:
	default:
		q = s_rx_work_queue;
		break;
	}

	if (cmd_len > 0u) {
		copy = btcore_frame_alloc(cmd_len);
		if (!copy) {
			/* 有堆兜底后走到这里说明连堆也拿不到，此时才真丢帧，按次数抽样 */
			if ((s_frame_heap_fail % 32u) == 1u)
				divoom_print("btcore_uart frame alloc fail cmd=0x%02x len=%u pool_fail=%u heap_fail=%u\n",
					     (unsigned)cmd, (unsigned)cmd_len,
					     (unsigned)s_frame_pool_fail,
					     (unsigned)s_frame_heap_fail);
			return -ENOMEM;
		}
		memcpy(copy, cmd_data, cmd_len);
	}

	return btcore_enqueue_parsed_cmd(q, cmd, copy, cmd_len);
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

/* --- Layer 2: ring bytes → FSM → work queue (runs on btcore_rx) --- */

static int btcore_rx_enqueue_parsed_frame(const uint8_t *frame, int frame_len);

static void btcore_rx_fsm_reset(btcore_rx_fsm_t *fsm)
{
	fsm->state = BTCORE_FSM_HUNT;
	fsm->need = 0;
	fsm->pos = 0;
}

static void btcore_rx_fsm_drop_first(btcore_rx_fsm_t *fsm)
{
	if (fsm->pos <= 1) {
		btcore_rx_fsm_reset(fsm);
		return;
	}

	memmove(fsm->frame, fsm->frame + 1, fsm->pos - 1);
	fsm->pos--;
	fsm->need = 0;
	if (fsm->frame[0] != BTCORE_HEADER) {
		btcore_rx_fsm_reset(fsm);
		return;
	}
	fsm->state = BTCORE_FSM_COLLECT;
}

static int btcore_rx_fsm_update_need(btcore_rx_fsm_t *fsm)
{
	uint16_t L;

	if (fsm->pos < 4)
		return 0;

	L = (uint16_t)fsm->frame[1] | ((uint16_t)fsm->frame[2] << 8);
	if (L < 2 || L > BTCORE_MAX_L_BODY) {
		if (L > BTCORE_MAX_L_BODY && fsm->frame[3] == 0x18) {
			divoom_print("btcore_uart drop 0x18 frame L=%u > max=%u\n",
				     (unsigned)L, (unsigned)BTCORE_MAX_L_BODY);
		}
		return -1;
	}

	fsm->need = (uint16_t)(5 + L);
	if (fsm->need > BTCORE_MAX_FRAME_SIZE)
		return -1;

	return 0;
}

static void btcore_rx_fsm_try_complete_frame(btcore_rx_fsm_t *fsm)
{
	for (;;) {
		if (fsm->state != BTCORE_FSM_COLLECT)
			return;

		if (fsm->need == 0) {
			if (btcore_rx_fsm_update_need(fsm) != 0) {
				btcore_rx_fsm_drop_first(fsm);
				continue;
			}
			if (fsm->need == 0)
				return;
		}

		if (fsm->pos < fsm->need)
			return;

		if (fsm->frame[fsm->need - 1] != BTCORE_TAIL) {
			btcore_rx_fsm_drop_first(fsm);
			continue;
		}

		if (btcore_rx_enqueue_parsed_frame(fsm->frame, (int)fsm->need) != 0) {
			btcore_rx_fsm_drop_first(fsm);
			continue;
		}

		btcore_rx_fsm_reset(fsm);
		return;
	}
}

static void btcore_rx_fsm_feed_byte(btcore_rx_fsm_t *fsm, uint8_t b)
{
	if (fsm->state == BTCORE_FSM_HUNT) {
		if (b != BTCORE_HEADER)
			return;
		fsm->frame[0] = b;
		fsm->pos = 1;
		fsm->need = 0;
		fsm->state = BTCORE_FSM_COLLECT;
		return;
	}

	if (fsm->pos >= BTCORE_MAX_FRAME_SIZE) {
		btcore_rx_fsm_reset(fsm);
		return;
	}

	fsm->frame[fsm->pos++] = b;
	btcore_rx_fsm_try_complete_frame(fsm);
}

/*
 * 与逐字节 btcore_rx_fsm_feed_byte 行为等价，只是在 need 已知后一次搬完整段：
 * pos < need 期间 try_complete 恒为空转，省略这些调用不改变结果。
 * need 未知（pos < 4）、帧头搜索命中、以及 pos 溢出这三种边界仍走 feed_byte，
 * 使 drop_first 重同步语义保持一致。
 */
static void btcore_rx_fsm_feed_block(btcore_rx_fsm_t *fsm, const uint8_t *buf, uint32_t len)
{
	const uint8_t *p = buf;
	uint32_t remain = len;

	while (remain > 0u) {
		uint32_t want, n;

		if (fsm->state == BTCORE_FSM_HUNT) {
			const uint8_t *hdr = (const uint8_t *)memchr(p, BTCORE_HEADER, remain);

			if (!hdr)
				return;
			remain -= (uint32_t)(hdr - p);
			p = hdr;
			btcore_rx_fsm_feed_byte(fsm, *p++);
			remain--;
			continue;
		}

		/* 任何无法确定正剩余长度的情形都回落逐字节，保证 remain 必然递减 */
		if (fsm->need == 0u || fsm->need <= fsm->pos ||
		    fsm->pos >= BTCORE_MAX_FRAME_SIZE) {
			btcore_rx_fsm_feed_byte(fsm, *p++);
			remain--;
			continue;
		}

		want = (uint32_t)(fsm->need - fsm->pos);
		n = (want > remain) ? remain : want;

		memcpy(fsm->frame + fsm->pos, p, n);
		fsm->pos = (uint16_t)(fsm->pos + n);
		p += n;
		remain -= n;

		btcore_rx_fsm_try_complete_frame(fsm);
	}
}

/*
 * 排空 port UART 接收缓冲并喂 FSM。返回 <0 表示读失败需退出线程。
 * 一次没读满说明已空，不必再多调一次读。
 */
static int btcore_rx_pump(void)
{
	int n;

	for (;;) {
		n = divoom_port_uart_read(s_uart, s_rx_chunk, sizeof(s_rx_chunk), 0);
		if (n < 0)
			return -1;
		if (n == 0)
			break;
		btcore_rx_fsm_feed_block(&s_rx_fsm, s_rx_chunk, (uint32_t)n);
		if ((size_t)n < sizeof(s_rx_chunk))
			break;
	}

	return 0;
}

static int btcore_rx_enqueue_parsed_frame(const uint8_t *frame, int frame_len)
{
	uint8_t cmd;
	const uint8_t *cmd_data;
	uint16_t cmd_len;
	int r;

	r = btcore_uart_parse_frame(frame, frame_len, &cmd, &cmd_data, &cmd_len);
	if (r != 0)
		return r;

#ifdef DIVOOM_TIMES_VIBE
	if (divoom_btcore_uart_cmd_is_rx_suppressed())
		return 0;
#endif

	if (btcore_uart_rx_frame_log_enabled(cmd))
		btcore_uart_log_frame("rx", frame, frame_len, cmd, cmd_len);

	return btcore_rx_route_frame(cmd, cmd_data, cmd_len);
}

/* --- Layer 3: 命令队列 → business（btcore_work 与 btcore_slow 共用同一循环） --- */

static void btcore_work_dispatch_msg(const btcore_rx_msg_t *msg)
{
	if (!msg || msg->cmd == BTCORE_RX_CMD_POISON)
		return;
	if (!s_rx_cb)
		return;
	if (msg->len > 0u)
		s_rx_cb(msg->cmd, msg->data, msg->len);
	else
		s_rx_cb(msg->cmd, NULL, 0);
}

/*
 * 两条消费线程的唯一区别是队列与优先级：work 跑轻量业务与 UI 状态机，
 * slow 跑文件 IO 与 DB，后者被拆出来就是为了不让一次落盘堵住前者。
 */
static void btcore_msg_thread_loop(void **q, bool *running, bool *done,
				   const char *name)
{
	divoom_print("btcore_uart %s thread enter\n", name);

	while (*running) {
		btcore_rx_msg_t msg;

		if (divoom_port_queue_recv(*q, &msg, 50) != 0)
			continue;
		if (msg.cmd == BTCORE_RX_CMD_POISON)
			break;
		btcore_work_dispatch_msg(&msg);
		btcore_frame_free(msg.data);
	}

	while (divoom_port_queue_len(*q) > 0) {
		btcore_rx_msg_t msg;

		if (divoom_port_queue_recv(*q, &msg, 0) != 0)
			break;
		if (msg.cmd == BTCORE_RX_CMD_POISON)
			continue;
		btcore_work_dispatch_msg(&msg);
		btcore_frame_free(msg.data);
	}

	divoom_print("btcore_uart %s thread exit\n", name);
	*done = true;
}

static void *btcore_work_thread_main(void *arg)
{
	(void)arg;
	btcore_msg_thread_loop(&s_rx_work_queue, &s_work_thread_running,
			       &s_work_thread_done, "work");
	return NULL;
}

static void *btcore_slow_thread_main(void *arg)
{
	(void)arg;
	divoom_print("btcore_uart slow thread enter\n");

	while (s_slow_thread_running) {
		btcore_rx_msg_t msg;

		if (divoom_port_queue_recv(s_rx_slow_queue, &msg, 50) != 0) {
			divoom_btcore_file_rx_poll();
			continue;
		}
		if (msg.cmd == BTCORE_RX_CMD_POISON)
			break;
		btcore_work_dispatch_msg(&msg);
		btcore_frame_free(msg.data);
	}

	while (divoom_port_queue_len(s_rx_slow_queue) > 0) {
		btcore_rx_msg_t msg;

		if (divoom_port_queue_recv(s_rx_slow_queue, &msg, 0) != 0)
			break;
		if (msg.cmd == BTCORE_RX_CMD_POISON)
			continue;
		btcore_work_dispatch_msg(&msg);
		btcore_frame_free(msg.data);
	}

	divoom_print("btcore_uart slow thread exit\n");
	s_slow_thread_done = true;
	return NULL;
}

/*
 * 帧池余量上报。转堆兜底时打全量摘要；没转堆时按 peak 每涨 1/4 容量报一行，
 * 这样容量够不够不必等到出问题才知道。与环水位同点调用，热路径无开销。
 */
static void btcore_frame_pool_report(void)
{
	static uint32_t reported_fail;
	static uint32_t reported_heap;
	static uint32_t reported_peak[BTCORE_FRAME_POOL_LEVELS];
	int i;

	/*
	 * heap_total 由 0 变非 0 是最重要的信号：容量不足已被兜住，
	 * 但热路径开始做 malloc，须当成待调容量看，不能当正常状态。
	 */
	if (s_frame_pool_fail != reported_fail || s_frame_heap_total != reported_heap) {
		uint32_t fail_delta = s_frame_pool_fail - reported_fail;

		reported_fail = s_frame_pool_fail;
		reported_heap = s_frame_heap_total;
		/* 池耗尽时每帧 dump 会拖死 rx；按次数抽样 */
		if (fail_delta == 0u || (s_frame_pool_fail % 32u) == 0u ||
		    s_frame_pool_fail <= 2u)
			btcore_uart_frame_pool_dump();
		for (i = 0; i < BTCORE_FRAME_POOL_LEVELS; i++)
			reported_peak[i] = s_frame_pools[i].peak;
		return;
	}

	for (i = 0; i < BTCORE_FRAME_POOL_LEVELS; i++) {
		btcore_frame_pool_t *p = &s_frame_pools[i];
		uint32_t step = p->blk_count / 4u;

		if (step == 0u)
			step = 1u;
		if (p->peak >= reported_peak[i] + step) {
			divoom_print("btcore_uart pool[%d] size=%u peak=%u/%u starve=%u\n",
				     i, (unsigned)p->blk_size, (unsigned)p->peak,
				     (unsigned)p->blk_count, (unsigned)p->starve);
			reported_peak[i] = p->peak;
		}
	}
}

/*
 * 环水位与丢弃量上报。只在环已排空（即刚扛过一轮抢占）时调用，热路径无开销；
 * 按容量 1/8 步进抬高报告门槛，最多打印数条即到顶，不会刷屏。
 */
static void btcore_rx_ring_report_watermark(void)
{
	static uint32_t reported_hw;
	static uint32_t reported_drop;
	divoom_port_uart_rx_info_t rx;
	uint32_t step;

	if (divoom_port_uart_rx_info(s_uart, &rx) != 0 || !rx.buffered)
		return;

	if (rx.drop != reported_drop) {
		divoom_print("btcore_uart rx ring DROP=%u high_water=%u/%u\n",
			     (unsigned)rx.drop, (unsigned)rx.high_water,
			     (unsigned)rx.cap);
		reported_drop = rx.drop;
		reported_hw = rx.high_water;
		return;
	}
	step = rx.cap / 8u;
	if (step == 0u)
		step = 1u;
	if (rx.high_water > reported_hw + step) {
		divoom_print("btcore_uart rx ring high_water=%u/%u\n",
			     (unsigned)rx.high_water, (unsigned)rx.cap);
		reported_hw = rx.high_water;
	}
}

static void *btcore_rx_thread_main(void *arg)
{
	int n;
	(void)arg;

	divoom_print("btcore_uart rx thread enter, port=%d\n", s_uart);

	while (s_thread_running) {
		if (!s_uart_open) {
			divoom_print("btcore_uart uart closed, exit thread\n");
			break;
		}

		if (btcore_rx_pump() < 0) {
			divoom_print("btcore_uart read err\n");
			break;
		}

		btcore_rx_ring_report_watermark();
		btcore_frame_pool_report();

		/* 缓冲已空，睡到下一批。mailbox 时 port 内部最多等 1 字节 */
		n = divoom_port_uart_read(s_uart, s_rx_chunk, sizeof(s_rx_chunk),
					  BTCORE_RX_IDLE_WAIT_MS);
		if (n < 0) {
			divoom_print("btcore_uart read err\n");
			break;
		}
		if (n > 0)
			btcore_rx_fsm_feed_block(&s_rx_fsm, s_rx_chunk, (uint32_t)n);
	}
	divoom_print("btcore_uart rx parse thread exit\n");
	s_rx_thread_done = true;
	return NULL;
}

int btcore_uart_is_ready(void)
{
	return s_inited && s_uart_open && s_thread_running;
}

/* 给两条消费线程各发一个毒丸并等其退出；未创建成功的线程 done 已置位，不会白等 */
static void btcore_stop_msg_threads(void)
{
	btcore_rx_msg_t poison;
	int i;

	memset(&poison, 0, sizeof(poison));
	poison.cmd = BTCORE_RX_CMD_POISON;

	s_work_thread_running = false;
	s_slow_thread_running = false;
	if (s_rx_work_queue)
		divoom_port_queue_send(s_rx_work_queue, &poison, 100);
	if (s_rx_slow_queue)
		divoom_port_queue_send(s_rx_slow_queue, &poison, 100);

	for (i = 0; i < 200 && !(s_work_thread_done && s_slow_thread_done); i++)
		divoom_system_delay_ms(10);
	if (!s_work_thread_done)
		divoom_print("btcore_uart work thread exit timeout\n");
	if (!s_slow_thread_done)
		divoom_print("btcore_uart slow thread exit timeout\n");
}

/*
 * 队列里的 data 指向帧池固定槽位，残留项必须先归还再删队列：池不像堆，
 * 丢掉的槽位不会被回收，且 btcore_frame_pool_init 有 ready 标志不会重建链表。
 */
static void btcore_free_msg_queue(void **q, const char *name)
{
	btcore_rx_msg_t msg;
	int dropped = 0;

	if (!*q)
		return;

	while (divoom_port_queue_recv(*q, &msg, 0) == 0) {
		if (msg.cmd != BTCORE_RX_CMD_POISON)
			dropped++;
		btcore_frame_free(msg.data);
	}
	if (dropped)
		divoom_print("btcore_uart %s queue dropped %d msg on deinit\n", name, dropped);

	divoom_port_queue_destroy(*q);
	*q = NULL;
}

int btcore_uart_init(const btcore_uart_config_t *cfg)
{
	int baud = BTCORE_UART_BAUD_DEFAULT;

	if (s_inited)
		return 0;
	if (cfg && cfg->baudrate > 0)
		baud = cfg->baudrate;

	btcore_frame_pool_init();

	s_tx_mutex = divoom_port_mutex_create();
	if (!s_tx_mutex) {
		divoom_print("btcore_uart create tx mutex fail\n");
		return -1;
	}

	if (divoom_port_uart_open(s_uart, baud) != 0) {
		divoom_print("btcore_uart open port=%d baud=%d fail\n", s_uart, baud);
		divoom_port_mutex_destroy(s_tx_mutex);
		s_tx_mutex = NULL;
		return -1;
	}
	s_uart_open = true;

	btcore_rx_fsm_reset(&s_rx_fsm);

	s_rx_work_queue = divoom_port_queue_create("btcore_rx_work", sizeof(btcore_rx_msg_t),
						   BTCORE_RX_WORK_QUEUE_DEPTH);
	s_rx_slow_queue = divoom_port_queue_create("btcore_rx_slow", sizeof(btcore_rx_msg_t),
						   BTCORE_RX_SLOW_QUEUE_DEPTH);
	if (!s_rx_work_queue || !s_rx_slow_queue) {
		divoom_print("btcore_uart create cmd queue fail\n");
		btcore_free_msg_queue(&s_rx_work_queue, "work");
		btcore_free_msg_queue(&s_rx_slow_queue, "slow");
		divoom_port_uart_close(s_uart);
		s_uart_open = false;
		divoom_port_mutex_destroy(s_tx_mutex);
		s_tx_mutex = NULL;
		return -1;
	}

	s_work_thread_running = true;
	s_work_thread_done = false;
	if (!divoom_system_create_task_prio(btcore_work_thread_main, "btcore_work", NULL,
					    DIVOOM_TASK_STACK_BTCORE_UART_WORK,
					    BTCORE_RX_WORK_TASK_PRIO)) {
		divoom_print("btcore_uart create work task fail\n");
		s_work_thread_running = false;
		s_work_thread_done = true;
		s_slow_thread_done = true;
		goto fail_threads;
	}

	s_slow_thread_running = true;
	s_slow_thread_done = false;
	if (!divoom_system_create_task_prio(btcore_slow_thread_main, "btcore_slow", NULL,
					    DIVOOM_TASK_STACK_BTCORE_UART_SLOW,
					    BTCORE_RX_SLOW_TASK_PRIO)) {
		divoom_print("btcore_uart create slow task fail\n");
		s_slow_thread_running = false;
		s_slow_thread_done = true;
		goto fail_threads;
	}

	s_thread_running = true;
	s_rx_thread_done = false;
	if (!divoom_system_create_task_prio(btcore_rx_thread_main, "btcore_rx", NULL,
					    DIVOOM_TASK_STACK_BTCORE_UART_RX,
					    BTCORE_RX_PARSE_TASK_PRIO)) {
		divoom_print("btcore_uart create rx task fail\n");
		s_thread_running = false;
		goto fail_threads;
	}

	s_inited = true;
	divoom_system_delay_ms(10);
	if (s_rx_thread_done) {
		divoom_print("btcore_uart rx task exited early\n");
		btcore_uart_deinit();
		return -1;
	}
	{
		divoom_port_uart_rx_info_t rx;

		if (divoom_port_uart_rx_info(s_uart, &rx) != 0)
			memset(&rx, 0, sizeof(rx));
		divoom_print("btcore_uart init ok port=%d baud=%d rx_ring=%d (pump|work|slow)\n",
			     s_uart, baud, rx.buffered);
	}
	return 0;

fail_threads:
	btcore_stop_msg_threads();
	btcore_free_msg_queue(&s_rx_work_queue, "work");
	btcore_free_msg_queue(&s_rx_slow_queue, "slow");
	divoom_port_uart_close(s_uart);
	s_uart_open = false;
	divoom_port_mutex_destroy(s_tx_mutex);
	s_tx_mutex = NULL;
	return -1;
}

void btcore_uart_deinit(void)
{
	if (!s_inited)
		return;
	s_thread_running = false;
	for (int i = 0; i < 200 && !s_rx_thread_done; i++)
		divoom_system_delay_ms(10);
	if (!s_rx_thread_done)
		divoom_print("btcore_uart rx thread exit timeout\n");

	btcore_stop_msg_threads();
	divoom_port_uart_close(s_uart);
	s_uart_open = false;
	btcore_free_msg_queue(&s_rx_work_queue, "work");
	btcore_free_msg_queue(&s_rx_slow_queue, "slow");
	divoom_port_mutex_destroy(s_tx_mutex);
	s_tx_mutex = NULL;
	s_inited = false;
}

int btcore_uart_send(uint8_t cmd, const uint8_t *cmd_data, uint16_t cmd_len)
{
	uint8_t *frame;
	uint16_t L;
	uint16_t chk;
	size_t total;

	if (!s_inited || !s_uart_open)
		return -EINVAL;
	if (cmd_len > BTCORE_MAX_CMD_DATA_LEN)
		return -EINVAL;
    if(s_send_uart_buf == NULL)
    {
        s_send_uart_len = 7 + BTCORE_MAX_CMD_DATA_LEN;
        s_send_uart_buf = divoom_system_malloc(s_send_uart_len);
    }
    if(s_send_uart_len < cmd_len)
    {
        s_send_uart_len = cmd_len + 7;
        divoom_system_free(s_send_uart_buf);
        s_send_uart_buf = divoom_system_malloc(s_send_uart_len);
        
    }
    frame  = s_send_uart_buf;
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

	divoom_port_mutex_lock(s_tx_mutex);
	(void)divoom_port_uart_write(s_uart, frame, total);
	divoom_port_mutex_unlock(s_tx_mutex);
	return 0;
}

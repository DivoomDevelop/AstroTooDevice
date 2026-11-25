/*
 * BtCore UART 帧组包：字节流 → 0xAA 完整帧，帧格式见 btcore_uart.h。
 * 对标 divoom_app_protocol_AcceptData(buf, size) 批量喂入；仅 btcore_rx 线程调用。
 */
#include "divoom_btcore_uart_parse.h"
#include "btcore_uart.h"
#include "divoom.h"

#if !DIVOOM_BUILD_RTOS_DEVICE
#error "divoom_btcore_uart_parse.c is RTOS device-only."
#endif

#include <string.h>

#define DIVOOM_BTCORE_UART_PARSE_HEADER  0xAAu
#define DIVOOM_BTCORE_UART_PARSE_TAIL    0xBBu
#define DIVOOM_BTCORE_UART_PARSE_MAX_FRAME_SIZE  (7u + BTCORE_MAX_CMD_DATA_LEN)
#define DIVOOM_BTCORE_UART_PARSE_MAX_L_BODY      (BTCORE_MAX_CMD_DATA_LEN + 2u)
#define DIVOOM_BTCORE_UART_PARSE_DRAIN_CHUNK     512u

typedef enum {
	DIVOOM_BTCORE_UART_PARSE_HUNT,
	DIVOOM_BTCORE_UART_PARSE_COLLECT,
} divoom_btcore_uart_parse_state_t;

typedef struct {
	divoom_btcore_uart_parse_state_t state;
	uint16_t                         need;
	uint16_t                         pos;
	uint8_t                          frame[DIVOOM_BTCORE_UART_PARSE_MAX_FRAME_SIZE];
} divoom_btcore_uart_parse_ctx_t;

static divoom_btcore_uart_parse_ctx_t      s_parse_ctx;
static divoom_btcore_uart_parse_on_frame_fn s_on_frame;
static void                               *s_on_frame_ctx;
/* btcore_rx thread only */
static uint8_t                             s_drain_chunk[DIVOOM_BTCORE_UART_PARSE_DRAIN_CHUNK];

static void divoom_btcore_uart_parse_fsm_reset(divoom_btcore_uart_parse_ctx_t *ctx)
{
	ctx->state = DIVOOM_BTCORE_UART_PARSE_HUNT;
	ctx->need = 0;
	ctx->pos = 0;
}

static void divoom_btcore_uart_parse_fsm_drop_first(divoom_btcore_uart_parse_ctx_t *ctx)
{
	if (ctx->pos <= 1) {
		divoom_btcore_uart_parse_fsm_reset(ctx);
		return;
	}

	memmove(ctx->frame, ctx->frame + 1, ctx->pos - 1);
	ctx->pos--;
	ctx->need = 0;
	if (ctx->frame[0] != DIVOOM_BTCORE_UART_PARSE_HEADER) {
		divoom_btcore_uart_parse_fsm_reset(ctx);
		return;
	}
	ctx->state = DIVOOM_BTCORE_UART_PARSE_COLLECT;
}

static int divoom_btcore_uart_parse_fsm_update_need(divoom_btcore_uart_parse_ctx_t *ctx)
{
	uint16_t L;

	if (ctx->pos < 4)
		return 0;

	L = (uint16_t)ctx->frame[1] | ((uint16_t)ctx->frame[2] << 8);
	if (L < 2 || L > DIVOOM_BTCORE_UART_PARSE_MAX_L_BODY) {
		if (L > DIVOOM_BTCORE_UART_PARSE_MAX_L_BODY && ctx->frame[3] == 0x18) {
			divoom_print("btcore_uart drop 0x18 frame L=%u > max=%u\n",
				     (unsigned)L, (unsigned)DIVOOM_BTCORE_UART_PARSE_MAX_L_BODY);
		}
		return -1;
	}

	ctx->need = (uint16_t)(5 + L);
	if (ctx->need > DIVOOM_BTCORE_UART_PARSE_MAX_FRAME_SIZE)
		return -1;

	return 0;
}

static void divoom_btcore_uart_parse_fsm_try_complete(divoom_btcore_uart_parse_ctx_t *ctx)
{
	for (;;) {
		if (ctx->state != DIVOOM_BTCORE_UART_PARSE_COLLECT)
			return;

		if (ctx->need == 0) {
			if (divoom_btcore_uart_parse_fsm_update_need(ctx) != 0) {
				divoom_btcore_uart_parse_fsm_drop_first(ctx);
				continue;
			}
			if (ctx->need == 0)
				return;
		}

		if (ctx->pos < ctx->need)
			return;

		if (ctx->frame[ctx->need - 1] != DIVOOM_BTCORE_UART_PARSE_TAIL) {
			divoom_btcore_uart_parse_fsm_drop_first(ctx);
			continue;
		}

		if (s_on_frame && s_on_frame(ctx->frame, (int)ctx->need, s_on_frame_ctx) != 0) {
			divoom_btcore_uart_parse_fsm_drop_first(ctx);
			continue;
		}

		divoom_btcore_uart_parse_fsm_reset(ctx);
		return;
	}
}

static void divoom_btcore_uart_parse_fsm_feed_byte(divoom_btcore_uart_parse_ctx_t *ctx, uint8_t b)
{
	if (ctx->state == DIVOOM_BTCORE_UART_PARSE_HUNT) {
		if (b != DIVOOM_BTCORE_UART_PARSE_HEADER)
			return;
		ctx->frame[0] = b;
		ctx->pos = 1;
		ctx->need = 0;
		ctx->state = DIVOOM_BTCORE_UART_PARSE_COLLECT;
		return;
	}

	if (ctx->pos >= DIVOOM_BTCORE_UART_PARSE_MAX_FRAME_SIZE) {
		divoom_btcore_uart_parse_fsm_reset(ctx);
		return;
	}

	ctx->frame[ctx->pos++] = b;
	divoom_btcore_uart_parse_fsm_try_complete(ctx);
}

void divoom_btcore_uart_parse_init(divoom_btcore_uart_parse_on_frame_fn on_frame, void *ctx)
{
	s_on_frame = on_frame;
	s_on_frame_ctx = ctx;
	divoom_btcore_uart_parse_reset();
}

void divoom_btcore_uart_parse_reset(void)
{
	divoom_btcore_uart_parse_fsm_reset(&s_parse_ctx);
}

void divoom_btcore_uart_parse_feed(const uint8_t *buf, uint32_t size)
{
	uint32_t i;

	if (!buf || size == 0)
		return;

	for (i = 0; i < size; i++)
		divoom_btcore_uart_parse_fsm_feed_byte(&s_parse_ctx, buf[i]);
}

void divoom_btcore_uart_parse_drain_ring(void *ring)
{
	uint32_t avail, got;

	if (!ring)
		return;

	for (;;) {
		avail = divoom_ring_buffer_getdatasize(ring);
		if (avail == 0)
			break;
		if (avail > DIVOOM_BTCORE_UART_PARSE_DRAIN_CHUNK)
			avail = DIVOOM_BTCORE_UART_PARSE_DRAIN_CHUNK;
		got = divoom_ring_buffer_get(ring, s_drain_chunk, avail);
		divoom_btcore_uart_parse_feed(s_drain_chunk, got);
	}
}

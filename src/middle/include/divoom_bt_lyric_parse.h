#ifndef _DIVOOM_BT_LYRIC_PARSE_H_
#define _DIVOOM_BT_LYRIC_PARSE_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DIVOOM_BT_LYRIC_STR_LEN 256
#define DIVOOM_BT_LYRIC_BURST_MS 300u
#define DIVOOM_BT_LYRIC_HOLD_MS 300u

/*
 * 组合 type2 要不要拆成「歌名 + 歌手」。只改本宏，不必 menuconfig。
 *
 * 1（当前默认）：type2 已是组合串时，整段写入歌名栏，本突发 type1 当歌词，
 *    不拆左右。无分隔符的 type2 仍是歌手（Spotify / 关歌词 / 合作歌手）。
 *    组合分隔符：` · ` / `•` / ` — ` / `--` / 空格 ` - `。
 *    裸 `-` 左右含汉字时整段当标题（含中途连上：type1 已是歌词）。
 *    `350Ton-Xll！` 仍当歌手。裸 `/` 不走本档。
 * 0：组合串按原规则拆开并锚定左右（酷狗对调、汽水对调、波点/汽水 —）。
 *    鸿蒙酷狗 type2 `歌手-歌名`（裸 `-`）按成对拆：左歌手右歌名。
 */
#ifndef DIVOOM_BT_LYRIC_COMBINED_AS_TITLE
#define DIVOOM_BT_LYRIC_COMBINED_AS_TITLE 1
#endif

typedef enum {
	DIVOOM_BT_LYRIC_EV_TYPE1 = 0,
	DIVOOM_BT_LYRIC_EV_TYPE2,
	DIVOOM_BT_LYRIC_EV_TOTAL,
	DIVOOM_BT_LYRIC_EV_POS,
	DIVOOM_BT_LYRIC_EV_TICK
} divoom_bt_lyric_ev;

typedef struct {
	char title[DIVOOM_BT_LYRIC_STR_LEN];
	char artist[DIVOOM_BT_LYRIC_STR_LEN];
	char lyric[DIVOOM_BT_LYRIC_STR_LEN];
	char burst_type1[DIVOOM_BT_LYRIC_STR_LEN];
	char last_type2[DIVOOM_BT_LYRIC_STR_LEN];
	char amb_first_type1[DIVOOM_BT_LYRIC_STR_LEN];
	char hold_lyric[DIVOOM_BT_LYRIC_STR_LEN];
	uint32_t burst_type1_ms;
	uint32_t hold_ms;
	uint32_t song_len;
	uint32_t play_pos;
	uint8_t burst_type1_valid;
	uint8_t combined;
	uint8_t lyric_phase;
	uint8_t uart_lyric_valid;
	uint8_t hold_active;
	uint8_t amb_kind; /* 0=none, 1=bare slash, 2=bare dash */
} divoom_bt_lyric_ctx;

typedef struct {
	const char *title;  /* → ctx，feed 返回前有效 */
	const char *artist;
	const char *lyric;
	uint8_t title_changed;
	uint8_t artist_changed;
	uint8_t lyric_changed;
	uint8_t song_changed;
	uint8_t uart_lyric_valid;
	const char *trace;
} divoom_bt_lyric_out;

void divoom_bt_lyric_parse_reset(divoom_bt_lyric_ctx *ctx);
void divoom_bt_lyric_parse_feed(divoom_bt_lyric_ctx *ctx, divoom_bt_lyric_ev ev,
	const char *text, uint32_t num, uint32_t now_ms, divoom_bt_lyric_out *out);

#ifdef __cplusplus
}
#endif

#endif /* _DIVOOM_BT_LYRIC_PARSE_H_ */

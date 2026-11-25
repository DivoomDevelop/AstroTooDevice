#ifndef DIVOOM_BTCORE_KEY_LIGHT_H
#define DIVOOM_BTCORE_KEY_LIGHT_H

#include "divoom.h"

#ifdef DIVOOM_TIMES_VIBE

#ifdef __cplusplus
extern "C" {
#endif

#define DIVOOM_BTCORE_KEY_LIGHT_BRIGHT_MAX		100u
#define DIVOOM_BTCORE_KEY_LIGHT_BRIGHT_DEFAULT		100u

/* JL divoom_key_light_ctrl：与 DM_BTCORE_KEY_LIGHT_CTRL mode 字段一致 */
typedef enum {
	DIVOOM_BTCORE_KEY_LIGHT_MODE_WINDMILL = 0,		/* 风车；FFT 调速 */
	DIVOOM_BTCORE_KEY_LIGHT_MODE_BREATHE,			/* 淡进淡出 */
	DIVOOM_BTCORE_KEY_LIGHT_MODE_FLOW_WINDMILL,		/* 流动风车；FFT 调速 */
	DIVOOM_BTCORE_KEY_LIGHT_MODE_STARS_BREATHE,		/* 星星闪烁 */
	DIVOOM_BTCORE_KEY_LIGHT_MODE_RAINBOW,			/* 彩虹 */
	DIVOOM_BTCORE_KEY_LIGHT_MODE_FAST_RUN,			/* 快跑 */
	DIVOOM_BTCORE_KEY_LIGHT_MODE_FIREWORM,			/* 萤火虫 */
	DIVOOM_BTCORE_KEY_LIGHT_MODE_WHITE,			/* 纯白常亮 */
	DIVOOM_BTCORE_KEY_LIGHT_MODE_MAX,
} divoom_btcore_key_light_mode_t;

/* 出厂/无配置默认：星星闪烁。非法 mode 回退仍用 WINDMILL（枚举首项） */
#define DIVOOM_BTCORE_KEY_LIGHT_MODE_DEFAULT		DIVOOM_BTCORE_KEY_LIGHT_MODE_STARS_BREATHE

/* 产测：与 LCD 白/红/绿/蓝屏测同步，合法区间 [0x80, 0x84) */
#define DIVOOM_BTCORE_KEY_LIGHT_MODE_FACTORY_START	0x80u
#define DIVOOM_BTCORE_KEY_LIGHT_MODE_FACTORY_CNT	4u
#define DIVOOM_BTCORE_KEY_LIGHT_MODE_FACTORY_END \
	(DIVOOM_BTCORE_KEY_LIGHT_MODE_FACTORY_START + DIVOOM_BTCORE_KEY_LIGHT_MODE_FACTORY_CNT)

#define DIVOOM_BTCORE_KEY_LIGHT_MODE_FACTORY_WHITE \
	(DIVOOM_BTCORE_KEY_LIGHT_MODE_FACTORY_START + 0u)
#define DIVOOM_BTCORE_KEY_LIGHT_MODE_FACTORY_RED \
	(DIVOOM_BTCORE_KEY_LIGHT_MODE_FACTORY_START + 1u)
#define DIVOOM_BTCORE_KEY_LIGHT_MODE_FACTORY_GREEN \
	(DIVOOM_BTCORE_KEY_LIGHT_MODE_FACTORY_START + 2u)
#define DIVOOM_BTCORE_KEY_LIGHT_MODE_FACTORY_BLUE \
	(DIVOOM_BTCORE_KEY_LIGHT_MODE_FACTORY_START + 3u)

/* 与杰理 DIVOOM_KEY_LIGHT_MODE_SCREEN_OFF 对齐；不进设置页 IS_VALID */
#define DIVOOM_BTCORE_KEY_LIGHT_MODE_SCREEN_OFF	0x87u

#define DIVOOM_BTCORE_KEY_LIGHT_MODE_IS_FACTORY(mode) \
	((uint8_t)(mode) >= (uint8_t)DIVOOM_BTCORE_KEY_LIGHT_MODE_FACTORY_START && \
	 (uint8_t)(mode) < (uint8_t)DIVOOM_BTCORE_KEY_LIGHT_MODE_FACTORY_END)

#define DIVOOM_BTCORE_KEY_LIGHT_MODE_IS_VALID(mode) \
	(((uint8_t)(mode) < (uint8_t)DIVOOM_BTCORE_KEY_LIGHT_MODE_MAX) || \
	 DIVOOM_BTCORE_KEY_LIGHT_MODE_IS_FACTORY(mode))

/* 下发 [1, mode, on_off]；不改 JL 侧亮度 */
int divoom_btcore_key_light_set(uint8_t mode, uint8_t on_off);

/* 下发 [1, mode, on_off, bright] */
int divoom_btcore_key_light_set_full(uint8_t mode, uint8_t on_off, uint8_t bright);

/* 保持 mode/on_off，仅改亮度 */
int divoom_btcore_key_light_set_brightness(uint8_t bright);

/* 关灯 [1, cached_mode, 0] */
int divoom_btcore_key_light_off(void);

/* 熄屏发 SCREEN_OFF，亮屏按缓存用户灯效退出；不改用户偏好 */
void divoom_btcore_key_light_follow_screen(uint8_t screen_on);

/* 查询 JL [0] → [mode, on_off, bright]（仅 CLI 联调，开机不用） */
int divoom_btcore_key_light_sync_from_jl(void);

uint8_t divoom_btcore_key_light_get_mode(void);
uint8_t divoom_btcore_key_light_get_on_off(void);
uint8_t divoom_btcore_key_light_get_brightness(void);

#ifdef __cplusplus
}
#endif

#endif /* DIVOOM_TIMES_VIBE */
#endif /* DIVOOM_BTCORE_KEY_LIGHT_H */

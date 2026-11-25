#ifndef DIVOOM_BTCORE_KEY_H
#define DIVOOM_BTCORE_KEY_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 0x4c payload 与 JL divoom_main_key_deal_main 一致：
 *   [0] msg.value — key_driver 硬件索引（KEY_AD_NUM* / KEY_AD2_NUM*，见 JL key_driver.h）
 *   [1] msg.event — KEY_ACTION 0–4（CLICK/LONG/HOLD/UP/DOUBLE_CLICK）
 * type 字段不再转发，各 value 全局唯一。
 */
#define DIVOOM_BTCORE_HW_KEY_POWER 10u /* JL KEY_AD_NUM0 */

/* hardware value + action → DIVOOM_KEY_ASTROTOO_KEY_*；0xFF = 忽略 */
uint8_t divoom_btcore_key_map(uint8_t key_value, uint8_t key_action);

/* action → DIVOOM_PRESS_* */
uint16_t divoom_btcore_press_map(uint8_t key_action);

/* UP/DOWN/LEFT/RIGHT/ENTER/ESC → LVGL；其余 → main */
int divoom_btcore_key_is_lvgl_nav(uint8_t ui_key);

/* 关屏仍响应：音量±、播放/暂停、上一曲/下一曲（ui_key 为 DIVOOM_KEY_ASTROTOO_KEY_*） */
int divoom_btcore_key_allowed_when_screen_off(uint8_t ui_key);

#ifdef __cplusplus
}
#endif

#endif

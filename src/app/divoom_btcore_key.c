#include "divoom.h"

#ifdef DIVOOM_TIMES_VIBE
#include "divoom_btcore_key.h"
#include "divoom_btcore_uart_cmd.h"
#include "divoom_user_key.h"


static int btcore_is_vol_short_action(uint8_t action)
{
	return (action == DIVOOM_BTCORE_KEY_ACTION_CLICK
		|| action == DIVOOM_BTCORE_KEY_ACTION_DOUBLE_CLICK) ? 1 : 0;
}

static int btcore_is_lig_action(uint8_t action)
{
	return (action == DIVOOM_BTCORE_KEY_ACTION_LONG
		|| action == DIVOOM_BTCORE_KEY_ACTION_HOLD
		|| action == DIVOOM_BTCORE_KEY_ACTION_UP) ? 1 : 0;
}

uint8_t divoom_btcore_key_map(uint8_t key_value, uint8_t key_action)
{
	switch (key_value)
	{
	case 11: /* KEY_AD_NUM1 MODE */
		return (uint8_t)DIVOOM_KEY_ASTROTOO_KEY_MUSIC_MODE;
	case 12: /* KEY_AD_NUM2 VOL+ / 长按亮度+ */
#if 0
		if (btcore_is_vol_short_action(key_action))
			return (uint8_t)DIVOOM_KEY_ASTROTOO_KEY_VOL_ADD;
		if (btcore_is_lig_action(key_action))
			return (uint8_t)DIVOOM_KEY_ASTROTOO_KEY_LIG_ADD;
		return 0xFFu;
#else
		return (uint8_t)DIVOOM_KEY_ASTROTOO_KEY_LIGVOL_ADD;
#endif
	case 13: /* KEY_AD_NUM3 PP */
		return (uint8_t)DIVOOM_KEY_ASTROTOO_KEY_MUSIC_PLAY;
	case 14: /* KEY_AD_NUM4 VOL- / 长按亮度- */
#if 0
		if (btcore_is_vol_short_action(key_action))
			return (uint8_t)DIVOOM_KEY_ASTROTOO_KEY_VOL_SUB;
		if (btcore_is_lig_action(key_action))
			return (uint8_t)DIVOOM_KEY_ASTROTOO_KEY_LIG_SUB;
		return 0xFFu;
#else
		return (uint8_t)DIVOOM_KEY_ASTROTOO_KEY_LIGVOL_SUB;
#endif
	case 80: /* KEY_AD2_NUM0 右 */
		return (uint8_t)DIVOOM_KEY_ASTROTOO_KEY_RIGHT;
	case 81: /* KEY_AD2_NUM1 OK */
		return (uint8_t)DIVOOM_KEY_ASTROTOO_KEY_ENTER;
	case 82: /* KEY_AD2_NUM2 上 */
		return (uint8_t)DIVOOM_KEY_ASTROTOO_KEY_UP;
	case 83: /* KEY_AD2_NUM3 左 */
		return (uint8_t)DIVOOM_KEY_ASTROTOO_KEY_LEFT;
	case 84: /* KEY_AD2_NUM4 下 */
		return (uint8_t)DIVOOM_KEY_ASTROTOO_KEY_DOWN;
	case 85: /* KEY_AD2_NUM5 返回 */
		return (uint8_t)DIVOOM_KEY_ASTROTOO_KEY_ESC;
	default:
		return 0xFFu;
	}
}

uint16_t divoom_btcore_press_map(uint8_t key_action)
{
	static const uint16_t map[5] = {
		DIVOOM_PRESS_SHORT,
		DIVOOM_PRESS_LONG,
		DIVOOM_PRESSING,
		DIVOOM_PRESS_UP,
		DIVOOM_PRESS_DOUBLE,
	};

	if (key_action < 5u)
		return map[key_action];
	return DIVOOM_NO_PRESS;
}

int divoom_btcore_key_is_lvgl_nav(uint8_t ui_key)
{
	switch (ui_key) {
	case DIVOOM_KEY_ASTROTOO_KEY_UP:
	case DIVOOM_KEY_ASTROTOO_KEY_DOWN:
	case DIVOOM_KEY_ASTROTOO_KEY_LEFT:
	case DIVOOM_KEY_ASTROTOO_KEY_RIGHT:
	case DIVOOM_KEY_ASTROTOO_KEY_ENTER:
	case DIVOOM_KEY_ASTROTOO_KEY_ESC:
		return 1;
	default:
		return 0;
	}
}

int divoom_btcore_key_allowed_when_screen_off(uint8_t ui_key)
{
	switch (ui_key) {
	case DIVOOM_KEY_ASTROTOO_KEY_LIGVOL_ADD:
	case DIVOOM_KEY_ASTROTOO_KEY_LIGVOL_SUB:
	case DIVOOM_KEY_ASTROTOO_KEY_MUSIC_PLAY:
	case DIVOOM_KEY_ASTROTOO_KEY_LEFT:
	case DIVOOM_KEY_ASTROTOO_KEY_RIGHT:
		return 1;
	default:
		return 0;
	}
}

#endif /* DIVOOM_TIMES_VIBE */

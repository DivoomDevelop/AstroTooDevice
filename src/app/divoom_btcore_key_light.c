#include "divoom.h"

#ifdef DIVOOM_TIMES_VIBE

typedef struct _divoom_btcore_key_light_ctrl {
	uint8_t mode;
	uint8_t on_off;
	uint8_t bright;
	uint8_t screen_off_flag;
} divoom_btcore_key_light_ctrl;

static divoom_btcore_key_light_ctrl s_key_light = {
	.mode = DIVOOM_BTCORE_KEY_LIGHT_MODE_DEFAULT,
	.on_off = 1u,
	.bright = DIVOOM_BTCORE_KEY_LIGHT_BRIGHT_DEFAULT,
	.screen_off_flag = 0u,
};

static uint8_t divoom_btcore_key_light_mode_normalize(uint8_t mode)
{
	if (DIVOOM_BTCORE_KEY_LIGHT_MODE_IS_VALID(mode))
		return mode;
	return DIVOOM_BTCORE_KEY_LIGHT_MODE_WINDMILL;
}

static uint8_t divoom_btcore_key_light_bright_normalize(uint8_t bright)
{
	return bright > DIVOOM_BTCORE_KEY_LIGHT_BRIGHT_MAX ?
	       DIVOOM_BTCORE_KEY_LIGHT_BRIGHT_MAX : bright;
}

static void divoom_btcore_key_light_cache_set(uint8_t mode, uint8_t on_off, uint8_t bright)
{
	s_key_light.mode = divoom_btcore_key_light_mode_normalize(mode);
	s_key_light.on_off = on_off ? 1u : 0u;
	s_key_light.bright = divoom_btcore_key_light_bright_normalize(bright);
}

static int divoom_btcore_key_light_send_set(uint8_t mode, uint8_t on_off,
					    uint8_t with_bright, uint8_t bright)
{
	uint8_t payload[4];

	payload[0] = 1u;
	payload[1] = mode;
	payload[2] = on_off ? 1u : 0u;
	if (!with_bright)
		return divoom_btcore_uart_cmd_send(DM_BTCORE_KEY_LIGHT_CTRL, payload, 3u);

	payload[3] = divoom_btcore_key_light_bright_normalize(bright);
	return divoom_btcore_uart_cmd_send(DM_BTCORE_KEY_LIGHT_CTRL, payload, sizeof(payload));
}

int divoom_btcore_key_light_set(uint8_t mode, uint8_t on_off)
{
	mode = divoom_btcore_key_light_mode_normalize(mode);
	if (s_key_light.screen_off_flag) {
		divoom_btcore_key_light_cache_set(mode, on_off, s_key_light.bright);
		return 0;
	}
	if (!divoom_btcore_uart_cmd_is_ready())
		return -1;
	if (divoom_btcore_key_light_send_set(mode, on_off, 0u, s_key_light.bright) != 0)
		return -1;
	divoom_btcore_key_light_cache_set(mode, on_off, s_key_light.bright);
	return 0;
}

int divoom_btcore_key_light_set_full(uint8_t mode, uint8_t on_off, uint8_t bright)
{
	mode = divoom_btcore_key_light_mode_normalize(mode);
	bright = divoom_btcore_key_light_bright_normalize(bright);
	if (s_key_light.screen_off_flag) {
		divoom_btcore_key_light_cache_set(mode, on_off, bright);
		return 0;
	}
	if (!divoom_btcore_uart_cmd_is_ready())
		return -1;
	if (divoom_btcore_key_light_send_set(mode, on_off, 1u, bright) != 0)
		return -1;
	divoom_btcore_key_light_cache_set(mode, on_off, bright);
	return 0;
}

int divoom_btcore_key_light_set_brightness(uint8_t bright)
{
	return divoom_btcore_key_light_set_full(s_key_light.mode, s_key_light.on_off, bright);
}

int divoom_btcore_key_light_off(void)
{
	return divoom_btcore_key_light_set(s_key_light.mode, 0u);
}

void divoom_btcore_key_light_follow_screen(uint8_t screen_on)
{
	s_key_light.screen_off_flag = screen_on ? 0u : 1u;
	if (!divoom_btcore_uart_cmd_is_ready())
		return;
	if (!screen_on) {
		(void)divoom_btcore_key_light_send_set(DIVOOM_BTCORE_KEY_LIGHT_MODE_SCREEN_OFF,
						       1u, 0u, s_key_light.bright);
		return;
	}
	(void)divoom_btcore_key_light_send_set(s_key_light.mode, s_key_light.on_off, 0u,
					       s_key_light.bright);
}

int divoom_btcore_key_light_sync_from_jl(void)
{
	uint8_t payload = 0u;
	uint8_t reply[3];
	uint16_t reply_len = 0;
	int ret;

	if (!divoom_btcore_uart_cmd_is_ready())
		return -1;

	ret = divoom_btcore_uart_cmd_send_wait(DM_BTCORE_KEY_LIGHT_CTRL, &payload, 1u,
					       reply, (uint16_t)sizeof(reply), &reply_len);
	if (ret != 0 || reply_len < 3u)
		return -1;

	if (reply[0] == DIVOOM_BTCORE_KEY_LIGHT_MODE_SCREEN_OFF)
		return 0;

	divoom_btcore_key_light_cache_set(reply[0], reply[1], reply[2]);
	return 0;
}

uint8_t divoom_btcore_key_light_get_mode(void)
{
	return s_key_light.mode;
}

uint8_t divoom_btcore_key_light_get_on_off(void)
{
	return s_key_light.on_off;
}

uint8_t divoom_btcore_key_light_get_brightness(void)
{
	return s_key_light.bright;
}

#if DIVOOM_BUILD_RTOS_DEVICE && defined(DIVOOM_BTCORE_UART_CLI)

#include <console.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int divoom_btcore_key_light_cli_main(int argc, char **argv)
{
	if (argc < 2) {
		printf("keylight get | set <mode> <on_off> [bright] | bright <0-100>\n");
		printf("       off | factory <white|red|green|blue> [bright]\n");
		printf("  user modes: 0=windmill 1=breathe 2=flow_windmill 3=stars\n");
		printf("              4=rainbow 5=fast_run 6=fireworm 7=white\n");
		printf("  factory: 0x80=white 0x81=red 0x82=green 0x83=blue (LCD sync)\n");
		return 0;
	}

	if (strcmp(argv[1], "get") == 0) {
		if (divoom_btcore_key_light_sync_from_jl() != 0) {
			printf("keylight get failed\n");
			return -1;
		}
		printf("keylight mode=%u on_off=%u bright=%u\n",
		       (unsigned)divoom_btcore_key_light_get_mode(),
		       (unsigned)divoom_btcore_key_light_get_on_off(),
		       (unsigned)divoom_btcore_key_light_get_brightness());
		return 0;
	}

	if (strcmp(argv[1], "off") == 0) {
		if (divoom_btcore_key_light_off() != 0) {
			printf("keylight off failed\n");
			return -1;
		}
		printf("keylight off ok\n");
		return 0;
	}

	if (strcmp(argv[1], "bright") == 0) {
		unsigned bright;

		if (argc < 3) {
			printf("usage: keylight bright <0-100>\n");
			return -1;
		}
		bright = (unsigned)strtoul(argv[2], NULL, 0);
		if (divoom_btcore_key_light_set_brightness((uint8_t)bright) != 0) {
			printf("keylight bright failed\n");
			return -1;
		}
		printf("keylight bright ok bright=%u\n", bright);
		return 0;
	}

	if (strcmp(argv[1], "factory") == 0) {
		uint8_t mode = DIVOOM_BTCORE_KEY_LIGHT_MODE_FACTORY_WHITE;
		unsigned bright = DIVOOM_BTCORE_KEY_LIGHT_BRIGHT_DEFAULT;

		if (argc < 3) {
			printf("usage: keylight factory <white|red|green|blue> [bright]\n");
			return -1;
		}
		if (strcmp(argv[2], "white") == 0)
			mode = DIVOOM_BTCORE_KEY_LIGHT_MODE_FACTORY_WHITE;
		else if (strcmp(argv[2], "red") == 0)
			mode = DIVOOM_BTCORE_KEY_LIGHT_MODE_FACTORY_RED;
		else if (strcmp(argv[2], "green") == 0)
			mode = DIVOOM_BTCORE_KEY_LIGHT_MODE_FACTORY_GREEN;
		else if (strcmp(argv[2], "blue") == 0)
			mode = DIVOOM_BTCORE_KEY_LIGHT_MODE_FACTORY_BLUE;
		else {
			printf("unknown factory color: %s\n", argv[2]);
			return -1;
		}
		if (argc >= 4)
			bright = (unsigned)strtoul(argv[3], NULL, 0);
		if (divoom_btcore_key_light_set_full(mode, 1u, (uint8_t)bright) != 0) {
			printf("keylight factory failed\n");
			return -1;
		}
		printf("keylight factory ok mode=0x%02x bright=%u\n",
		       (unsigned)mode, bright);
		return 0;
	}

	if (strcmp(argv[1], "set") == 0) {
		unsigned mode;
		unsigned on_off;
		unsigned bright = 0;
		int has_bright = 0;

		if (argc < 4) {
			printf("usage: keylight set <mode> <on_off> [bright]\n");
			return -1;
		}
		mode = (unsigned)strtoul(argv[2], NULL, 0);
		on_off = (unsigned)strtoul(argv[3], NULL, 0);
		if (argc >= 5) {
			bright = (unsigned)strtoul(argv[4], NULL, 0);
			has_bright = 1;
		}
		if (has_bright) {
			if (divoom_btcore_key_light_set_full((uint8_t)mode, (uint8_t)on_off,
							     (uint8_t)bright) != 0) {
				printf("keylight set failed\n");
				return -1;
			}
			printf("keylight set ok mode=%u on_off=%u bright=%u\n",
			       mode, on_off, bright);
		} else {
			if (divoom_btcore_key_light_set((uint8_t)mode, (uint8_t)on_off) != 0) {
				printf("keylight set failed\n");
				return -1;
			}
			printf("keylight set ok mode=%u on_off=%u (bright unchanged)\n",
			       mode, on_off);
		}
		return 0;
	}

	printf("unknown subcmd: %s\n", argv[1]);
	return -1;
}

FINSH_FUNCTION_EXPORT_CMD(divoom_btcore_key_light_cli_main, keylight, JL key RGB light (0x66));

#endif /* DIVOOM_BTCORE_UART_CLI */

#endif /* DIVOOM_TIMES_VIBE */



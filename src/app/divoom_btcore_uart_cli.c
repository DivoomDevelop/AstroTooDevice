/*
 * BtCore Finsh 终端调试命令 btcore
 *
 * 启用：divoom.h 中 #define DIVOOM_BTCORE_UART_CLI
 * 文档：tools/btcore_cli/README.md
 *
 * 子命令：
 *   help | list [f|j|b|0xNN] | status | send | wait | waitto | rx | key | keys
 *   link | version | boot | poweroff
 *
 * 方向：F→J 用 send/wait；J→F 用 rx；双向见 list 第三列。
 *
 * btcore list 命令表由 tools/btcore_cli/gen_btcore_cli_cmds.py 生成（见 divoom_btcore_uart_cli_cmds.inc）。
 * 新增 divoom_btcore_cmd_id_t 后：python3 tools/btcore_cli/gen_btcore_cli_cmds.py --sync-meta
 */
#include "divoom.h"

#if DIVOOM_BUILD_RTOS_DEVICE && defined(DIVOOM_TIMES_VIBE) && defined(DIVOOM_BTCORE_UART_CLI)

#include <console.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "divoom_btcore_key.h"

#define BTCORE_CLI_PAYLOAD_MAX 256u

/* dir: F=F101→JL  J=JL→F101  B=双向 */
typedef struct btcore_cli_cmd_row {
	uint8_t id;
	char dir;
	const char *name;
	const char *payload;
	const char *example;
} btcore_cli_cmd_row_t;

#include "divoom_btcore_uart_cli_cmds.inc"

typedef struct btcore_cli_key_name {
	const char *name;
	uint8_t hw_id;
} btcore_cli_key_name_t;

static const btcore_cli_key_name_t s_btcore_cli_keys[] = {
	{ "power",  DIVOOM_BTCORE_HW_KEY_POWER },
	{ "mode",   11u },
	{ "chmod",  11u },
	{ "vol+",   12u },
	{ "voladd", 12u },
	{ "pp",     13u },
	{ "play",   13u },
	{ "vol-",   14u },
	{ "volsub", 14u },
	{ "right",  80u },
	{ "ok",     81u },
	{ "enter",  81u },
	{ "up",     82u },
	{ "left",   83u },
	{ "down",   84u },
	{ "back",   85u },
	{ "esc",    85u },
};

static void btcore_cli_print_hex(const char *prefix, const uint8_t *data, uint16_t len)
{
	uint16_t i;

	if (prefix)
		printf("%s", prefix);
	if (!data || len == 0u) {
		printf("(empty)\n");
		return;
	}
	for (i = 0; i < len; i++)
		printf("%s%02x", i ? " " : "", (unsigned)data[i]);
	printf("\n");
}

static int btcore_cli_parse_u8(const char *s, uint8_t *out)
{
	char *end = NULL;
	unsigned long v;

	if (!s || !out)
		return -1;
	v = strtoul(s, &end, 0);
	if (!end || *end != '\0' || v > 255ul)
		return -1;
	*out = (uint8_t)v;
	return 0;
}

static int btcore_cli_parse_payload(int argc, char **argv, int start,
				    uint8_t *out, uint16_t *out_len)
{
	int i;
	uint8_t b;

	if (!out || !out_len)
		return -1;

	*out_len = 0;
	for (i = start; i < argc; i++) {
		if (*out_len >= BTCORE_CLI_PAYLOAD_MAX)
			return -1;
		if (btcore_cli_parse_u8(argv[i], &b) != 0)
			return -1;
		out[(*out_len)++] = b;
	}
	return 0;
}

static int btcore_cli_require_ready(void)
{
	if (!divoom_btcore_uart_cmd_is_ready()) {
		printf("btcore: UART not ready (wait for init)\n");
		return -1;
	}
	return 0;
}

static int btcore_cli_lookup_key(const char *name, uint8_t *out_hw)
{
	size_t i;

	if (!name || !out_hw)
		return -1;
	for (i = 0; i < sizeof(s_btcore_cli_keys) / sizeof(s_btcore_cli_keys[0]); i++) {
		if (strcasecmp(name, s_btcore_cli_keys[i].name) == 0) {
			*out_hw = s_btcore_cli_keys[i].hw_id;
			return 0;
		}
	}
	return btcore_cli_parse_u8(name, out_hw);
}

static int btcore_cli_lookup_action(const char *name, uint8_t *out_action)
{
	if (!name || !out_action)
		return -1;
	if (strcasecmp(name, "click") == 0 || strcasecmp(name, "short") == 0) {
		*out_action = DIVOOM_BTCORE_KEY_ACTION_CLICK;
		return 0;
	}
	if (strcasecmp(name, "long") == 0) {
		*out_action = DIVOOM_BTCORE_KEY_ACTION_LONG;
		return 0;
	}
	if (strcasecmp(name, "hold") == 0 || strcasecmp(name, "pressing") == 0) {
		*out_action = DIVOOM_BTCORE_KEY_ACTION_HOLD;
		return 0;
	}
	if (strcasecmp(name, "up") == 0 || strcasecmp(name, "long_up") == 0) {
		*out_action = DIVOOM_BTCORE_KEY_ACTION_UP;
		return 0;
	}
	if (strcasecmp(name, "double") == 0 || strcasecmp(name, "dbl") == 0) {
		*out_action = DIVOOM_BTCORE_KEY_ACTION_DOUBLE_CLICK;
		return 0;
	}
	return btcore_cli_parse_u8(name, out_action);
}

static const char *btcore_cli_dir_str(char dir)
{
	switch (dir) {
	case 'F': return "F->J";
	case 'J': return "J->F";
	case 'B': return "双向";
	default:  return "?";
	}
}

static int btcore_cli_list_match_dir(char dir, char filter_dir)
{
	if (filter_dir == 0)
		return 1;
	if (filter_dir == 'B')
		return dir == 'B';
	if (filter_dir == 'F')
		return dir == 'F' || dir == 'B';
	if (filter_dir == 'J')
		return dir == 'J' || dir == 'B';
	return 1;
}

static int btcore_cli_list_match_id(uint8_t id, const char *filter)
{
	unsigned long v;
	char *end;

	if (!filter || filter[0] == '\0')
		return 1;
	if (filter[0] == 'f' || filter[0] == 'F' || filter[0] == 'j' || filter[0] == 'J'
	    || filter[0] == 'b' || filter[0] == 'B')
		return 1;
	v = strtoul(filter, &end, 0);
	if (!end || *end != '\0')
		return 0;
	return id == (uint8_t)v;
}

static void btcore_cli_print_help(void)
{
	printf("BtCore F101<->JL UART test (see tools/btcore_cli/README.md)\n");
	printf("Usage:\n");
	printf("  btcore help\n");
	printf("  btcore list [f|j|b|0xNN]              all cmds / filter by dir or id\n");
	printf("  btcore status\n");
	printf("  btcore send <cmd> [byte ...]          F101 -> JL, no wait\n");
	printf("  btcore wait <cmd> [byte ...]          F101 -> JL, wait reply 1s\n");
	printf("  btcore waitto <sec> <cmd> [byte ...]  F101 -> JL, custom timeout\n");
	printf("  btcore rx <cmd> [byte ...]            simulate JL -> F101 locally\n");
	printf("  btcore key <name|hw> <action>         rx 0x4c shortcut\n");
	printf("  btcore link                           DM_BTCORE_CHECK_LINK (0x41)\n");
	printf("  btcore version                        DM_BTCORE_DEVICE_VERSION (0x50)\n");
	printf("  btcore boot                           DM_BTCORE_MAIN_SOC_BOOT_DONE (0x5f)\n");
	printf("  btcore poweroff                       DM_BTCORE_SYSTEM_POWER_OFF (0x19)\n");
	printf("  btcore keys                           list key names / hw ids\n");
	printf("\n");
	printf("cmd / byte: decimal or 0xNN; action: click|long|hold|up|double or 0-4\n");
	printf("list filter: f=F->J  j=J->F  b=bidir only  0x32=one cmd id\n");
	printf("key hw ids: power=10 mode=11 vol+=12 pp=13 vol-=14\n");
	printf("            right=80 ok=81 up=82 left=83 down=84 back=85\n");
}

static int btcore_cli_cmd_list(int argc, char **argv)
{
	const char *filter = (argc >= 3) ? argv[2] : NULL;
	char filter_dir = 0;
	size_t i;
	size_t n = sizeof(s_btcore_cli_cmds) / sizeof(s_btcore_cli_cmds[0]);
	size_t shown = 0;

	if (filter) {
		if (strcasecmp(filter, "f") == 0)
			filter_dir = 'F';
		else if (strcasecmp(filter, "j") == 0)
			filter_dir = 'J';
		else if (strcasecmp(filter, "b") == 0)
			filter_dir = 'B';
	}

	printf("ID   Dir   Name                      Payload              Example\n");
	for (i = 0; i < n; i++) {
		const btcore_cli_cmd_row_t *row = &s_btcore_cli_cmds[i];

		if (!btcore_cli_list_match_dir(row->dir, filter_dir))
			continue;
		if (!btcore_cli_list_match_id(row->id, filter))
			continue;
		printf("0x%02x %-5s %-25s %-20s %s\n",
		       (unsigned)row->id,
		       btcore_cli_dir_str(row->dir),
		       row->name,
		       row->payload ? row->payload : "",
		       row->example ? row->example : "-");
		shown++;
	}
	printf("(%u/%u cmds; doc: tools/btcore_cli/README.md)\n",
	       (unsigned)shown, (unsigned)n);
	return 0;
}

static int btcore_cli_cmd_status(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	printf("btcore ready=%d rx_suppressed=%d\n",
	       divoom_btcore_uart_cmd_is_ready(),
	       divoom_btcore_uart_cmd_is_rx_suppressed());
	return 0;
}

static int btcore_cli_do_xfer(uint8_t cmd, const uint8_t *payload, uint16_t payload_len,
			      int wait, int timeout_s)
{
	int ret;

	printf("btcore %s cmd=0x%02x len=%u\n",
	       wait ? "wait" : "send", (unsigned)cmd, (unsigned)payload_len);
	btcore_cli_print_hex("  tx: ", payload, payload_len);

	if (!wait) {
		ret = divoom_btcore_uart_cmd_send(cmd, payload_len ? payload : NULL, payload_len);
		printf("btcore send ret=%d\n", ret);
		return ret;
	}

	{
		uint8_t reply[BTCORE_CLI_PAYLOAD_MAX];
		uint16_t reply_len = 0;

		ret = divoom_btcore_uart_cmd_send_wait_timeout(cmd,
			payload_len ? payload : NULL, payload_len,
			reply, sizeof(reply), &reply_len, timeout_s > 0 ? timeout_s : 1);
		if (ret != 0) {
			printf("btcore wait ret=%d (timeout=%ds)\n", ret,
			       timeout_s > 0 ? timeout_s : 1);
			return ret;
		}
		printf("btcore wait ok reply_len=%u\n", (unsigned)reply_len);
		btcore_cli_print_hex("  rx: ", reply, reply_len);
	}
	return 0;
}

static int btcore_cli_cmd_send(int argc, char **argv, int wait, int timeout_s)
{
	uint8_t cmd;
	uint8_t payload[BTCORE_CLI_PAYLOAD_MAX];
	uint16_t payload_len = 0;
	int arg_cmd = 2;
	int arg_payload = 3;

	if (argc < arg_cmd + 1) {
		printf("Usage: btcore %s <cmd> [byte ...]\n", wait ? "wait" : "send");
		return -1;
	}
	if (btcore_cli_require_ready() != 0)
		return -1;
	if (btcore_cli_parse_u8(argv[arg_cmd], &cmd) != 0) {
		printf("btcore: invalid cmd '%s'\n", argv[arg_cmd]);
		return -1;
	}
	if (argc > arg_payload) {
		if (btcore_cli_parse_payload(argc, argv, arg_payload, payload, &payload_len) != 0) {
			printf("btcore: invalid payload byte\n");
			return -1;
		}
	}
	return btcore_cli_do_xfer(cmd, payload, payload_len, wait, timeout_s);
}

static int btcore_cli_cmd_waitto(int argc, char **argv)
{
	uint8_t cmd;
	uint8_t payload[BTCORE_CLI_PAYLOAD_MAX];
	uint16_t payload_len = 0;
	int timeout_s;

	if (argc < 4) {
		printf("Usage: btcore waitto <sec> <cmd> [byte ...]\n");
		return -1;
	}
	timeout_s = atoi(argv[2]);
	if (timeout_s <= 0)
		timeout_s = 1;
	if (btcore_cli_require_ready() != 0)
		return -1;
	if (btcore_cli_parse_u8(argv[3], &cmd) != 0) {
		printf("btcore: invalid cmd '%s'\n", argv[3]);
		return -1;
	}
	if (argc > 4) {
		if (btcore_cli_parse_payload(argc, argv, 4, payload, &payload_len) != 0) {
			printf("btcore: invalid payload byte\n");
			return -1;
		}
	}
	return btcore_cli_do_xfer(cmd, payload, payload_len, 1, timeout_s);
}

static int btcore_cli_cmd_rx(int argc, char **argv)
{
	uint8_t cmd;
	uint8_t payload[BTCORE_CLI_PAYLOAD_MAX];
	uint16_t payload_len = 0;

	if (argc < 3) {
		printf("Usage: btcore rx <cmd> [byte ...]\n");
		return -1;
	}
	if (btcore_cli_parse_u8(argv[2], &cmd) != 0) {
		printf("btcore: invalid cmd '%s'\n", argv[2]);
		return -1;
	}
	if (argc > 3) {
		if (btcore_cli_parse_payload(argc, argv, 3, payload, &payload_len) != 0) {
			printf("btcore: invalid payload byte\n");
			return -1;
		}
	}

	printf("btcore rx simulate cmd=0x%02x len=%u\n",
	       (unsigned)cmd, (unsigned)payload_len);
	btcore_cli_print_hex("  data: ", payload, payload_len);
	divoom_btcore_uart_cmd_debug_simulate_rx(cmd,
		payload_len ? payload : NULL, payload_len);
	return 0;
}

static int btcore_cli_cmd_key(int argc, char **argv)
{
	uint8_t hw;
	uint8_t action;
	uint8_t payload[2];
	uint8_t ui_key;

	if (argc < 4) {
		printf("Usage: btcore key <name|hw> <action>\n");
		return -1;
	}
	if (btcore_cli_lookup_key(argv[2], &hw) != 0) {
		printf("btcore: unknown key '%s'\n", argv[2]);
		return -1;
	}
	if (btcore_cli_lookup_action(argv[3], &action) != 0 || action > 4u) {
		printf("btcore: invalid action '%s'\n", argv[3]);
		return -1;
	}

	payload[0] = hw;
	payload[1] = action;
	ui_key = divoom_btcore_key_map(hw, action);
	printf("btcore key hw=%u action=%u -> ui_key=0x%02x press=%u\n",
	       (unsigned)hw, (unsigned)action, (unsigned)ui_key,
	       (unsigned)divoom_btcore_press_map(action));
	divoom_btcore_uart_cmd_debug_simulate_rx(DM_BTCORE_SEND_KEY_SYSTEM_MSG, payload, 2);
	return 0;
}

static int btcore_cli_cmd_link(int argc, char **argv)
{
	int ret;

	(void)argc;
	(void)argv;

	if (btcore_cli_require_ready() != 0)
		return -1;
	ret = divoom_btcore_uart_cmd_send_check_link();
	printf("btcore link ret=%d\n", ret);
	return ret;
}

static int btcore_cli_cmd_version(int argc, char **argv)
{
	uint32_t ver = 0;
	int ret;

	(void)argc;
	(void)argv;

	if (btcore_cli_require_ready() != 0)
		return -1;
	ret = divoom_btcore_uart_cmd_get_jl_device_version(&ver);
	printf("btcore version ret=%d jl_ver=%u (0x%08x)\n", ret, (unsigned)ver, (unsigned)ver);
	return ret;
}

static int btcore_cli_cmd_boot(int argc, char **argv)
{
	int ret;

	(void)argc;
	(void)argv;

	if (btcore_cli_require_ready() != 0)
		return -1;
	ret = divoom_btcore_uart_cmd_notify_aic_host_audio_ready();
	printf("btcore boot ret=%d\n", ret);
	return ret;
}

static int btcore_cli_cmd_poweroff(int argc, char **argv)
{
	int ret;

	(void)argc;
	(void)argv;

	if (btcore_cli_require_ready() != 0)
		return -1;
	printf("btcore: sending SYSTEM_POWER_OFF (0x19)\n");
	ret = divoom_btcore_uart_cmd_send(DM_BTCORE_SYSTEM_POWER_OFF, NULL, 0);
	printf("btcore poweroff ret=%d\n", ret);
	return ret;
}

static int btcore_cli_cmd_keys(int argc, char **argv)
{
	size_t i;

	(void)argc;
	(void)argv;

	for (i = 0; i < sizeof(s_btcore_cli_keys) / sizeof(s_btcore_cli_keys[0]); i++)
		printf("  %-8s hw=%u\n", s_btcore_cli_keys[i].name,
		       (unsigned)s_btcore_cli_keys[i].hw_id);
	return 0;
}

static int btcore_cli_main(int argc, char **argv)
{
	if (argc < 2 || strcmp(argv[1], "help") == 0 || strcmp(argv[1], "-h") == 0) {
		btcore_cli_print_help();
		return 0;
	}
	if (strcmp(argv[1], "list") == 0)
		return btcore_cli_cmd_list(argc, argv);
	if (strcmp(argv[1], "status") == 0)
		return btcore_cli_cmd_status(argc, argv);
	if (strcmp(argv[1], "send") == 0)
		return btcore_cli_cmd_send(argc, argv, 0, 0);
	if (strcmp(argv[1], "wait") == 0)
		return btcore_cli_cmd_send(argc, argv, 1, 0);
	if (strcmp(argv[1], "waitto") == 0)
		return btcore_cli_cmd_waitto(argc, argv);
	if (strcmp(argv[1], "rx") == 0)
		return btcore_cli_cmd_rx(argc, argv);
	if (strcmp(argv[1], "key") == 0)
		return btcore_cli_cmd_key(argc, argv);
	if (strcmp(argv[1], "link") == 0)
		return btcore_cli_cmd_link(argc, argv);
	if (strcmp(argv[1], "version") == 0)
		return btcore_cli_cmd_version(argc, argv);
	if (strcmp(argv[1], "boot") == 0)
		return btcore_cli_cmd_boot(argc, argv);
	if (strcmp(argv[1], "poweroff") == 0)
		return btcore_cli_cmd_poweroff(argc, argv);
	if (strcmp(argv[1], "keys") == 0)
		return btcore_cli_cmd_keys(argc, argv);

	printf("btcore: unknown subcommand '%s' (try: btcore help)\n", argv[1]);
	return -1;
}

FINSH_FUNCTION_EXPORT_CMD(btcore_cli_main, btcore,
	F101<->JL BtCore UART test: list/send/wait/rx/key);

#endif /* DIVOOM_BUILD_RTOS_DEVICE && DIVOOM_TIMES_VIBE && DIVOOM_BTCORE_UART_CLI */

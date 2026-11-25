#include "divoom.h"
#include "divoom_product_test.h"
#include "divoom_product_test_usr_check.h"
#include "divoom_product_test_ui.h"
#include "divoom_pt_ui.h"
#include "divoom_btcore_uart_cmd.h"
#include "divoom_btcore_key.h"
#include "divoom_ota_jl.h"
#include "divoom_onoff.h"
#include "divoom_user_key.h"
#include "divoom_wifi_con.h"
#include "divoom_ota_server.h"
#include "divoom_ota_rtos.h"
#include "divoom_http_client.h"
#include "divoom_app_com.h"
#include "md5.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef DIVOOM_SIMULATOR
#include "aic_wifi.h"
#endif

#ifdef CONFIG_DRIVERS_RTC
#include <sunxi_hal_rtc.h>
#endif
#ifdef CONFIG_DRIVERS_V2_RTC
#include <hal_rtc.h>
#endif
#ifdef CONFIG_DRIVERS_SPINOR
#include <sunxi_hal_spinor.h>
#endif
#ifdef CONFIG_DRIVERS_WATCHDOG
#include <sunxi_hal_watchdog.h>
#endif
#ifdef CONFIG_DRIVERS_V2_WATCHDOG
#include <hal_watchdog.h>
#endif

static int s_pt_active;
static volatile int s_pt_wifi_boot_pending;
static uint8 s_pt_brightness = DIVOOM_PT_BRIGHTNESS_DEFAULT;
static uint8 s_pt_volume = DIVOOM_PT_VOLUME_DEFAULT;
static uint8 s_pt_insert_uac;   /* 0x4F：UAC 音频类已枚举 */
static uint8 s_pt_insert_usb;   /* 0x35：USB 5V/充电插入 */
static uint8 s_pt_insert_linein;
static uint8 s_pt_insert_tf;
static uint32_t s_pt_jl_version;
static int s_pt_download_file_flag; /* DIVOOM_PT_DL_* */
static int s_pt_wifi_up;            /* 本次产测曾经取得过 IP，主动关 WiFi 后仍保留 */
static int s_pt_wifi_connected;     /* 当前工厂网络连接状态，仅用于重连控制 */
static int s_pt_download_cur;
static int s_pt_download_total;
static int s_pt_customize_updating;
static int s_pt_customize_applied;
static int s_pt_fw_percent;
static int s_pt_fw_result;
static int s_pt_customize_fail;
static int s_pt_fw_checked;
static int s_pt_ota_started;
static volatile int s_pt_update_owner; /* 0: idle, 1: OTA, 2: format, 3: read-only /usr test */
static int s_pt_ota_retry_pending;
static int s_pt_allow_flash;
static int s_pt_flash_started;
static int s_pt_jl_update_started;
static volatile int s_pt_next_selected;
static volatile int s_pt_menu_selected;
static int s_pt_reboot_marker_armed;
static volatile int s_pt_wifi_retry_pending;
static volatile int s_pt_wifi_shutdown;
static uint32_t s_pt_wifi_retry_tick;
static divoom_update_json_listguide_info s_pt_listguide;

#define DIVOOM_PT_CUSTOMIZE_JSON_PATH  DIVOOM_TMP_DIR "/divoom_update_info_customize.json"
#define DIVOOM_PT_CRC_RAW_TMP_PATH      DIVOOM_TMP_DIR "/file_crc_raw.bin"
#define DIVOOM_PT_CRC_UPDATE_TMP_PATH   DIVOOM_TMP_DIR "/file_crc_update.bin"
#define DIVOOM_PT_CRC_VERSION_TMP_PATH  DIVOOM_TMP_DIR "/file_crc_version.txt"
#define DIVOOM_PT_CRC_LOCAL_PATH        "/usr/file_crc.bin"
#define DIVOOM_PT_CRC_VERSION_PATH      "/usr/file_crc_version.txt"
#define DIVOOM_PT_CRC_ROOT_PATH         "/usr"
#define DIVOOM_PT_CRC_PATH_MAX          512
#define DIVOOM_PT_CRC_URL_MAX           256
#define DIVOOM_PT_CRC_MD5_TEXT_SIZE     33
#define DIVOOM_PT_COPY_FILE_MAX         200
#define DIVOOM_PT_OTA_RETRY_MS          5000
#define DIVOOM_PT_WIFI_RETRY_MS         2000

enum {
	DIVOOM_PT_CUSTOMIZE_ERROR = -1,
	DIVOOM_PT_CUSTOMIZE_NONE = 0,
	DIVOOM_PT_CUSTOMIZE_APPLIED = 1,
};

enum {
	DIVOOM_PT_CRC_COUNT_DOWNLOAD = 0,
	DIVOOM_PT_CRC_DOWNLOAD,
	DIVOOM_PT_CRC_DELETE,
};

typedef struct {
	int action;
	const char *base_url;
	int actionable;
	int completed;
	int total;
	int failed;
	int *download_cur;
	int *download_total;
} divoom_pt_crc_apply_ctx;

typedef struct {
	uint32_t msg;
	uint32_t param;
} divoom_pt_msg;

static void *s_pt_msg_queue;
static int s_pt_task_started;
static void divoom_product_test_schedule_ota_retry(void);

static uint32_t divoom_product_test_get_le32(const uint8_t *p)
{
	return (uint32_t)p[0]
		| ((uint32_t)p[1] << 8)
		| ((uint32_t)p[2] << 16)
		| ((uint32_t)p[3] << 24);
}

/*
 * 产测 TF 听音：JL 扫盘完成后会发空 0x42 向 F101 要歌单。
 * 产测不走歌单握手（不回 0x42、也等不到 0x5e），把空 0x42 当作扫盘完成，
 * 直接 0x0e 播第 1 首，并下 8 档音量方便产线听声。
 */
static void divoom_product_test_tf_play_first(void)
{
	uint8_t vol[2] = {1, 8};
	uint8_t play_id[3] = {1, 1, 0};

	if (!divoom_btcore_uart_cmd_is_ready())
		return;
	divoom_btcore_uart_cmd_send(DM_BTCORE_SD_MUSIC_VOLUME, vol, 2);
	divoom_btcore_uart_cmd_send(DM_BTCORE_SD_MUSIC_PLAYED_ID, play_id, 3);
	divoom_print("product_test: tf play id=1 vol=8\n");
}

static void divoom_product_test_set_insert_status(uint8 type, uint8 inserted)
{
	uint8 value = inserted ? 1 : 0;
	uint8 prev;

	if (type == DIVOOM_PRODUCT_TEST_INSERT_UAC) {
		prev = s_pt_insert_uac;
		s_pt_insert_uac = value;
		if (prev != value)
			divoom_print("product_test: uac(0x4f) %u -> %u usb(0x35)=%u\n",
				     (unsigned)prev, (unsigned)value, (unsigned)s_pt_insert_usb);
	} else if (type == DIVOOM_PRODUCT_TEST_INSERT_USB) {
		prev = s_pt_insert_usb;
		s_pt_insert_usb = value;
		if (prev != value)
			divoom_print("product_test: usb(0x35) %u -> %u uac(0x4f)=%u\n",
				     (unsigned)prev, (unsigned)value, (unsigned)s_pt_insert_uac);
	} else if (type == DIVOOM_PRODUCT_TEST_INSERT_LINEIN) {
		prev = s_pt_insert_linein;
		s_pt_insert_linein = value;
		if (prev != value)
			divoom_print("product_test: linein %u -> %u\n",
				     (unsigned)prev, (unsigned)value);
	} else if (type == DIVOOM_PRODUCT_TEST_INSERT_TF) {
		prev = s_pt_insert_tf;
		s_pt_insert_tf = value;
		if (prev != value)
			divoom_print("product_test: tf %u -> %u\n",
				     (unsigned)prev, (unsigned)value);
	}
}

static void divoom_product_test_handle_key(const uint8_t *cmd_data, uint16_t cmd_len)
{
	uint8_t key_value;
	uint8_t key_action;
	uint8_t ui_key;
	uint16_t ui_press;

	if (!cmd_data || cmd_len < 2u)
		return;

	key_value = cmd_data[0];
	key_action = cmd_data[1];

	if (key_value == DIVOOM_BTCORE_HW_KEY_POWER) {
		if (key_action != DIVOOM_BTCORE_KEY_ACTION_CLICK)
			return;
		ui_key = (uint8_t)kbPOWER;
	} else {
		ui_key = divoom_btcore_key_map(key_value, key_action);
		if (ui_key == 0xFFu)
			return;
	}

	ui_press = divoom_btcore_press_map(key_action);
	if (ui_press == DIVOOM_NO_PRESS)
		return;

	divoom_pt_ui_post_key(ui_key, (uint8_t)ui_press);
}

void divoom_product_test_set_active(int active)
{
	s_pt_active = active ? 1 : 0;
}

int divoom_product_test_is_active(void)
{
	return s_pt_active;
}

uint8 divoom_product_test_get_brightness(void)
{
	return s_pt_brightness;
}

void divoom_product_test_set_brightness(uint8 value)
{
	s_pt_brightness = (value > 100) ? 100 : value;
}

uint8 divoom_product_test_get_volume(void)
{
	return s_pt_volume;
}

void divoom_product_test_set_volume(uint8 value)
{
	s_pt_volume = (value > 100) ? 100 : value;
}

uint8 divoom_product_test_get_insert_status(uint8 type)
{
	/* 产测「插 USB」看 5V(0x35)；UAC(0x4F) 在 USB_PC_NO_APP_MODE 下插线不会置位 */
	if (type == DIVOOM_PRODUCT_TEST_INSERT_UAC)
		return (s_pt_insert_usb || s_pt_insert_uac) ? 1 : 0;
	if (type == DIVOOM_PRODUCT_TEST_INSERT_LINEIN)
		return s_pt_insert_linein;
	if (type == DIVOOM_PRODUCT_TEST_INSERT_TF)
		return s_pt_insert_tf;
	return 0;
}

static int s_pt_check_link_sent;

/* CHECK_LINK 只握手：杰理 set_link 后会主动推 0x35/0x4F/0x06，不必 send_wait 查询 */
static void divoom_product_test_send_check_link(void)
{
	const char *fw_path = DIVOOM_BTCORE_JL_FW_UPDATE_PATH;
	uint8_t payload[4 + 1 + 255];
	size_t path_len;
	uint16_t payload_len;

	if (s_pt_check_link_sent)
		return;
	if (!divoom_btcore_uart_cmd_is_ready())
		return;
	if (!fw_path)
		fw_path = "";

	path_len = strlen(fw_path);
	if (path_len > 255u)
		path_len = 255u;

	payload[0] = (uint8_t)DIVOOM_SOFTWARE_VERSION;
	payload[1] = (uint8_t)(DIVOOM_SOFTWARE_VERSION >> 8);
	payload[2] = (uint8_t)(DIVOOM_SOFTWARE_VERSION >> 16);
	payload[3] = (uint8_t)(DIVOOM_SOFTWARE_VERSION >> 24);
	payload[4] = (uint8_t)path_len;
	if (path_len)
		memcpy(payload + 5, fw_path, path_len);
	payload_len = (uint16_t)(5u + path_len);

	if (divoom_btcore_uart_cmd_send(DM_BTCORE_CHECK_LINK, payload, payload_len) != 0) {
		divoom_print("product_test: check link send failed\n");
		return;
	}
	s_pt_check_link_sent = 1;
	divoom_print("product_test: check link sent\n");
}

static int divoom_product_test_mk_parent(const char *file_path)
{
	char dir_path[DIVOOM_PT_CRC_PATH_MAX];
	const char *slash;
	int dir_len;

	if (!file_path || !file_path[0])
		return 0;
	slash = strrchr(file_path, '/');
	if (!slash || slash == file_path)
		return 1;
	dir_len = (int)(slash - file_path);
	if (dir_len <= 0 || dir_len >= (int)sizeof(dir_path))
		return 0;
	memcpy(dir_path, file_path, (size_t)dir_len);
	dir_path[dir_len] = '\0';
	return divoom_app_com_check_and_mk_dir(dir_path) == 0;
}

static int divoom_product_test_json_type(const divoom_cJSON *item, int type)
{
	return item && ((item->type & 0xff) == type);
}

static int divoom_product_test_crc_hex_value(char value);

static divoom_cJSON *divoom_product_test_json_item_alias(
	divoom_cJSON *object, const char *name, const char *alias)
{
	divoom_cJSON *item;

	item = divoom_cJSON_GetObjectItem(object, name);
	return item ? item : divoom_cJSON_GetObjectItem(object, alias);
}

static int divoom_product_test_copy_path_valid(const char *path)
{
	const char *segment;
	const char *cursor;
	size_t length;

	if (!path)
		return 0;
	length = strnlen(path, DIVOOM_PT_CRC_PATH_MAX);
	if (length == 0 || length >= DIVOOM_PT_CRC_PATH_MAX ||
	    (strncmp(path, "/usr/", 5) != 0 &&
	     strncmp(path, "/userdata/", 10) != 0) ||
	    strcmp(path, DIVOOM_PT_CRC_LOCAL_PATH) == 0 ||
	    strcmp(path, DIVOOM_PT_CRC_VERSION_PATH) == 0)
		return 0;
	for (cursor = path; *cursor; cursor++) {
		unsigned char value = (unsigned char)*cursor;

		if (value < 0x20 || value == 0x7f || *cursor == '\\' ||
		    *cursor == ':')
			return 0;
	}
	segment = path + 1;
	while (*segment) {
		const char *slash = strchr(segment, '/');
		size_t segment_len = slash ? (size_t)(slash - segment) :
			strlen(segment);

		if (segment_len == 0 ||
		    (segment_len == 1 && segment[0] == '.') ||
		    (segment_len == 2 && segment[0] == '.' && segment[1] == '.'))
			return 0;
		if (!slash)
			break;
		segment = slash + 1;
	}
	return 1;
}

static int divoom_product_test_copy_crc32(divoom_cJSON *item,
	uint32_t *crc32)
{
	uint32_t value = 0;
	size_t i;
	const char *text;

	if (!item || !crc32)
		return 0;
	if (divoom_product_test_json_type(item, divoom_cJSON_Number)) {
		if (item->valuedouble < 0.0 || item->valuedouble > 4294967295.0)
			return 0;
		value = (uint32_t)item->valuedouble;
		if ((double)value != item->valuedouble)
			return 0;
		*crc32 = value;
		return 1;
	}
	if (!divoom_product_test_json_type(item, divoom_cJSON_String) ||
	    !item->valuestring)
		return 0;
	text = item->valuestring;
	if (strlen(text) != 8U)
		return 0;
	for (i = 0; i < 8U; i++) {
		int digit = divoom_product_test_crc_hex_value(text[i]);

		if (digit < 0)
			return 0;
		value = (value << 4) | (uint32_t)digit;
	}
	*crc32 = value;
	return 1;
}

static int divoom_product_test_copy_item_parse(divoom_cJSON *item,
	char *local_path, size_t local_size, char *url, size_t url_size,
	uint32_t *crc32)
{
	divoom_cJSON *local_item;
	divoom_cJSON *url_item;
	divoom_cJSON *crc_item;
	const char *url_text;

	if (!divoom_product_test_json_type(item, divoom_cJSON_Object))
		return 0;
	local_item = divoom_product_test_json_item_alias(item, "local", "path");
	url_item = divoom_product_test_json_item_alias(item, "net", "url");
	crc_item = divoom_product_test_json_item_alias(item, "crc32", "crc");
	if (!divoom_product_test_json_type(local_item, divoom_cJSON_String) ||
	    !local_item->valuestring ||
	    !divoom_product_test_copy_path_valid(local_item->valuestring) ||
	    strlen(local_item->valuestring) + sizeof(".divoom_download") >=
		local_size ||
	    !divoom_product_test_json_type(url_item, divoom_cJSON_String) ||
	    !url_item->valuestring || !url_item->valuestring[0] ||
	    strlen(url_item->valuestring) >= url_size ||
	    !divoom_product_test_copy_crc32(crc_item, crc32))
		return 0;
	url_text = url_item->valuestring;
	if (strncmp(url_text, "http://", 7) != 0 &&
	    strncmp(url_text, "https://", 8) != 0)
		return 0;
	strcpy(local_path, local_item->valuestring);
	strcpy(url, url_text);
	return 1;
}

static int divoom_product_test_copy_local_matches(const char *path,
	uint32_t expected_crc)
{
	uint64_t size;
	uint32_t crc32;

	return divoom_port_file_crc32(path, &size, &crc32) == 0 &&
		crc32 == expected_crc;
}

static int divoom_product_test_crc_bookkeeping_path(const char *relative_path)
{
	return relative_path &&
		(strcmp(relative_path, "file_crc.bin") == 0 ||
		 strcmp(relative_path, "file_crc_version.txt") == 0 ||
		 strcmp(relative_path, "file_crc.bin.divoom_new") == 0 ||
		 strcmp(relative_path, "file_crc.bin.divoom_old") == 0 ||
		 strcmp(relative_path, "file_crc_version.txt.divoom_new") == 0 ||
		 strcmp(relative_path, "file_crc_version.txt.divoom_old") == 0);
}

static int divoom_product_test_crc_join_url(char *output, size_t output_size,
					    const char *base, const char *relative_path)
{
	int written;
	size_t base_len;

	if (!output || !output_size || !base || !base[0] ||
	    !relative_path || !relative_path[0])
		return 0;
	base_len = strlen(base);
	written = snprintf(output, output_size,
		base[base_len - 1] == '/' ? "%s%s" : "%s/%s",
		base, relative_path);
	return written > 0 && written < (int)output_size;
}

static int divoom_product_test_crc_hex_value(char value)
{
	if (value >= '0' && value <= '9')
		return value - '0';
	if (value >= 'a' && value <= 'f')
		return value - 'a' + 10;
	if (value >= 'A' && value <= 'F')
		return value - 'A' + 10;
	return -1;
}

static int divoom_product_test_crc_check_md5(const char *path,
					     const char *expected)
{
	unsigned char digest[16];
	char *file_data;
	int file_len = 0;
	int i;

	if (!path || !expected || strlen(expected) != 32U)
		return 0;
	file_data = divoom_app_com_read_file((char *)path, &file_len);
	if (!file_data || file_len <= 0) {
		if (file_data)
			divoom_system_free(file_data);
		return 0;
	}
	md5((unsigned char *)file_data, file_len, digest);
	divoom_system_free(file_data);
	for (i = 0; i < 16; i++) {
		int high = divoom_product_test_crc_hex_value(expected[i * 2]);
		int low = divoom_product_test_crc_hex_value(expected[i * 2 + 1]);

		if (high < 0 || low < 0 || digest[i] != (unsigned char)((high << 4) | low))
			return 0;
	}
	return 1;
}

static int divoom_product_test_crc_write_empty(const char *path)
{
	void *fp;

	if (!divoom_product_test_mk_parent(path))
		return 0;
	fp = divoom_port_file_open(path, "wb");
	if (!fp)
		return 0;
	return divoom_port_file_close(fp) == 0;
}

static void divoom_product_test_crc_report_progress(divoom_pt_crc_apply_ctx *ctx)
{
	if (!ctx || ctx->total <= 0)
		return;
	ctx->completed++;
	if (ctx->download_cur)
		*ctx->download_cur = ctx->completed;
	divoom_product_test_on_ota_percent(
		(ctx->completed * 100) / ctx->total);
}

static int divoom_product_test_crc_change(
	const divoom_port_dir_change_t *change, void *user_data)
{
	divoom_pt_crc_apply_ctx *ctx = (divoom_pt_crc_apply_ctx *)user_data;
	char local_path[DIVOOM_PT_CRC_PATH_MAX];
	char temp_path[DIVOOM_PT_CRC_PATH_MAX];
	char download_url[DIVOOM_PT_CRC_URL_MAX];
	divoom_port_stat_t local_stat;
	uint64_t actual_size = 0;
	uint64_t expected_size;
	uint32_t actual_crc = 0;
	uint32_t expected_crc;
	int download_change;
	int delete_change;
	int written;

	if (!ctx || !change || !change->relative_path ||
	    divoom_product_test_crc_bookkeeping_path(change->relative_path))
		return 0;
	download_change = change->type == DIVOOM_PORT_DIR_CHANGE_MODIFIED ||
		change->type == DIVOOM_PORT_DIR_CHANGE_ADDED;
	delete_change = change->type == DIVOOM_PORT_DIR_CHANGE_REMOVED;
	if (ctx->action == DIVOOM_PT_CRC_COUNT_DOWNLOAD) {
		if (download_change)
			ctx->actionable++;
		return 0;
	}
	written = snprintf(local_path, sizeof(local_path), "%s/%s",
		DIVOOM_PT_CRC_ROOT_PATH, change->relative_path);
	if (written <= 0 || written >= (int)sizeof(local_path)) {
		ctx->failed = 1;
		return 1;
	}

	if (ctx->action == DIVOOM_PT_CRC_DOWNLOAD && download_change) {
		expected_size = change->new_size;
		expected_crc = change->new_crc32;
		written = snprintf(temp_path, sizeof(temp_path), "%s.divoom_new",
			local_path);
		if (written <= 0 || written >= (int)sizeof(temp_path)) {
			ctx->failed = 1;
			return 1;
		}
		if (!divoom_product_test_mk_parent(local_path)) {
			ctx->failed = 1;
			return 1;
		}
		divoom_port_remove_file(temp_path);
		if (expected_size == 0) {
			if (!divoom_product_test_crc_write_empty(temp_path)) {
				ctx->failed = 1;
				return 1;
			}
		} else {
			if (!divoom_product_test_crc_join_url(download_url,
				    sizeof(download_url), ctx->base_url,
				    change->relative_path) ||
			    divoom_http_client_down_file(temp_path, download_url,
				    0, NULL) != 1) {
				divoom_print("product_test: crc file download fail %s\n",
					     change->relative_path);
				divoom_port_remove_file(temp_path);
				ctx->failed = 1;
				return 1;
			}
		}
		if (divoom_port_file_crc32(temp_path, &actual_size, &actual_crc) != 0 ||
		    actual_size != expected_size || actual_crc != expected_crc) {
			divoom_print("product_test: downloaded file check fail %s "
				     "size=%llu/%llu crc=%08x/%08x\n",
				     change->relative_path,
				     (unsigned long long)actual_size,
				     (unsigned long long)expected_size,
				     (unsigned)actual_crc, (unsigned)expected_crc);
			divoom_port_remove_file(temp_path);
			ctx->failed = 1;
			return 1;
		}
		if (divoom_port_stat(local_path, &local_stat) == 0 &&
		    (local_stat.is_dir ? divoom_port_remove_tree(local_path) :
		     divoom_port_remove_file(local_path)) != 0) {
			divoom_port_remove_file(temp_path);
			ctx->failed = 1;
			return 1;
		}
		if (divoom_port_rename_file(temp_path, local_path) != 0) {
			divoom_port_remove_file(temp_path);
			ctx->failed = 1;
			return 1;
		}
		divoom_product_test_crc_report_progress(ctx);
	} else if (ctx->action == DIVOOM_PT_CRC_DELETE && delete_change) {
		if (divoom_port_stat(local_path, &local_stat) == 0 &&
		    (local_stat.is_dir ? divoom_port_remove_tree(local_path) :
		     divoom_port_remove_file(local_path)) != 0) {
			divoom_print("product_test: crc file delete fail %s\n",
				     change->relative_path);
			ctx->failed = 1;
			return 1;
		}
	}
	return 0;
}

static int divoom_product_test_crc_compare_manifests(
	const char *old_manifest_path, const char *new_manifest_path,
	divoom_pt_crc_apply_ctx *ctx)
{
	divoom_port_dir_compare_result_t result;
	int rc;

	memset(&result, 0, sizeof(result));
	rc = divoom_port_dir_manifest_compare(old_manifest_path,
		new_manifest_path,
		ctx ? divoom_product_test_crc_change : NULL, ctx, &result);
	if (rc < 0) {
		divoom_print("product_test: crc manifest compare fail rc=%d\n", rc);
		return 0;
	}
	return 1;
}

static int divoom_product_test_crc_commit_file(const char *source,
					       const char *destination)
{
	char new_path[DIVOOM_PT_CRC_PATH_MAX];
	char old_path[DIVOOM_PT_CRC_PATH_MAX];
	int had_destination;

	if (snprintf(new_path, sizeof(new_path), "%s.divoom_new", destination) >=
		(int)sizeof(new_path) ||
	    snprintf(old_path, sizeof(old_path), "%s.divoom_old", destination) >=
		(int)sizeof(old_path))
		return 0;
	divoom_port_remove_file(new_path);
	divoom_port_remove_file(old_path);
	if (divoom_port_copy_file(source, new_path) != 0)
		return 0;
	divoom_port_sync();
	had_destination = divoom_port_file_exists(destination);
	if (had_destination &&
	    divoom_port_rename_file(destination, old_path) != 0) {
		divoom_port_remove_file(new_path);
		return 0;
	}
	if (divoom_port_rename_file(new_path, destination) != 0) {
		divoom_port_remove_file(new_path);
		if (had_destination)
			divoom_port_rename_file(old_path, destination);
		return 0;
	}
	if (had_destination)
		divoom_port_remove_file(old_path);
	divoom_port_sync();
	return 1;
}

static int divoom_product_test_copy_list_prepare(divoom_cJSON *file_list,
	uint8_t *needs_download, int *file_count, int *download_count)
{
	char local_path[DIVOOM_PT_CRC_PATH_MAX];
	char previous_path[DIVOOM_PT_CRC_PATH_MAX];
	char url[DIVOOM_PT_CRC_URL_MAX];
	char previous_url[DIVOOM_PT_CRC_URL_MAX];
	uint32_t crc32;
	uint32_t previous_crc;
	int count;
	int i;
	int j;

	if (!needs_download || !file_count || !download_count)
		return 0;
	*file_count = 0;
	*download_count = 0;
	memset(needs_download, 0, DIVOOM_PT_COPY_FILE_MAX);
	if (!file_list)
		return 1;
	if (!divoom_product_test_json_type(file_list, divoom_cJSON_Array))
		return 0;
	count = divoom_cJSON_GetArraySize(file_list);
	if (count < 0 || count > DIVOOM_PT_COPY_FILE_MAX)
		return 0;
	for (i = 0; i < count; i++) {
		divoom_cJSON *item = divoom_cJSON_GetArrayItem(file_list, i);

		if (!divoom_product_test_copy_item_parse(item, local_path,
				sizeof(local_path), url, sizeof(url), &crc32)) {
			divoom_print("product_test: invalid CopyFileList item %d\n", i);
			return 0;
		}
		for (j = 0; j < i; j++) {
			divoom_cJSON *previous = divoom_cJSON_GetArrayItem(file_list, j);

			if (!divoom_product_test_copy_item_parse(previous, previous_path,
					sizeof(previous_path), previous_url,
					sizeof(previous_url), &previous_crc) ||
			    strcmp(local_path, previous_path) == 0) {
				divoom_print("product_test: duplicate CopyFileList target %s\n",
					local_path);
				return 0;
			}
		}
		if (divoom_product_test_copy_local_matches(local_path, crc32)) {
			divoom_print("product_test: CopyFileList current %s crc=%08x\n",
				local_path, (unsigned)crc32);
			continue;
		}
		needs_download[i] = 1;
		(*download_count)++;
	}
	*file_count = count;
	return 1;
}

static int divoom_product_test_copy_list_apply(divoom_cJSON *file_list,
	const uint8_t *needs_download, int file_count,
	divoom_pt_crc_apply_ctx *progress, int *applied)
{
	char local_path[DIVOOM_PT_CRC_PATH_MAX];
	char temp_path[DIVOOM_PT_CRC_PATH_MAX];
	char url[DIVOOM_PT_CRC_URL_MAX];
	divoom_port_stat_t stat_info;
	uint32_t crc32;
	int i;

	if (!needs_download || !progress || !applied)
		return 0;
	for (i = 0; i < file_count; i++) {
		divoom_cJSON *item;

		if (!needs_download[i])
			continue;
		item = divoom_cJSON_GetArrayItem(file_list, i);
		if (!divoom_product_test_copy_item_parse(item, local_path,
				sizeof(local_path), url, sizeof(url), &crc32))
			return 0;
		/* CRC is used to decide whether the local file needs replacement. */
		if (divoom_product_test_copy_local_matches(local_path, crc32)) {
			divoom_product_test_crc_report_progress(progress);
			continue;
		}
		if (divoom_port_stat(local_path, &stat_info) == 0 && stat_info.is_dir) {
			divoom_print("product_test: CopyFileList target is directory %s\n",
				local_path);
			return 0;
		}
		if (snprintf(temp_path, sizeof(temp_path), "%s.divoom_download",
			local_path) >= (int)sizeof(temp_path) ||
		    !divoom_product_test_mk_parent(local_path))
			return 0;
		divoom_port_remove_file(temp_path);
		if (divoom_http_client_down_file(temp_path, url, 0, NULL) != 1 ||
		    !divoom_product_test_crc_commit_file(temp_path, local_path)) {
			divoom_print("product_test: CopyFileList download fail %s\n",
				local_path);
			divoom_port_remove_file(temp_path);
			return 0;
		}
		divoom_port_remove_file(temp_path);
		*applied = 1;
		divoom_print("product_test: CopyFileList updated %s crc=%08x\n",
			local_path, (unsigned)crc32);
		divoom_product_test_crc_report_progress(progress);
	}
	return 1;
}

static int divoom_product_test_crc_read_version(void)
{
	char *file_data;
	divoom_cJSON *root = NULL;
	divoom_cJSON *version_item;
	int value = 0;
	int file_len = 0;

	file_data = divoom_app_com_read_file((char *)DIVOOM_PT_CRC_VERSION_PATH,
		&file_len);
	if (!file_data || file_len <= 0 || file_len > 128) {
		if (file_data)
			divoom_system_free(file_data);
		return 0;
	}
	root = divoom_cJSON_Parse(file_data);
	divoom_system_free(file_data);
	if (!divoom_product_test_json_type(root, divoom_cJSON_Object))
		goto out;
	version_item = divoom_cJSON_GetObjectItem(root, "version");
	if (!divoom_product_test_json_type(version_item, divoom_cJSON_Number) ||
	    version_item->valueint <= 0 ||
	    version_item->valuedouble != (double)version_item->valueint)
		goto out;
	value = version_item->valueint;
out:
	if (root)
		divoom_cJSON_Delete(root);
	return value;
}

static int divoom_product_test_crc_write_version(int version)
{
	char text[24];
	void *fp;
	int length;
	int ok;

	length = snprintf(text, sizeof(text), "{\"version\":%d}\n", version);
	if (length <= 0 || length >= (int)sizeof(text))
		return 0;
	divoom_app_com_del_file((char *)DIVOOM_PT_CRC_VERSION_TMP_PATH);
	fp = divoom_port_file_open(DIVOOM_PT_CRC_VERSION_TMP_PATH, "wb");
	if (!fp)
		return 0;
	ok = divoom_port_file_write(text, 1, (size_t)length, fp) ==
		(size_t)length;
	if (divoom_port_file_close(fp) != 0)
		ok = 0;
	if (!ok)
		return 0;
	return divoom_product_test_crc_commit_file(
		DIVOOM_PT_CRC_VERSION_TMP_PATH, DIVOOM_PT_CRC_VERSION_PATH);
}

static int divoom_product_test_crc_json_entry(divoom_cJSON *object,
		char *url, size_t url_size, char *md5_text, size_t md5_size,
		int *version)
{
	divoom_cJSON *url_item;
	divoom_cJSON *md5_item;
	divoom_cJSON *version_item;
	size_t i;

	if (!divoom_product_test_json_type(object, divoom_cJSON_Object))
		return 0;
	url_item = divoom_cJSON_GetObjectItem(object, "url");
	md5_item = divoom_cJSON_GetObjectItem(object, "md5");
	version_item = divoom_cJSON_GetObjectItem(object, "version");
	if (!divoom_product_test_json_type(url_item, divoom_cJSON_String) ||
	    !url_item->valuestring || !url_item->valuestring[0] ||
	    strlen(url_item->valuestring) >= url_size ||
	    !divoom_product_test_json_type(md5_item, divoom_cJSON_String) ||
	    !md5_item->valuestring || strlen(md5_item->valuestring) != 32U ||
	    md5_size < DIVOOM_PT_CRC_MD5_TEXT_SIZE ||
	    !divoom_product_test_json_type(version_item, divoom_cJSON_Number) ||
	    version_item->valueint <= 0 ||
	    version_item->valuedouble != (double)version_item->valueint)
		return 0;
	for (i = 0; i < 32U; i++) {
		if (divoom_product_test_crc_hex_value(md5_item->valuestring[i]) < 0)
			return 0;
	}
	strcpy(url, url_item->valuestring);
	strcpy(md5_text, md5_item->valuestring);
	*version = version_item->valueint;
	return 1;
}

static int divoom_product_test_get_listguide(divoom_update_json_listguide_info *guide,
						    int *download_flag)
{
	divoom_cJSON *root = NULL;
	divoom_cJSON *customize_flag_item, *prog_flag_item;
	divoom_cJSON *customize_item, *prog_item, *jl_prog_item;
	char *file_info;

	if (!guide || !download_flag)
		return 0;
	memset(guide, 0, sizeof(*guide));
	*download_flag = DIVOOM_PT_DL_CONNECTING;
	file_info = divoom_http_client_get_srv((char *)DIVOOM_TEST_SERVER_HTTP_LIST_GUIDE_ADDR);
	if (file_info == NULL)
		return 0;

	root = divoom_cJSON_Parse(file_info);
	if (!divoom_product_test_json_type(root, divoom_cJSON_Object)) {
		divoom_system_free(file_info);
		if (root)
			divoom_cJSON_Delete(root);
		return 0;
	}
	customize_flag_item = divoom_cJSON_GetObjectItem(root, "CustomizeNewFlag");
	prog_flag_item = divoom_cJSON_GetObjectItem(root, "ProgFlag");
	if (!divoom_product_test_json_type(customize_flag_item, divoom_cJSON_Number) ||
	    !divoom_product_test_json_type(prog_flag_item, divoom_cJSON_Number) ||
	    customize_flag_item->valuedouble != (double)customize_flag_item->valueint ||
	    prog_flag_item->valuedouble != (double)prog_flag_item->valueint ||
	    (customize_flag_item->valueint != 0 && customize_flag_item->valueint != 1) ||
	    (prog_flag_item->valueint != 0 && prog_flag_item->valueint != 1))
		goto invalid;

	guide->CustomizeFlag = customize_flag_item->valueint;
	guide->prog_flag = prog_flag_item->valueint;
	jl_prog_item = divoom_cJSON_GetObjectItem(root, "JL_Prog");
	if (jl_prog_item) {
		if (!divoom_product_test_json_type(jl_prog_item, divoom_cJSON_String) ||
		    !jl_prog_item->valuestring || !jl_prog_item->valuestring[0] ||
		    strlen(jl_prog_item->valuestring) >= sizeof(guide->jl_prog_json))
			goto invalid;
		divoom_cJSON_GetObjectStringN(root, "JL_Prog", guide->jl_prog_json,
					    sizeof(guide->jl_prog_json));
		guide->jl_prog_flag = 1;
	}
	divoom_print("product_test: listguide customize=%d prog=%d jl=%d\n",
		     guide->CustomizeFlag, guide->prog_flag, guide->jl_prog_flag);
	if (guide->CustomizeFlag) {
		customize_item = divoom_cJSON_GetObjectItem(root, "Customize");
		if (!divoom_product_test_json_type(customize_item, divoom_cJSON_String) ||
		    !customize_item->valuestring || !customize_item->valuestring[0] ||
		    strlen(customize_item->valuestring) >= sizeof(guide->customize_json))
			goto invalid;
		divoom_cJSON_GetObjectStringN(root, "Customize", guide->customize_json,
					    sizeof(guide->customize_json));
		*download_flag = DIVOOM_PT_DL_DOWNLOADING;
	}
	if (guide->jl_prog_flag)
		*download_flag = DIVOOM_PT_DL_DOWNLOADING;
	if (guide->prog_flag) {
		prog_item = divoom_cJSON_GetObjectItem(root, "Prog");
		if (!divoom_product_test_json_type(prog_item, divoom_cJSON_String) ||
		    !prog_item->valuestring || !prog_item->valuestring[0] ||
		    strlen(prog_item->valuestring) >= sizeof(guide->prog_json))
			goto invalid;
		divoom_cJSON_GetObjectStringN(root, "Prog", guide->prog_json,
					    sizeof(guide->prog_json));
		*download_flag = DIVOOM_PT_DL_DOWNLOADING;
	}
	if (*download_flag == DIVOOM_PT_DL_CONNECTING)
		*download_flag = DIVOOM_PT_DL_NONE;
	divoom_cJSON_Delete(root);
	divoom_system_free(file_info);
	return 1;

invalid:
	divoom_print("product_test: invalid listguide response\n");
	divoom_cJSON_Delete(root);
	divoom_system_free(file_info);
	return 0;
}

static int divoom_download_customize(const char *url, int *download_flag,
				     int *download_cur, int *download_total)
{
	divoom_cJSON *root = NULL;
	divoom_cJSON *base_item, *raw_item, *update_item, *copy_list;
	char base_url[DIVOOM_PT_CRC_URL_MAX];
	char raw_url[DIVOOM_PT_CRC_URL_MAX];
	char update_url[DIVOOM_PT_CRC_URL_MAX];
	char raw_md5[DIVOOM_PT_CRC_MD5_TEXT_SIZE];
	char update_md5[DIVOOM_PT_CRC_MD5_TEXT_SIZE];
	uint8_t copy_needs_download[DIVOOM_PT_COPY_FILE_MAX];
	char *file_info;
	int raw_version, update_version, local_version;
	int copy_file_count, copy_download_count;
	int need_bootstrap;
	int crc_applied = 0;
	int copy_applied = 0;
	int result = DIVOOM_PT_CUSTOMIZE_ERROR;
	divoom_pt_crc_apply_ctx ctx;

	if (!url || !url[0])
		return DIVOOM_PT_CUSTOMIZE_ERROR;

	divoom_product_test_mk_parent(DIVOOM_PT_CUSTOMIZE_JSON_PATH);
	divoom_app_com_del_file((char *)DIVOOM_PT_CUSTOMIZE_JSON_PATH);
	if (divoom_http_client_down_file((char *)DIVOOM_PT_CUSTOMIZE_JSON_PATH,
					 (char *)url, 0, NULL) != 1) {
		divoom_print("product_test: customize json dl fail\n");
		divoom_app_com_del_file((char *)DIVOOM_PT_CUSTOMIZE_JSON_PATH);
		return DIVOOM_PT_CUSTOMIZE_ERROR;
	}

	file_info = divoom_app_com_read_file(DIVOOM_PT_CUSTOMIZE_JSON_PATH, NULL);
	if (file_info == NULL) {
		divoom_app_com_del_file((char *)DIVOOM_PT_CUSTOMIZE_JSON_PATH);
		return DIVOOM_PT_CUSTOMIZE_ERROR;
	}
	root = divoom_cJSON_Parse(file_info);
	divoom_system_free(file_info);
	if (root == NULL) {
		divoom_app_com_del_file((char *)DIVOOM_PT_CUSTOMIZE_JSON_PATH);
		return DIVOOM_PT_CUSTOMIZE_ERROR;
	}

	if (!divoom_product_test_json_type(root, divoom_cJSON_Object) ||
	    !divoom_product_test_json_type(
		base_item = divoom_cJSON_GetObjectItem(root, "base_url_file"),
		divoom_cJSON_String) ||
	    !base_item->valuestring || !base_item->valuestring[0] ||
	    strlen(base_item->valuestring) >= sizeof(base_url) ||
	    !divoom_product_test_crc_json_entry(
		raw_item = divoom_cJSON_GetObjectItem(root, "raw_crc_file"),
		raw_url, sizeof(raw_url), raw_md5, sizeof(raw_md5), &raw_version) ||
	    !divoom_product_test_crc_json_entry(
		update_item = divoom_cJSON_GetObjectItem(root, "update_crc_file"),
		update_url, sizeof(update_url), update_md5, sizeof(update_md5),
		&update_version)) {
		divoom_print("product_test: invalid crc customize schema\n");
		goto cleanup;
	}
	copy_list = divoom_cJSON_GetObjectItem(root, "CopyFileList");
	if (!divoom_product_test_copy_list_prepare(copy_list,
			copy_needs_download, &copy_file_count,
			&copy_download_count)) {
		divoom_print("product_test: invalid CopyFileList\n");
		goto cleanup;
	}
	strcpy(base_url, base_item->valuestring);
	if (download_flag)
		*download_flag = DIVOOM_PT_DL_DOWNLOADING;
	if (download_cur)
		*download_cur = 0;
	if (download_total)
		*download_total = 0;
	memset(&ctx, 0, sizeof(ctx));
	ctx.base_url = base_url;
	ctx.download_cur = download_cur;
	ctx.download_total = download_total;

	local_version = divoom_product_test_crc_read_version();
	need_bootstrap = !divoom_port_file_exists(DIVOOM_PT_CRC_LOCAL_PATH) ||
		local_version <= 0 ||
		!divoom_product_test_crc_compare_manifests(
			DIVOOM_PT_CRC_LOCAL_PATH, DIVOOM_PT_CRC_LOCAL_PATH, NULL);
	if (need_bootstrap) {
		divoom_app_com_del_file((char *)DIVOOM_PT_CRC_RAW_TMP_PATH);
		if (divoom_http_client_down_file((char *)DIVOOM_PT_CRC_RAW_TMP_PATH,
				raw_url, 0, NULL) != 1 ||
		    !divoom_product_test_crc_check_md5(
			DIVOOM_PT_CRC_RAW_TMP_PATH, raw_md5) ||
		    !divoom_product_test_crc_compare_manifests(
			DIVOOM_PT_CRC_RAW_TMP_PATH,
			DIVOOM_PT_CRC_RAW_TMP_PATH, NULL) ||
		    !divoom_product_test_crc_commit_file(DIVOOM_PT_CRC_RAW_TMP_PATH,
				DIVOOM_PT_CRC_LOCAL_PATH) ||
		    !divoom_product_test_crc_write_version(raw_version)) {
			divoom_print("product_test: bootstrap raw crc failed\n");
			goto cleanup;
		}
		local_version = raw_version;
	}

	if (update_version > local_version) {
		divoom_app_com_del_file((char *)DIVOOM_PT_CRC_UPDATE_TMP_PATH);
		if (divoom_http_client_down_file((char *)DIVOOM_PT_CRC_UPDATE_TMP_PATH,
					update_url, 0, NULL) != 1 ||
		    !divoom_product_test_crc_check_md5(
			DIVOOM_PT_CRC_UPDATE_TMP_PATH, update_md5)) {
			divoom_print("product_test: update crc download failed\n");
			goto cleanup;
		}

		ctx.action = DIVOOM_PT_CRC_COUNT_DOWNLOAD;
		if (!divoom_product_test_crc_compare_manifests(
				DIVOOM_PT_CRC_LOCAL_PATH,
				DIVOOM_PT_CRC_UPDATE_TMP_PATH, &ctx))
			goto cleanup;
		ctx.total = ctx.actionable + copy_download_count;
		ctx.actionable = 0;
		if (download_total)
			*download_total = ctx.total;

		ctx.action = DIVOOM_PT_CRC_DELETE;
		if (!divoom_product_test_crc_compare_manifests(
				DIVOOM_PT_CRC_LOCAL_PATH,
				DIVOOM_PT_CRC_UPDATE_TMP_PATH, &ctx) || ctx.failed)
			goto cleanup;
		ctx.action = DIVOOM_PT_CRC_DOWNLOAD;
		if (!divoom_product_test_crc_compare_manifests(
				DIVOOM_PT_CRC_LOCAL_PATH,
				DIVOOM_PT_CRC_UPDATE_TMP_PATH, &ctx) || ctx.failed)
			goto cleanup;

		if (!divoom_product_test_crc_commit_file(
				DIVOOM_PT_CRC_UPDATE_TMP_PATH,
				DIVOOM_PT_CRC_LOCAL_PATH) ||
		    !divoom_product_test_crc_write_version(update_version)) {
			divoom_print("product_test: crc ledger commit failed\n");
			goto cleanup;
		}
		crc_applied = 1;
	} else {
		ctx.total = copy_download_count;
		if (download_total)
			*download_total = ctx.total;
	}

	if (!divoom_product_test_copy_list_apply(copy_list,
			copy_needs_download, copy_file_count, &ctx, &copy_applied))
		goto cleanup;
	result = (crc_applied || copy_applied) ?
		DIVOOM_PT_CUSTOMIZE_APPLIED : DIVOOM_PT_CUSTOMIZE_NONE;

cleanup:
	divoom_app_com_del_file((char *)DIVOOM_PT_CRC_RAW_TMP_PATH);
	divoom_app_com_del_file((char *)DIVOOM_PT_CRC_UPDATE_TMP_PATH);
	divoom_app_com_del_file((char *)DIVOOM_PT_CRC_VERSION_TMP_PATH);
	divoom_app_com_del_file((char *)DIVOOM_PT_CUSTOMIZE_JSON_PATH);
	divoom_cJSON_Delete(root);
	return result;
}

static const char *divoom_product_test_fw_result_name(int result)
{
	switch (result) {
	case DIVOOM_OTA_SERVER_UPGRADE_FILE_OK:
		return "OK";
	case DIVOOM_OTA_SERVER_UPGRADE_FILE_ERR:
		return "ERR";
	case DIVOOM_OTA_SERVER_UPGRADE_FILE_READY:
		return "READY";
	case DIVOOM_OTA_SERVER_UPGRADE_FILE_ERROR_URL:
		return "URL";
	case DIVOOM_OTA_SERVER_UPGRADE_FILE_DOWNLOADING:
		return "DL";
	default:
		return "?";
	}
}

static void divoom_product_test_shutdown_wifi(void)
{
	s_pt_wifi_shutdown = 1;
	s_pt_wifi_retry_pending = 0;
	s_pt_wifi_connected = 0;
#ifndef DIVOOM_SIMULATOR
	int ret;

	ret = aw_wifi_stop();
	divoom_print("product_test: aw_wifi_stop ret=%d\n", ret);
	ret = aw_wifi_deinit();
	divoom_print("product_test: aw_wifi_deinit ret=%d\n", ret);
#endif
}

static void divoom_product_test_wifi_status_callback(DIVOOM_WIFI_STATUS status,
	void *data)
{
	(void)data;
	if (status != DIVOOM_WIFI_NO_SSID
	    && status != DIVOOM_WIFI_OTHER_ERR
	    && status != DIVOOM_WIFI_DISCONNECTED)
		return;

	s_pt_wifi_connected = 0;
	if (!s_pt_active || s_pt_wifi_shutdown)
		return;
	if (!s_pt_wifi_retry_pending)
		divoom_print("product_test: factory AP failed status=%d; retry in %d ms\n",
			     status, DIVOOM_PT_WIFI_RETRY_MS);
	s_pt_wifi_retry_tick = divoom_system_get_ticks();
	__sync_synchronize();
	s_pt_wifi_retry_pending = 1;
}

static void divoom_product_test_wifi_retry_poll(void)
{
	uint32_t now;

	if (!s_pt_wifi_retry_pending)
		return;
	if (!s_pt_active || s_pt_wifi_shutdown || s_pt_wifi_connected) {
		s_pt_wifi_retry_pending = 0;
		return;
	}
	now = divoom_system_get_ticks();
	if ((uint32_t)(now - s_pt_wifi_retry_tick) < DIVOOM_PT_WIFI_RETRY_MS)
		return;

	s_pt_wifi_retry_pending = 0;
	s_pt_download_file_flag = DIVOOM_PT_DL_CONNECTING;
	divoom_print("product_test: reconnect factory AP %s\n",
		     DIVOOM_PRODUCT_TEST_WIFI_SSID);
	divoom_wifi_connect_ssid(DIVOOM_PRODUCT_TEST_WIFI_SSID,
		DIVOOM_PRODUCT_TEST_WIFI_PASSWD);
}

uint8 divoom_product_test_wifi_model_init(void)
{
	return divoom_wifi_model_init(divoom_product_test_wifi_status_callback, 1);
}

static void divoom_product_test_release_update_network(void)
{
	divoom_print("product_test: update complete; stop WiFi\n");
	divoom_product_test_shutdown_wifi();
}

static int divoom_product_test_reboot_after_customize(void)
{
	if (!divoom_product_test_is_upgrade_aging() || !s_pt_customize_applied)
		return 0;
	if (!divoom_onoff_arm_upgrade_aging_boot()) {
		s_pt_download_file_flag = DIVOOM_PT_DL_LISTGUIDE_FAIL;
		divoom_print("product_test: arm customize reboot flag failed; retry\n");
		divoom_product_test_schedule_ota_retry();
		return 1;
	}

	s_pt_download_file_flag = DIVOOM_PT_DL_DONE;
	divoom_print("product_test: customize applied, no prog upgrade; stop WiFi and reboot\n");
	divoom_product_test_release_update_network();
	divoom_system_delay_ms(200);
	divoom_system_reboot();
	return 1;
}

static int divoom_product_test_arm_upgrade_reboot(int *reboot_mode)
{
	int mode = -1;
	int marker_ok = 1;

	if (divoom_product_test_is_upgrade_aging()) {
		mode = DIVOOM_PRODUCT_TEST_BOOT_UPGRADE_AGING;
		marker_ok = divoom_onoff_arm_upgrade_aging_boot();
	} else if (!s_pt_menu_selected) {
		/* 未选择菜单时，升级重启后仍回产测选择页。 */
		mode = DIVOOM_PRODUCT_TEST_BOOT_NORMAL;
		marker_ok = divoom_onoff_arm_product_test_menu_boot();
	}
	if (reboot_mode)
		*reboot_mode = mode;
	if (!marker_ok)
		return 0;
	s_pt_reboot_marker_armed = mode >= 0;
	return 1;
}

static int divoom_product_test_jl_sha1_valid(const char *sha1)
{
	int i;

	if (!sha1 || strlen(sha1) != 40U)
		return 0;
	for (i = 0; i < 40; i++) {
		if (divoom_product_test_crc_hex_value(sha1[i]) < 0)
			return 0;
	}
	return 1;
}

static int divoom_product_test_get_jl_update_info(
	const char *url, divoom_general_json_get_update_file_info_respone *info)
{
	divoom_cJSON *root = NULL;
	divoom_cJSON *version_item;
	divoom_cJSON *file_item;
	divoom_cJSON *sha1_item;
	divoom_cJSON *hardware_item;
	char *file_info;
	int valid = 0;

	if (!url || !url[0] || !info)
		return 0;
	memset(info, 0, sizeof(*info));
	file_info = divoom_http_client_get_srv((char *)url);
	if (!file_info)
		return 0;
	root = divoom_cJSON_Parse(file_info);
	if (!divoom_product_test_json_type(root, divoom_cJSON_Object))
		goto out;
	version_item = divoom_cJSON_GetObjectItem(root, "Version");
	file_item = divoom_cJSON_GetObjectItem(root, "FileId");
	sha1_item = divoom_cJSON_GetObjectItem(root, "Sha1");
	hardware_item = divoom_cJSON_GetObjectItem(root, "Hardware");
	if (!divoom_product_test_json_type(version_item, divoom_cJSON_Number) ||
	    version_item->valueint <= 0 ||
	    version_item->valuedouble != (double)version_item->valueint ||
	    !divoom_ota_jl_version_is_valid((uint32_t)version_item->valueint) ||
	    !divoom_product_test_json_type(file_item, divoom_cJSON_String) ||
	    !file_item->valuestring || !file_item->valuestring[0] ||
	    strlen(file_item->valuestring) >= sizeof(info->file_id) ||
	    !divoom_product_test_json_type(sha1_item, divoom_cJSON_String) ||
	    !divoom_product_test_jl_sha1_valid(sha1_item->valuestring))
		goto out;
	info->type = 1;
	info->version = version_item->valueint;
	if (divoom_product_test_json_type(hardware_item, divoom_cJSON_Number) &&
	    hardware_item->valuedouble == (double)hardware_item->valueint)
		info->hardware = hardware_item->valueint;
	strcpy(info->file_id, file_item->valuestring);
	strcpy(info->sha1_sum, sha1_item->valuestring);
	valid = 1;

out:
	if (!valid)
		divoom_print("product_test: invalid JL firmware guide %s\n", url);
	if (root)
		divoom_cJSON_Delete(root);
	divoom_system_free(file_info);
	return valid;
}

static void divoom_product_test_jl_progress(void *user_data, uint8 percent)
{
	(void)user_data;
	s_pt_download_file_flag = DIVOOM_PT_DL_JL_UPDATING;
	divoom_product_test_on_ota_percent(percent);
}

static void divoom_product_test_jl_result(void *user_data, int success)
{
	(void)user_data;
	s_pt_jl_update_started = 0;
	if (!success) {
		if (s_pt_reboot_marker_armed)
			divoom_onoff_clear_upgrade_aging_boot();
		s_pt_reboot_marker_armed = 0;
		s_pt_download_file_flag = DIVOOM_PT_DL_LISTGUIDE_FAIL;
		divoom_print("product_test: JL upgrade failed; retry in %d ms\n",
			DIVOOM_PT_OTA_RETRY_MS);
		divoom_product_test_schedule_ota_retry();
		return;
	}
	s_pt_download_file_flag = DIVOOM_PT_DL_DONE;
	s_pt_fw_percent = 100;
	divoom_print("product_test: JL upgrade complete; stop WiFi and reboot\n");
	divoom_product_test_release_update_network();
	divoom_system_delay_ms(200);
	divoom_system_reboot();
}

static int divoom_product_test_start_jl_update(void)
{
	divoom_general_json_get_update_file_info_respone info;
	uint32_t local_version;
	int reboot_mode = -1;
	int ret;

	if (!s_pt_listguide.jl_prog_flag || !s_pt_listguide.jl_prog_json[0])
		return 0;
	if (s_pt_jl_update_started)
		return 1;
	if (!divoom_product_test_get_jl_update_info(
			s_pt_listguide.jl_prog_json, &info))
		return -1;
	local_version = divoom_ota_jl_get_local_version();
	if (divoom_ota_jl_version_is_valid(local_version) &&
	    (uint32_t)info.version <= local_version) {
		divoom_print("product_test: JL current local=%u server=%u\n",
			(unsigned)local_version, (unsigned)info.version);
		return 0;
	}
	if (!divoom_product_test_arm_upgrade_reboot(&reboot_mode)) {
		divoom_print("product_test: arm JL reboot flag mode=%d failed\n",
			reboot_mode);
		return -1;
	}
	s_pt_download_file_flag = DIVOOM_PT_DL_JL_UPDATING;
	s_pt_fw_percent = 0;
	s_pt_jl_update_started = 1;
	ret = divoom_ota_jl_start_local_update(&info,
		divoom_product_test_jl_progress, divoom_product_test_jl_result, NULL);
	if (ret == 1) {
		divoom_print("product_test: start JL upgrade local=%u server=%u reboot_mode=%d\n",
			(unsigned)local_version, (unsigned)info.version, reboot_mode);
		return 1;
	}
	s_pt_jl_update_started = 0;
	if (s_pt_reboot_marker_armed)
		divoom_onoff_clear_upgrade_aging_boot();
	s_pt_reboot_marker_armed = 0;
	return ret == 0 ? 0 : -1;
}

static void divoom_product_test_complete_update_chain(void)
{
	s_pt_download_file_flag = (s_pt_customize_applied || s_pt_fw_checked) ?
		DIVOOM_PT_DL_DONE : DIVOOM_PT_DL_NONE;
	if (divoom_product_test_reboot_after_customize())
		return;
	divoom_product_test_release_update_network();
	divoom_print("product_test: update chain done flag=%d customize=%d fw=%s jl=%u\n",
		s_pt_download_file_flag, s_pt_customize_applied,
		divoom_product_test_fw_result_name(s_pt_fw_result),
		(unsigned)divoom_ota_jl_get_local_version());
}

static void divoom_product_test_continue_after_prog(void)
{
	int jl_result = divoom_product_test_start_jl_update();

	if (jl_result > 0)
		return;
	if (jl_result < 0) {
		s_pt_download_file_flag = DIVOOM_PT_DL_LISTGUIDE_FAIL;
		divoom_print("product_test: JL check/start failed; retry in %d ms\n",
			DIVOOM_PT_OTA_RETRY_MS);
		divoom_product_test_schedule_ota_retry();
		return;
	}
	divoom_product_test_complete_update_chain();
}

static void *divoom_product_test_ota_task(void *arg)
{
	int customize_result = DIVOOM_PT_CUSTOMIZE_NONE;

	(void)arg;
	divoom_print("product_test: ota task start\n");
	s_pt_customize_fail = 0;
	s_pt_fw_checked = 0;
	s_pt_fw_result = 0;
	for (;;) {
		if (!divoom_product_test_get_listguide(&s_pt_listguide,
				&s_pt_download_file_flag)) {
			s_pt_download_file_flag = DIVOOM_PT_DL_LISTGUIDE_FAIL;
			divoom_print("product_test: listguide failed; retry in %d ms\n",
				     DIVOOM_PT_OTA_RETRY_MS);
			divoom_system_delay_ms(DIVOOM_PT_OTA_RETRY_MS);
			continue;
		}
		customize_result = DIVOOM_PT_CUSTOMIZE_NONE;
		if (s_pt_listguide.CustomizeFlag) {
			s_pt_customize_updating = 1;
            /* The update may replace power_on.mp4: release its read handle
             * before any resource writes, while the UI stays responsive. */
            if (!divoom_product_test_ui_pause_aging_video(3000)) {
                s_pt_customize_updating = 0;
                s_pt_customize_fail = 1;
                divoom_product_test_ui_resume_aging_video();
                divoom_print("product_test: aging video busy; defer resource update\n");
                divoom_system_delay_ms(DIVOOM_PT_OTA_RETRY_MS);
                continue;
            }
			customize_result = divoom_download_customize(
				s_pt_listguide.customize_json, &s_pt_download_file_flag,
				&s_pt_download_cur, &s_pt_download_total);
			s_pt_customize_updating = 0;
            divoom_product_test_ui_resume_aging_video();
			if (customize_result == DIVOOM_PT_CUSTOMIZE_ERROR) {
				s_pt_customize_fail = 1;
				divoom_print("product_test: customize failed; reload listguide in %d ms\n",
					     DIVOOM_PT_OTA_RETRY_MS);
				divoom_system_delay_ms(DIVOOM_PT_OTA_RETRY_MS);
				continue;
			}
		}
		s_pt_customize_fail = 0;
		break;
	}
	if (customize_result == DIVOOM_PT_CUSTOMIZE_APPLIED)
		s_pt_customize_applied = 1;
	if (s_pt_listguide.prog_flag && s_pt_listguide.prog_json[0]) {
		s_pt_download_file_flag = DIVOOM_PT_DL_DOWNLOADING;
		s_pt_download_total = 0;
		s_pt_download_cur = 0;
		s_pt_fw_percent = 0;
		divoom_print("product_test: start firmware check/download\n");
		divoom_ota_server_start_local_update(s_pt_listguide.prog_json);
		return NULL;
	}
	divoom_product_test_continue_after_prog();
	return NULL;
}

static void divoom_product_test_start_ota(void)
{
	/* The landing page must remain available before any destructive operation. */
	if (!s_pt_menu_selected && !divoom_product_test_is_upgrade_aging())
		return;
	{
		int owner = __sync_val_compare_and_swap(&s_pt_update_owner, 0, 1);
		if (owner != 0 && owner != 1)
			return;
	}
	if (s_pt_ota_started)
		return;
	s_pt_ota_started = 1;
	if (!divoom_system_create_task(divoom_product_test_ota_task, "pt_ota",
				       NULL, DIVOOM_TASK_STACK_OTA_SERVER_LOCAL)) {
		s_pt_ota_started = 0;
		divoom_print("product_test: create ota task failed\n");
	}
}

int divoom_product_test_get_download_file_flag(void)
{
	return s_pt_download_file_flag;
}

int divoom_product_test_get_wifi_up(void)
{
	return s_pt_wifi_up;
}

void divoom_product_test_get_download_progress(int *cur, int *total)
{
	if (cur)
		*cur = s_pt_download_cur;
	if (total)
		*total = s_pt_download_total;
}

int divoom_product_test_is_customize_updating(void)
{
	return s_pt_customize_updating;
}

int divoom_product_test_get_firmware_percent(void)
{
	return s_pt_fw_percent;
}

int divoom_product_test_get_firmware_result(void)
{
	return s_pt_fw_result;
}

int divoom_product_test_get_customize_fail(void)
{
	return s_pt_customize_fail;
}

int divoom_product_test_get_firmware_checked(void)
{
	return s_pt_fw_checked;
}

void divoom_product_test_on_ota_percent(int percent)
{
	if (percent < 0)
		percent = 0;
	if (percent > 100)
		percent = 100;
	s_pt_fw_percent = percent;
}

void divoom_product_test_on_ota_unzip_percent(int percent)
{
	s_pt_download_file_flag = DIVOOM_PT_DL_UNZIPPING;
	divoom_product_test_on_ota_percent(percent);
}

static void divoom_product_test_try_flash_staged(void)
{
	int reboot_mode = -1;

	if (!s_pt_allow_flash || s_pt_flash_started)
		return;
	if (s_pt_fw_result != DIVOOM_OTA_SERVER_UPGRADE_FILE_READY)
		return;
	if (!__sync_bool_compare_and_swap(&s_pt_flash_started, 0, 1))
		return;
	if (!divoom_product_test_arm_upgrade_reboot(&reboot_mode)) {
		s_pt_flash_started = 0;
		s_pt_download_file_flag = DIVOOM_PT_DL_LISTGUIDE_FAIL;
		divoom_print("product_test: arm reboot flag mode=%d failed; retry\n",
			reboot_mode);
		divoom_product_test_schedule_ota_retry();
		return;
	}
	s_pt_download_file_flag = DIVOOM_PT_DL_FLASHING;
	s_pt_fw_percent = 0;
	divoom_print("product_test: start prog flash reboot_mode=%d selected=%d next=%d\n",
		reboot_mode, s_pt_menu_selected, s_pt_next_selected);
	divoom_product_test_shutdown_wifi();
	divoom_ota_server_active_update();
}

void divoom_product_test_on_wifi_page(void)
{
	s_pt_allow_flash = 1;
	divoom_product_test_try_flash_staged();
}

int divoom_product_test_start_usr_check(void)
{
    int owner = __sync_val_compare_and_swap(&s_pt_update_owner, 0, 3);
    if (owner != 0 && owner != 3) return 0;
    divoom_product_test_usr_check_start();
    return 1;
}

void divoom_product_test_select_next(void)
{
	/* Reserve before exposing menu_selected to the concurrent GOT_IP task. */
	if (!s_pt_next_selected)
		divoom_product_test_start_usr_check();
	/* Next 沿用原产测流程：只 staging，走到最后 WiFi 项时才允许刷写。 */
	s_pt_next_selected = 1;
	s_pt_menu_selected = 1;
	__sync_synchronize();
	if (!s_pt_flash_started)
		s_pt_allow_flash = 0;
	divoom_print("product_test: Next selected; defer prog flash to WiFi page\n");
	if (divoom_wifi_model_is_network_ready())
		divoom_product_test_start_ota();
}

int divoom_product_test_begin_format(void)
{
    if (s_pt_update_owner == 3) {
        divoom_product_test_usr_check_status check;
        divoom_product_test_usr_check_get_status(&check);
        if ((check.phase == DIVOOM_PT_USR_CHECK_DONE || check.phase == DIVOOM_PT_USR_CHECK_FAILED) &&
            (check.error_files || check.phase == DIVOOM_PT_USR_CHECK_FAILED || check.maintenance_required)) {
            divoom_product_test_stop_aging_audio();
            __sync_bool_compare_and_swap(&s_pt_update_owner, 3, 2);
        }
    }
	if (s_pt_update_owner != 2 &&
	    !__sync_bool_compare_and_swap(&s_pt_update_owner, 0, 2))
		return 0;
	s_pt_allow_flash = 0;
	s_pt_menu_selected = 1;
	s_pt_wifi_shutdown = 1; /* Disable factory AP retry, keep the STA alive. */
	s_pt_wifi_retry_pending = 0;
	return 1;
}

void divoom_product_test_end_format(void)
{
	if (s_pt_update_owner != 2)
		return;
	s_pt_menu_selected = 0;
	s_pt_wifi_shutdown = 0;
	s_pt_allow_flash = 1;
	__sync_synchronize();
	s_pt_update_owner = 0;
	divoom_wifi_connect_ssid(DIVOOM_PRODUCT_TEST_WIFI_SSID,
		DIVOOM_PRODUCT_TEST_WIFI_PASSWD);
}

void divoom_product_test_select_other(void)
{
	s_pt_menu_selected = 1;
	__sync_synchronize();
	divoom_print("product_test: non-aging menu option selected\n");
}

static void *divoom_product_test_ota_retry_task(void *arg)
{
	(void)arg;
	divoom_system_delay_ms(DIVOOM_PT_OTA_RETRY_MS);
	s_pt_ota_started = 0;
	s_pt_ota_retry_pending = 0;
	divoom_product_test_start_ota();
	return NULL;
}

static void divoom_product_test_schedule_ota_retry(void)
{
	if (s_pt_ota_retry_pending || s_pt_flash_started)
		return;
	s_pt_ota_retry_pending = 1;
	if (!divoom_system_create_task(divoom_product_test_ota_retry_task, "pt_ota_retry",
				       NULL, DIVOOM_TASK_STACK_OTA_SERVER_LOCAL)) {
		s_pt_ota_retry_pending = 0;
		/* Task allocation failure is transient; do not permanently stop OTA. */
		divoom_system_delay_ms(DIVOOM_PT_OTA_RETRY_MS);
		s_pt_ota_started = 0;
		divoom_product_test_start_ota();
	}
}

void divoom_product_test_on_ota_result(int result)
{
	s_pt_fw_result = result;
	s_pt_fw_checked = 1;
	divoom_print("product_test: fw local=%d result=%s(%d)\n",
		     DIVOOM_SOFTWARE_VERSION,
		     divoom_product_test_fw_result_name(result), result);
	if (result == DIVOOM_OTA_SERVER_UPGRADE_FILE_READY) {
		s_pt_download_file_flag = DIVOOM_PT_DL_DONE;
		divoom_product_test_try_flash_staged();
		if (s_pt_next_selected && !s_pt_flash_started) {
			divoom_print("product_test: Next downloads complete; stop WiFi, keep pass=%d\n",
				s_pt_wifi_up);
			divoom_product_test_release_update_network();
		}
		return;
	}
	if (result != DIVOOM_OTA_SERVER_UPGRADE_FILE_OK && !s_pt_flash_started) {
		s_pt_download_file_flag = DIVOOM_PT_DL_LISTGUIDE_FAIL;
		divoom_print("product_test: prog staging failed; retry in %d ms\n",
			     DIVOOM_PT_OTA_RETRY_MS);
		divoom_product_test_schedule_ota_retry();
		return;
	}
	if (result != DIVOOM_OTA_SERVER_UPGRADE_FILE_OK && s_pt_flash_started) {
		if (s_pt_reboot_marker_armed)
			divoom_onoff_clear_upgrade_aging_boot();
		s_pt_download_file_flag = DIVOOM_PT_DL_LISTGUIDE_FAIL;
		divoom_print("product_test: prog flash failed; automatic reflash disabled\n");
		return;
	}
	divoom_product_test_continue_after_prog();
}

static void divoom_product_test_handle_boot_done(void)
{
	divoom_product_test_send_check_link();
#if defined(DIVOOM_TIME_FRAME_SPEAKER) && defined(DIVOOM_TIMES_VIBE)
	if (divoom_btcore_uart_cmd_is_boot_done_notified())
		return;
	if (divoom_btcore_uart_cmd_try_notify_aic_host_audio_ready() != 0)
		return;
	divoom_btcore_uart_cmd_try_pending_poweroff();
#endif
}

static void divoom_product_test_handle_msg(const divoom_pt_msg *msg)
{
	if (!msg)
		return;

	switch (msg->msg) {
	case DIVOOM_PT_MSG_CHECK_LINK:
		divoom_product_test_send_check_link();
		break;
	case DIVOOM_PT_MSG_BOOT_DONE:
		divoom_product_test_handle_boot_done();
		break;
	case DIVOOM_PT_MSG_ENTER_EFEX:
		divoom_product_test_enter_efex_boot();
		break;
	case DIVOOM_PT_MSG_POWER_OFF:
		/* 产测关机：不播提示音、不保存配置，直接断电 */
		if (!divoom_onoff_is_poweroff_active())
			divoom_onoff_poweroff_no_tone();
		break;
	case DIVOOM_PT_MSG_WIFI_UP:
		s_pt_wifi_up = 1;
		s_pt_wifi_connected = 1;
		s_pt_wifi_retry_pending = 0;
		divoom_print("product_test: wifi up\n");
		break;
	case DIVOOM_PT_MSG_START_OTA:
		divoom_product_test_start_ota();
		break;
	default:
		divoom_print("product_test: unknown msg %u param=%u\n",
			     (unsigned)msg->msg, (unsigned)msg->param);
		break;
	}
}

int divoom_product_test_wifi_boot_pending(void)
{
	return s_pt_wifi_boot_pending != 0;
}

/* Keep all Wi-Fi initialization out of the factory entry path. */
static void divoom_product_test_wifi_boot_poll(void)
{
	if (s_pt_wifi_boot_pending != 1 || !divoom_pt_ui_is_active())
		return;
    /* Recovery worker owns Wi-Fi initialization, including F/R/V resumes. */
    if (divoom_product_test_is_format_recovery()) {
        s_pt_wifi_boot_pending = 0;
        return;
    }
    s_pt_wifi_boot_pending = 2;
    divoom_print("product_test: test UI ready; start Wi-Fi\n");
	if (!divoom_product_test_wifi_init())
		divoom_print("product_test: Wi-Fi init failed; test UI remains available\n");
	__sync_synchronize();
	s_pt_wifi_boot_pending = 0;
}

static void divoom_product_test_usr_check_poll(void)
{
    divoom_product_test_usr_check_status status;
    if (s_pt_update_owner != 3 ||
        (!s_pt_next_selected && !divoom_product_test_is_upgrade_aging())) return;
    divoom_product_test_usr_check_get_status(&status);
    if (status.phase == DIVOOM_PT_USR_CHECK_IDLE) {
        static uint32 retry_after;
        uint32 now = divoom_system_get_tickms();
        if ((int32_t)(now - retry_after) >= 0) {
            retry_after = now + 1000;
            divoom_product_test_usr_check_start();
        }
        return;
    }
    if (status.error_files || status.phase == DIVOOM_PT_USR_CHECK_FAILED || status.maintenance_required)
        divoom_product_test_stop_aging_audio();
    /* A failed or unstable screening must not release ownership to OTA. */
    if (status.phase != DIVOOM_PT_USR_CHECK_DONE || status.error_files || status.maintenance_required) return;
    if (!__sync_bool_compare_and_swap(&s_pt_update_owner, 3, 0)) return;
    if (divoom_wifi_model_is_network_ready())
        divoom_product_test_start_ota();
}

static void *divoom_product_test_task(void *arg)
{
	divoom_pt_msg *msg;

	(void)arg;
	divoom_print("product_test: task enter\n");

	while (1) {
        divoom_product_test_format_poll();
		while (s_pt_msg_queue && divoom_queue_items(s_pt_msg_queue)) {
			msg = divoom_queue_pop(s_pt_msg_queue);
			if (!msg)
				continue;
			divoom_product_test_handle_msg(msg);
			divoom_system_free(msg);
		}
		divoom_product_test_wifi_boot_poll();
		divoom_product_test_wifi_retry_poll();
		divoom_product_test_usr_check_poll();
		divoom_system_delay_ms(20);
	}

	return NULL;
}

void divoom_product_test_post(uint32 msg, uint32 param)
{
	divoom_pt_msg *item;

	if (!s_pt_msg_queue) {
		divoom_print("product_test: post drop msg=%u (task not started)\n",
			     (unsigned)msg);
		return;
	}

	item = divoom_system_malloc(sizeof(*item));
	if (!item)
		return;

	item->msg = msg;
	item->param = param;
	if (!divoom_queue_push(s_pt_msg_queue, item)) {
		divoom_print("product_test: post queue full msg=%u\n", (unsigned)msg);
		divoom_system_free(item);
	}
}

int divoom_product_test_task_start(void)
{
	if (s_pt_task_started)
		return 1;

	s_pt_msg_queue = divoom_queue_init(32, __func__, __LINE__);
	if (!s_pt_msg_queue) {
		divoom_print("product_test: msg queue init failed\n");
		return 0;
	}

	if (!divoom_system_create_task(divoom_product_test_task, "product_test",
				       NULL, DIVOOM_TASK_STACK_PRODUCT_TEST)) {
		divoom_print("product_test: create task failed\n");
		divoom_queue_destroy(s_pt_msg_queue, NULL);
		s_pt_msg_queue = NULL;
		s_pt_task_started = 0;
		return 0;
	}

	s_pt_task_started = 1;
	return 1;
}

void divoom_product_test_on_btcore_cmd(uint8_t cmd, const uint8_t *cmd_data, uint16_t cmd_len)
{
	switch (cmd) {
	case DM_BTCORE_SEND_KEY_SYSTEM_MSG:
		divoom_product_test_handle_key(cmd_data, cmd_len);
		break;

	case DM_BTCORE_SD_INSERT_STATUS:
		if (cmd_data && cmd_len >= 1u)
			divoom_product_test_set_insert_status(DIVOOM_PRODUCT_TEST_INSERT_TF, cmd_data[0]);
		break;

	case DM_BTCORE_SONGLIST_INFO:
		/* JL 扫盘完成查询歌单；产测不当应答，见 divoom_product_test_tf_play_first */
		divoom_print("product_test: jl 0x42, play id=1\n");
		divoom_product_test_tf_play_first();
		break;

	case DM_BTCORE_LINEIN_INSERT_STATUS:
		if (cmd_data && cmd_len >= 1u)
			divoom_product_test_set_insert_status(DIVOOM_PRODUCT_TEST_INSERT_LINEIN, cmd_data[0]);
		break;

	case DM_BTCORE_USB_IN_OUT_STA:
		if (cmd_data && cmd_len >= 1u)
			divoom_product_test_set_insert_status(DIVOOM_PRODUCT_TEST_INSERT_USB, cmd_data[0]);
		break;

	case DM_BTCORE_UAC_INSERT_STATUS:
		if (cmd_data && cmd_len >= 1u)
			divoom_product_test_set_insert_status(DIVOOM_PRODUCT_TEST_INSERT_UAC, cmd_data[0]);
		break;

	case DM_BTCORE_CHECK_LINK:
		if (cmd_data && cmd_len >= 1u)
			divoom_print("product_test: check link cnt=%u\n", (unsigned)cmd_data[0]);
		break;

	case DM_BTCORE_ENTER_BOOT_MODE:
		divoom_print("product_test: enter boot mode (0x67)\n");
		divoom_product_test_post(DIVOOM_PT_MSG_ENTER_EFEX, 0);
		break;

	case DM_BTCORE_DEVICE_VERSION:
		if (cmd_data && cmd_len >= 4u) {
			s_pt_jl_version = divoom_product_test_get_le32(cmd_data);
			divoom_print("product_test: jl version %u\n", (unsigned)s_pt_jl_version);
			divoom_ota_jl_on_version_notify(s_pt_jl_version);
		}
		break;

	case DM_BTCORE_SYSTEM_POWER_OFF:
		divoom_product_test_post(DIVOOM_PT_MSG_POWER_OFF, 0);
		break;

	default:
		break;
	}
}

void divoom_product_test_enter_efex_boot(void)
{
	if (s_pt_download_file_flag == DIVOOM_PT_DL_FLASHING) {
		divoom_print("product_test: skip efex while flashing\n");
		return;
	}

	/*
	 * 烧录是底线：先落 FEL 标志，再给 2s 硬件超时。
	 * close_flow 正常约 300ms；若 DSI/锁卡住，看门狗仍带着 FEL 复位进 USB。
	 */
#ifdef CONFIG_DRIVERS_RTC
	hal_rtc_set_fel_flag();
#elif defined(CONFIG_DRIVERS_V2_RTC)
	hal_rtc_set_fel_flag();
#endif
#ifdef CONFIG_DRIVERS_WATCHDOG
	hal_watchdog_reset(2);
#endif
	divoom_os_com_lcd_disable();
#ifdef CONFIG_DRIVERS_SPINOR
	hal_spinor_deinit();
#endif
#ifdef CONFIG_DRIVERS_WATCHDOG
	hal_watchdog_restart();
#elif defined(CONFIG_DRIVERS_V2_WATCHDOG)
	hal_watchdog_restart(WDT_CPUX);
#endif
}

uint8 divoom_product_test_app_schedule_play(uint8 event_id, void *user_data, uint32 resouce_type)
{
	(void)event_id;
	(void)user_data;
	(void)resouce_type;
	return true;
}

uint8 divoom_product_test_app_schedule_pause(uint8 event_id, void *user_data, uint8 active_model, uint8 active_event, uint32 resouce_type)
{
	(void)event_id;
	(void)user_data;
	(void)active_model;
	(void)active_event;
	(void)resouce_type;
	return true;
}

uint8 divoom_product_test_app_schedule_destroy(uint8 event_id, void *user_data, uint32 status, uint32 resouce_type)
{
	(void)event_id;
	(void)user_data;
	(void)status;
	(void)resouce_type;
	return true;
}

uint8 divoom_product_test_app_schedule_key_handle(uint8 key_id, uint8 key_type, uint32 resouce_type)
{
	(void)key_id;
	(void)key_type;
	(void)resouce_type;
	return 1;
}

/* Initialize only scalar state before exposing the test controls. */
static void divoom_product_test_wifi_state_reset(void)
{
	s_pt_wifi_shutdown = 0;
	s_pt_wifi_retry_pending = 0;
	s_pt_wifi_retry_tick = 0;
	s_pt_download_file_flag = DIVOOM_PT_DL_CONNECTING;
	s_pt_customize_updating = 0;
	s_pt_customize_applied = 0;
	s_pt_wifi_up = 0;
	s_pt_wifi_connected = 0;
	s_pt_customize_fail = 0;
	s_pt_fw_checked = 0;
	s_pt_fw_result = 0;
	/* 未选择菜单或进入老化升级都自动刷 Prog；Next 会显式改为延迟刷写。 */
	s_pt_allow_flash = 1;
	s_pt_flash_started = 0;
	s_pt_jl_update_started = 0;
	s_pt_next_selected = 0;
	s_pt_menu_selected = 0;
	s_pt_reboot_marker_armed = 0;
	if (divoom_product_test_is_format_recovery()) {
		s_pt_allow_flash = 0;
		s_pt_wifi_shutdown = 1;
	}
}

int divoom_product_test_wifi_init(void)
{
	if (!divoom_product_test_ensure_wifi())
		return 0;
	if (divoom_product_test_is_format_recovery())
		return 1; /* Recovery worker connects only to saved Wi-Fi. */
	divoom_print("product_test: connect factory AP %s\n", DIVOOM_PRODUCT_TEST_WIFI_SSID);
	divoom_wifi_connect_ssid(DIVOOM_PRODUCT_TEST_WIFI_SSID,
		DIVOOM_PRODUCT_TEST_WIFI_PASSWD);
	return 1;
}

void divoom_product_test_wifi_cleanup(void)
{
	divoom_product_test_release_update_network();
}

int divoom_product_test_init(void)
{
	/* The existing worker starts Wi-Fi only after the test screen is active. */
	divoom_product_test_wifi_state_reset();
	if (divoom_product_test_is_upgrade_aging() && !divoom_product_test_start_usr_check())
		return 0;
	s_pt_wifi_boot_pending = 1;
	divoom_product_test_ui_show();
	return 1;
}

void divoom_product_test_close(void)
{
}

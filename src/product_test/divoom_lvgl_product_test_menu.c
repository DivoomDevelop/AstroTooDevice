#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_oneline_led.h"
#include "divoom_lvgl_product_test_menu.h"
#include "divoom_product_test.h"
#include "divoom_product_test_usr_check.h"
#include "divoom_product_test_ui.h"
#include "divoom_product_test_format.h"
#include "divoom_boot_health.h"
#include "divoom_product_test_wifi_input.h"
#include "divoom_product_test_format_i18n.h"
#include "divoom_product_test_text.h"
#include "divoom_user_key.h"
#include "divoom_wifi_con.h"
#include <stdio.h>
#ifdef DIVOOM_TIMES_VIBE
#include "divoom_btcore_uart_cmd.h"
#include "divoom_ota_jl.h"
#endif

#ifndef DIVOOM_LVGL_KEY_ENTER
#define DIVOOM_LVGL_KEY_UP    DIVOOM_KEY_ASTROTOO_KEY_UP
#define DIVOOM_LVGL_KEY_DOWN  DIVOOM_KEY_ASTROTOO_KEY_DOWN
#define DIVOOM_LVGL_KEY_LEFT  DIVOOM_KEY_ASTROTOO_KEY_LEFT
#define DIVOOM_LVGL_KEY_RIGHT DIVOOM_KEY_ASTROTOO_KEY_RIGHT
#define DIVOOM_LVGL_KEY_ESC   DIVOOM_KEY_ASTROTOO_KEY_ESC
#define DIVOOM_LVGL_KEY_ENTER DIVOOM_KEY_ASTROTOO_KEY_ENTER
#endif

#ifndef DIVOOM_SCREEN_WIDTH
#define DIVOOM_SCREEN_WIDTH  480
#define DIVOOM_SCREEN_HIGH   480
#endif
#define PT_UI_VERSION_HOME  0
#define PT_UI_BOOT_CONFIRM  1
#define PT_UI_FORMAT_CONFIRM 2
#define PT_UI_FORMAT_STATUS  3
#define PT_BTN_NEXT         0
#define PT_BTN_CHECK_SYS    1
#define PT_BTN_FORMAT_USER  2
#define PT_BTN_FORMAT_SYS   3
#define PT_BTN_AGING        4
#define PT_BTN_BOOT         5
#define PT_BTN_UDISK        6
#define PT_BTN_COUNT        7

#define PT_LANDING_BTN_W        (DIVOOM_SCREEN_WIDTH - 64)
#define PT_LANDING_BTN_H        45
#define PT_LANDING_BTN_GAP      2
#define PT_LANDING_LIST_BOTTOM  12
/* All ready factory labels share one 28px snapshot; failed fonts use defaults. */
#define PT_FONT_BODY divoom_product_test_next_font(phandle->next_font ? phandle->next_font : phandle->format_i18n)
#define PT_FONT_TITLE PT_FONT_BODY
#define PT_FORMAT_TEXT(key, english) divoom_product_test_format_text(phandle->format_i18n, key, english)
#define PT_FORMAT_FONT(title, fallback) PT_FONT_BODY
#define PT_LANDING_FONT_BTN PT_FONT_BODY
#define PT_LANDING_FONT_CONFIRM PT_FONT_BODY
#define PT_RGB_COLOR_HOLD_MS    1000

static const char * const s_landing_btn_texts[PT_BTN_COUNT] = {
	"Next",
	"Check System File",
	"Format User Partition",
	"Format System Partition",
	"Upgrade Aging",
	"Enter Boot",
	"U Disk",
};

#ifdef CONFIG_CHERRYUSB_DEVICE_TEMPLATE_MSC_DUAL
extern int msc_dual_startup(void);
extern int msc_dual_deinit(void);
extern int msc_dual_enter_udisk(void);
extern int msc_dual_exit_udisk(void);
#endif

#define PT_FACTORY_KEY_LIGHT_WHITE	0x80u
#define PT_FACTORY_KEY_LIGHT_RED	0x81u
#define PT_FACTORY_KEY_LIGHT_GREEN	0x82u
#define PT_FACTORY_KEY_LIGHT_BLUE	0x83u
#define PT_FACTORY_KEY_LIGHT_BRIGHT	100u

#ifdef DIVOOM_TIMES_VIBE
static void divoom_lvgl_product_test_menu_key_light_sync(uint8_t factory_mode)
{
	divoom_btcore_key_light_set_full(factory_mode, 1u, PT_FACTORY_KEY_LIGHT_BRIGHT);
}

static void divoom_lvgl_product_test_menu_key_light_off(void)
{
	divoom_btcore_key_light_off();
}
#else
static void divoom_lvgl_product_test_menu_key_light_sync(uint8_t factory_mode)
{
	(void)factory_mode;
}

static void divoom_lvgl_product_test_menu_key_light_off(void) {}
#endif

typedef struct _divoom_lvgl_product_test_menu_ctrl
{
    int menu_id;
	
	void *main_obj;
	void *disp_label;
	void *file_label;
	lv_obj_t *usr_check_panel, *usr_check_state, *usr_check_progress;
	lv_obj_t *usr_check_bar, *usr_check_errors, *usr_check_hint;
	int usr_check_pass_visible;
	uint32 usr_check_pass_ms;

	void *version_label;
	void *btn_list;
	void *landing_btns[PT_BTN_COUNT];
	void *confirm_label;
	void *format_i18n;
	void *next_font;
	lv_obj_t *font_error;
	lv_font_t landing_font;
	void *format_dialog;
	void *format_title;
	void *format_body;
	void *format_buttons[2];
    lv_obj_t *repair_dialog, *repair_title, *repair_body_view, *repair_body;
    lv_obj_t *repair_buttons[2];
    unsigned repair_request;
    int repair_yes;
	void *format_status_panel;
	void *format_status_target;
	void *format_status_title;
	void *format_progress_group;
	void *format_files_value;
	void *format_files_bar;
	void *format_current_group;
	void *format_current_value;
	void *format_current_bar;
	void *format_status_message;
	void *format_stats_group;
	void *format_status_errors;
	void *format_status_retries;
	void *format_status_detail;
	void *format_status_hint;
	void *format_wifi_group;
	void *format_wifi_ssid;
	void *format_wifi_password;
	int udisk_active;
	int format_target_usr;
	int format_confirm_yes;

	int ui_phase;
	int focus_btn;
	int landing_done;

	int cur_mode;
	int key_index;
	int noise_db_value;
	uint8 bright;
	int backlight_turn_flag;

	int rgb_bright;
	divoom_channel_set_rgb_info rgb_info;

	lv_timer_t *noise_timer; 

	int insert_det_type;
	int insert_det_last;
	int insert_det_seen_insert;
	int insert_det_seen_remove;
	lv_timer_t *insert_det_timer;

	uint32 color_hold_enter_ms;

	int idle_time;
	lv_timer_t *menu_idle_timer;
	uint8_t jl_version_ok;
}divoom_lvgl_product_test_menu_ctrl, *Pdivoom_lvgl_product_test_menu_ctrl;

#ifdef DIVOOM_TIMES_VIBE
static int divoom_lvgl_product_test_menu_query_jl_version(uint32_t *out_ver)
{
	uint32_t ver;

	if (!out_ver)
		return 0;

	ver = divoom_ota_jl_get_local_version();
	if (divoom_ota_jl_version_is_valid(ver)) {
		*out_ver = ver;
		return 1;
	}
	if (divoom_btcore_uart_cmd_get_jl_device_version(&ver) == 0) {
		*out_ver = ver;
		return 1;
	}
	return 0;
}
#endif

typedef enum {
	PT_STATUS_LANG_EN,
	PT_STATUS_LANG_CN,
} pt_status_lang_t;

static void divoom_lvgl_product_test_menu_wifi_status_text(
	char *buf, int buf_len, pt_status_lang_t lang)
{
	if (!buf || buf_len <= 0)
		return;
	if (divoom_product_test_get_wifi_up()
	    || divoom_wifi_model_get_wifi_status()) {
		if (lang == PT_STATUS_LANG_CN)
			snprintf(buf, (size_t)buf_len, PT_TEXT_WIFI_OK);
		else
			snprintf(buf, (size_t)buf_len, "WiFi connected");
	} else {
		if (lang == PT_STATUS_LANG_CN)
			snprintf(buf, (size_t)buf_len, "#FF5252 %s#", PT_TEXT_WIFI_CONNECTING);
		else
			snprintf(buf, (size_t)buf_len, "WiFi connecting...");
	}
}

static void divoom_lvgl_product_test_menu_fw_status_text(
	char *buf, int buf_len, pt_status_lang_t lang)
{
	int flag = divoom_product_test_get_download_file_flag();
	int result = divoom_product_test_get_firmware_result();
	int cur = 0, total = 0, percent;
	char detail[80];
	const char *prefix;
	int use_server_prefix = 0;

	if (!buf || buf_len <= 0)
		return;

	divoom_product_test_get_download_progress(&cur, &total);
	percent = divoom_product_test_get_firmware_percent();
	if (percent < 0)
		percent = 0;
	if (percent > 100)
		percent = 100;
	if (flag == DIVOOM_PT_DL_NONE ||
	    (flag == DIVOOM_PT_DL_DONE &&
	     divoom_product_test_get_firmware_checked() &&
	     result == DIVOOM_OTA_SERVER_UPGRADE_FILE_OK &&
	     !divoom_product_test_get_customize_fail())) {
		snprintf(buf, (size_t)buf_len, lang == PT_STATUS_LANG_CN ? PT_TEXT_ALL_DONE : "All completed");
		return;
	}
	if (flag == DIVOOM_PT_DL_DOWNLOADING && total > 0) {
		snprintf(buf, (size_t)buf_len, lang == PT_STATUS_LANG_CN ? PT_TEXT_RECEIVING : "Receiving files (%d/%d)", cur, total);
		return;
	}
	if (flag == DIVOOM_PT_DL_CONNECTING) {
		if (divoom_product_test_get_wifi_up()) {
			if (lang == PT_STATUS_LANG_CN)
				snprintf(detail, sizeof(detail), PT_TEXT_CHECKING);
			else
				snprintf(detail, sizeof(detail), "Checking...");
		} else {
			if (lang == PT_STATUS_LANG_CN)
				snprintf(detail, sizeof(detail), PT_TEXT_WAITING);
			else
				snprintf(detail, sizeof(detail), "Waiting...");
		}
	} else if (flag == DIVOOM_PT_DL_DOWNLOADING) {
		snprintf(detail, sizeof(detail), lang == PT_STATUS_LANG_CN ? PT_TEXT_DOWNLOADING : "Downloading firmware %d%%", percent);
	} else if (flag == DIVOOM_PT_DL_UNZIPPING) {
		snprintf(detail, sizeof(detail), lang == PT_STATUS_LANG_CN ? PT_TEXT_EXTRACTING : "Extracting firmware %d%%", percent);
	} else if (flag == DIVOOM_PT_DL_FLASHING) {
		snprintf(detail, sizeof(detail), lang == PT_STATUS_LANG_CN ? PT_TEXT_INSTALLING : "Installing firmware %d%%", percent);
	} else if (flag == DIVOOM_PT_DL_JL_UPDATING) {
		snprintf(detail, sizeof(detail), lang == PT_STATUS_LANG_CN ? PT_TEXT_JL_UPDATING : "Updating JL firmware %d%%", percent);
	} else if (flag == DIVOOM_PT_DL_LISTGUIDE_FAIL) {
		use_server_prefix = 1;
		if (lang == PT_STATUS_LANG_CN)
			snprintf(detail, sizeof(detail), PT_TEXT_GUIDE_FAILED);
		else
			snprintf(detail, sizeof(detail), "Listguide failed");
	} else if (flag == DIVOOM_PT_DL_DONE) {
		if (result == DIVOOM_OTA_SERVER_UPGRADE_FILE_READY) {
			if (lang == PT_STATUS_LANG_CN)
				snprintf(detail, sizeof(detail), PT_TEXT_FW_READY);
			else
				snprintf(detail, sizeof(detail), "Firmware ready");
		} else if (result == DIVOOM_OTA_SERVER_UPGRADE_FILE_ERR) {
			if (lang == PT_STATUS_LANG_CN)
				snprintf(detail, sizeof(detail), PT_TEXT_DOWNLOAD_FAILED);
			else
				snprintf(detail, sizeof(detail), "Download failed");
		} else if (result == DIVOOM_OTA_SERVER_UPGRADE_FILE_ERROR_URL) {
			if (lang == PT_STATUS_LANG_CN)
				snprintf(detail, sizeof(detail), PT_TEXT_URL_FAILED);
			else
				snprintf(detail, sizeof(detail), "URL failed");
		} else if (divoom_product_test_get_customize_fail()) {
			if (lang == PT_STATUS_LANG_CN)
				snprintf(detail, sizeof(detail), PT_TEXT_RESOURCE_FAILED);
			else
				snprintf(detail, sizeof(detail), "Resource failed");
		} else if (divoom_product_test_get_firmware_checked()
			 && result == DIVOOM_OTA_SERVER_UPGRADE_FILE_OK) {
			if (lang == PT_STATUS_LANG_CN)
				snprintf(detail, sizeof(detail), PT_TEXT_FW_LATEST);
			else
				snprintf(detail, sizeof(detail), "Firmware up to date");
		} else {
			if (lang == PT_STATUS_LANG_CN)
				snprintf(detail, sizeof(detail), PT_TEXT_DOWNLOAD_DONE);
			else
				snprintf(detail, sizeof(detail), "Download complete");
		}
	} else {
		if (lang == PT_STATUS_LANG_CN)
			snprintf(detail, sizeof(detail), PT_TEXT_WAITING);
		else
			snprintf(detail, sizeof(detail), "Waiting...");
	}

	if (lang == PT_STATUS_LANG_CN)
		prefix = use_server_prefix ? PT_TEXT_SERVER : PT_TEXT_UPDATE;
	else
		prefix = use_server_prefix ? "Server" : "Update";
	snprintf(buf, (size_t)buf_len, "%s: %s", prefix, detail);
}

static void divoom_lvgl_product_test_menu_refresh_status_lines(
	Pdivoom_lvgl_product_test_menu_ctrl phandle, pt_status_lang_t lang)
{
	char buf[320];
	char wifi_line[48];
	char fw_line[96];
	char check_line[128] = {0};

	if (!phandle || !phandle->file_label)
		return;
	divoom_lvgl_product_test_menu_wifi_status_text(
		wifi_line, sizeof(wifi_line), lang);
	divoom_lvgl_product_test_menu_fw_status_text(
		fw_line, sizeof(fw_line), lang);
	if (phandle->landing_done) {
		divoom_product_test_usr_check_status status;
		divoom_product_test_usr_check_get_status(&status);
		if (status.phase == DIVOOM_PT_USR_CHECK_FAILED)
			snprintf(check_line, sizeof(check_line), PT_TEXT_CHECK_FAILED, status.error_code);
		else if (status.error_files)
			snprintf(check_line, sizeof(check_line), PT_TEXT_CHECK_ERRORS, status.error_files);
		else if (status.phase == DIVOOM_PT_USR_CHECK_DONE)
			snprintf(check_line, sizeof(check_line), "%s %d/%d", PT_TEXT_CHECK_PASS, status.done, status.total);
		else if (status.phase == DIVOOM_PT_USR_CHECK_RUNNING)
			snprintf(check_line, sizeof(check_line), "%s %d/%d", PT_TEXT_CHECK_RUNNING, status.done, status.total);
	}
	if (check_line[0])
		snprintf(buf, sizeof(buf), "%s\n%s\n%s", wifi_line, check_line, fw_line);
	else
		snprintf(buf, sizeof(buf), "%s\n%s", wifi_line, fw_line);
	lv_label_set_text(phandle->file_label, buf);
}

static void divoom_lvgl_product_test_menu_refresh_wifi_ui(Pdivoom_lvgl_product_test_menu_ctrl phandle)
{
	if (!phandle)
		return;
	if (phandle->disp_label)
		lv_obj_add_flag(phandle->disp_label, LV_OBJ_FLAG_HIDDEN);
	if (phandle->version_label)
		lv_obj_add_flag(phandle->version_label, LV_OBJ_FLAG_HIDDEN);
	if (phandle->file_label) {
		lv_obj_set_style_text_font(phandle->file_label, divoom_product_test_next_font(phandle->next_font), 0);
		lv_obj_set_style_text_color(phandle->file_label, lv_color_hex(0xFFFFFF), 0);
		lv_label_set_recolor(phandle->file_label, true);
		lv_obj_set_height(phandle->file_label, LV_SIZE_CONTENT);
		lv_label_set_long_mode(phandle->file_label, LV_LABEL_LONG_WRAP);
		lv_obj_align(phandle->file_label, LV_ALIGN_CENTER, 0, 0);
		lv_obj_clear_flag(phandle->file_label, LV_OBJ_FLAG_HIDDEN);
	}
	divoom_lvgl_product_test_menu_refresh_status_lines(phandle, PT_STATUS_LANG_CN);
}

static void divoom_lvgl_product_test_menu_refresh_version_ui(Pdivoom_lvgl_product_test_menu_ctrl phandle)
{
	char buf[160];
	uint32_t jl_ver = 0;
	int jl_ok = 0;

	if (!phandle)
		return;

#ifdef DIVOOM_TIMES_VIBE
	jl_ok = divoom_lvgl_product_test_menu_query_jl_version(&jl_ver);
	phandle->jl_version_ok = (uint8_t)jl_ok;
#endif

	if (jl_ok)
		snprintf(buf, sizeof(buf), "F101: %d  JL: %u",
			 DIVOOM_SOFTWARE_VERSION, (unsigned)jl_ver);
	else
		snprintf(buf, sizeof(buf), "F101: %d  JL: N/A",
			 DIVOOM_SOFTWARE_VERSION);

	if (phandle->version_label)
		lv_label_set_text(phandle->version_label, buf);

	if (phandle->file_label) {
		lv_obj_set_style_text_font(phandle->file_label, PT_FONT_BODY, 0);
		lv_obj_set_height(phandle->file_label, 90);
		lv_label_set_long_mode(phandle->file_label, LV_LABEL_LONG_DOT);
	}

	divoom_lvgl_product_test_menu_refresh_status_lines(phandle, PT_STATUS_LANG_EN);
}

static void divoom_lvgl_product_test_menu_update_mode(Pdivoom_lvgl_product_test_menu_ctrl phandle);
static void divoom_lvgl_product_test_menu_style_btn_focus(lv_obj_t *btn, int focused);
static void divoom_lvgl_product_test_menu_landing_refresh_focus(Pdivoom_lvgl_product_test_menu_ctrl phandle);
static void divoom_lvgl_product_test_menu_landing_show_home(Pdivoom_lvgl_product_test_menu_ctrl phandle);
static void divoom_lvgl_product_test_menu_landing_show_boot_confirm(Pdivoom_lvgl_product_test_menu_ctrl phandle);
static void divoom_lvgl_product_test_menu_landing_leave_udisk(Pdivoom_lvgl_product_test_menu_ctrl phandle);
static void divoom_lvgl_product_test_menu_udisk_toggle(Pdivoom_lvgl_product_test_menu_ctrl phandle);
static void divoom_lvgl_product_test_menu_landing_hide(Pdivoom_lvgl_product_test_menu_ctrl phandle);
static void divoom_lvgl_product_test_menu_landing_leave(Pdivoom_lvgl_product_test_menu_ctrl phandle);
static int divoom_lvgl_product_test_menu_on_version_key(Pdivoom_lvgl_product_test_menu_ctrl phandle, char key);

static lv_obj_t *divoom_lvgl_product_test_menu_landing_btn_get(
	Pdivoom_lvgl_product_test_menu_ctrl phandle, int idx)
{
	if (!phandle || idx < 0 || idx >= PT_BTN_COUNT)
		return NULL;
	return (lv_obj_t *)phandle->landing_btns[idx];
}

static void divoom_lvgl_product_test_menu_landing_list_set_visible(
	Pdivoom_lvgl_product_test_menu_ctrl phandle, int visible)
{
	if (!phandle->btn_list)
		return;

	if (visible)
		lv_obj_clear_flag(phandle->btn_list, LV_OBJ_FLAG_HIDDEN);
	else
		lv_obj_add_flag(phandle->btn_list, LV_OBJ_FLAG_HIDDEN);
}

static void divoom_lvgl_product_test_menu_style_btn_focus(lv_obj_t *btn, int focused)
{
	if (!btn)
		return;
	if (focused) {
		lv_obj_set_style_border_width(btn, 2, 0);
		lv_obj_set_style_border_color(btn, lv_color_hex(0xFFFFFF), 0);
		lv_obj_set_style_bg_color(btn, lv_color_hex(0x333333), 0);
	} else {
		lv_obj_set_style_border_width(btn, 1, 0);
		lv_obj_set_style_border_color(btn, lv_color_hex(0x888888), 0);
		lv_obj_set_style_bg_color(btn, lv_color_hex(0x111111), 0);
	}
}

static void divoom_lvgl_product_test_menu_style_udisk_btn(Pdivoom_lvgl_product_test_menu_ctrl phandle)
{
	lv_obj_t *btn = divoom_lvgl_product_test_menu_landing_btn_get(phandle, PT_BTN_UDISK);
	int focused = (phandle->focus_btn == PT_BTN_UDISK);

	if (!btn)
		return;

	if (phandle->udisk_active) {
		lv_obj_set_style_bg_color(btn, lv_color_hex(0x00AA00), 0);
		lv_obj_set_style_border_width(btn, focused ? 2 : 1, 0);
		lv_obj_set_style_border_color(btn,
			focused ? lv_color_hex(0xFFFFFF) : lv_color_hex(0x888888), 0);
	} else {
		divoom_lvgl_product_test_menu_style_btn_focus(btn, focused);
	}
}

static void divoom_lvgl_product_test_menu_landing_refresh_focus(Pdivoom_lvgl_product_test_menu_ctrl phandle)
{
	int i;

	for (i = 0; i < PT_BTN_COUNT; i++) {
		if (i == PT_BTN_UDISK)
			divoom_lvgl_product_test_menu_style_udisk_btn(phandle);
		else
			divoom_lvgl_product_test_menu_style_btn_focus(
				divoom_lvgl_product_test_menu_landing_btn_get(phandle, i),
				phandle->focus_btn == i);
	}
}

static void divoom_lvgl_product_test_menu_landing_show_home(Pdivoom_lvgl_product_test_menu_ctrl phandle)
{
	phandle->ui_phase = PT_UI_VERSION_HOME;
	phandle->focus_btn = PT_BTN_NEXT;
	if (phandle->version_label)
		lv_obj_clear_flag(phandle->version_label, LV_OBJ_FLAG_HIDDEN);
	if (phandle->file_label)
		lv_obj_clear_flag(phandle->file_label, LV_OBJ_FLAG_HIDDEN);
	divoom_lvgl_product_test_menu_landing_list_set_visible(phandle, 1);
	if (phandle->confirm_label)
		lv_obj_add_flag(phandle->confirm_label, LV_OBJ_FLAG_HIDDEN);
	if (phandle->format_dialog)
		lv_obj_add_flag(phandle->format_dialog, LV_OBJ_FLAG_HIDDEN);
    if (phandle->repair_dialog)
        lv_obj_add_flag(phandle->repair_dialog, LV_OBJ_FLAG_HIDDEN);
	if (phandle->format_status_panel)
		lv_obj_add_flag(phandle->format_status_panel, LV_OBJ_FLAG_HIDDEN);
	divoom_lvgl_product_test_menu_landing_refresh_focus(phandle);
}

static void divoom_lvgl_product_test_menu_landing_show_boot_confirm(Pdivoom_lvgl_product_test_menu_ctrl phandle)
{
	phandle->ui_phase = PT_UI_BOOT_CONFIRM;
	if (phandle->version_label)
		lv_obj_add_flag(phandle->version_label, LV_OBJ_FLAG_HIDDEN);
	if (phandle->file_label)
		lv_obj_add_flag(phandle->file_label, LV_OBJ_FLAG_HIDDEN);
	divoom_lvgl_product_test_menu_landing_list_set_visible(phandle, 0);
	if (phandle->format_dialog)
		lv_obj_add_flag(phandle->format_dialog, LV_OBJ_FLAG_HIDDEN);
    if (phandle->repair_dialog)
        lv_obj_add_flag(phandle->repair_dialog, LV_OBJ_FLAG_HIDDEN);
	if (phandle->format_status_panel)
		lv_obj_add_flag(phandle->format_status_panel, LV_OBJ_FLAG_HIDDEN);
	if (phandle->confirm_label) {
		lv_label_set_text(phandle->confirm_label, "Enter Boot mode?\n\nENTER: Confirm    ESC: Cancel");
		lv_obj_clear_flag(phandle->confirm_label, LV_OBJ_FLAG_HIDDEN);
	}
}

static void divoom_lvgl_product_test_menu_landing_leave_udisk(Pdivoom_lvgl_product_test_menu_ctrl phandle)
{
	if (!phandle || !phandle->udisk_active)
		return;

#ifdef CONFIG_CHERRYUSB_DEVICE_TEMPLATE_MSC_DUAL
	msc_dual_exit_udisk();
	msc_dual_deinit();
#endif
	phandle->udisk_active = 0;
}

static void divoom_lvgl_product_test_menu_udisk_toggle(Pdivoom_lvgl_product_test_menu_ctrl phandle)
{
	if (!phandle)
		return;
	divoom_product_test_select_other();

	if (phandle->udisk_active) {
		divoom_lvgl_product_test_menu_landing_leave_udisk(phandle);
	} else {
#ifdef CONFIG_CHERRYUSB_DEVICE_TEMPLATE_MSC_DUAL
		if (msc_dual_startup() == 0)
			msc_dual_enter_udisk();
		phandle->udisk_active = 1;
#endif
	}
}

static void divoom_lvgl_product_test_menu_enter_upgrade_aging(
	Pdivoom_lvgl_product_test_menu_ctrl phandle)
{
	divoom_lvgl_product_test_menu_landing_leave_udisk(phandle);
	if (!divoom_product_test_enter_upgrade_aging())
		divoom_print("product_test: enter upgrade-aging from menu failed\n");
}

static void divoom_lvgl_product_test_menu_landing_hide(Pdivoom_lvgl_product_test_menu_ctrl phandle)
{
	if (phandle->version_label)
		lv_obj_add_flag(phandle->version_label, LV_OBJ_FLAG_HIDDEN);
	if (phandle->file_label)
		lv_obj_add_flag(phandle->file_label, LV_OBJ_FLAG_HIDDEN);
	divoom_lvgl_product_test_menu_landing_list_set_visible(phandle, 0);
	if (phandle->confirm_label)
		lv_obj_add_flag(phandle->confirm_label, LV_OBJ_FLAG_HIDDEN);
	if (phandle->format_dialog)
		lv_obj_add_flag(phandle->format_dialog, LV_OBJ_FLAG_HIDDEN);
    if (phandle->repair_dialog)
        lv_obj_add_flag(phandle->repair_dialog, LV_OBJ_FLAG_HIDDEN);
	if (phandle->format_status_panel)
		lv_obj_add_flag(phandle->format_status_panel, LV_OBJ_FLAG_HIDDEN);
}

/* LVGL thread only: apply the snapshot prepared before the menu became ready. */
static void divoom_lvgl_product_test_menu_next_font_init(Pdivoom_lvgl_product_test_menu_ctrl phandle)
{
    divoom_print("product_test: NEXT font cached=%d (RAM only)\n", phandle->next_font != NULL);
    lv_obj_set_style_text_font(phandle->disp_label,
        divoom_product_test_next_font(phandle->next_font), 0);
    lv_obj_set_width(phandle->disp_label, DIVOOM_SCREEN_WIDTH - 48);
    lv_label_set_text(phandle->disp_label, "");
    if (!phandle->next_font) {
        phandle->font_error = lv_label_create(phandle->main_obj);
        lv_obj_set_style_text_font(phandle->font_error, divoom_product_test_next_font(NULL), 0);
        lv_obj_set_style_text_color(phandle->font_error, lv_color_hex(0xFF5252), 0);
        lv_obj_set_style_bg_color(phandle->font_error, lv_color_black(), 0);
        lv_obj_set_style_bg_opa(phandle->font_error, LV_OPA_COVER, 0);
        lv_label_set_text(phandle->font_error, PT_TEXT_FONT_ERROR);
        lv_obj_align(phandle->font_error, LV_ALIGN_TOP_MID, 0, 8);
    }
}

static void divoom_lvgl_product_test_menu_landing_leave(Pdivoom_lvgl_product_test_menu_ctrl phandle)
{
	if (phandle->landing_done)
		return;

	phandle->landing_done = 1;
	divoom_lvgl_product_test_menu_next_font_init(phandle);
	divoom_product_test_on_leave_version();
	divoom_lvgl_product_test_menu_landing_hide(phandle);
	if (phandle->disp_label) {
		lv_obj_set_style_text_font(phandle->disp_label, divoom_product_test_next_font(phandle->next_font), 0);
		lv_obj_clear_flag(phandle->disp_label, LV_OBJ_FLAG_HIDDEN);
	}
}
static void divoom_lvgl_product_test_menu_format_dialog_focus(Pdivoom_lvgl_product_test_menu_ctrl phandle)
{
	int i;
	for (i = 0; i < 2; ++i) {
		int selected = (phandle->format_confirm_yes == i);
		lv_obj_t *btn = phandle->format_buttons[i];
		lv_obj_set_style_bg_color(btn, lv_color_hex(selected ?
			(i ? 0xC44747 : 0x3269B2) : 0x222D40), 0);
		lv_obj_set_style_border_color(btn, lv_color_hex(selected ? 0xFFFFFF : 0x42516A), 0);
		lv_obj_set_style_border_width(btn, selected ? 2 : 1, 0);
	}
}

/* 状态页独立于确认弹窗，固定各字段位置，避免下载数字变化时布局跳动。 */
static lv_obj_t *divoom_lvgl_product_test_menu_format_label(lv_obj_t *parent,
	int x, int y, int width, const lv_font_t *font, uint32 color)
{
	lv_obj_t *label = lv_label_create(parent);
	lv_obj_set_width(label, width);
	lv_obj_set_style_text_font(label, font, 0);
	lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
	lv_obj_set_style_text_line_space(label, 4, 0);
	lv_obj_set_pos(label, x, y);
	return label;
}

static void divoom_lvgl_product_test_menu_format_status_create(Pdivoom_lvgl_product_test_menu_ctrl phandle)
{
	lv_obj_t *panel = lv_obj_create(phandle->main_obj);
	lv_obj_t *group;
	lv_obj_t *errors;
    lv_obj_t *current;
    int i;
	lv_obj_t *label;
	lv_obj_t *bar;
	int width = DIVOOM_SCREEN_WIDTH - 80;
	lv_obj_remove_style_all(panel);
	lv_obj_set_size(panel, DIVOOM_SCREEN_WIDTH - 32, DIVOOM_SCREEN_HIGH - 32);
	lv_obj_set_pos(panel, 16, 16);
	lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_bg_color(panel, lv_color_hex(0x101827), 0);
	lv_obj_set_style_bg_opa(panel, LV_OPA_100, 0);
	lv_obj_set_style_border_width(panel, 1, 0);
	lv_obj_set_style_border_color(panel, lv_color_hex(0x30415A), 0);
	lv_obj_set_style_radius(panel, 20, 0);
	phandle->format_status_panel = panel;
	phandle->format_status_target = divoom_lvgl_product_test_menu_format_label(panel, 24, 12, width, PT_FORMAT_FONT(0, PT_FONT_BODY), 0x80AEED);
	phandle->format_status_title = divoom_lvgl_product_test_menu_format_label(panel, 24, 62, width, PT_FORMAT_FONT(1, PT_FONT_TITLE), 0xFFFFFF);
	lv_label_set_long_mode(phandle->format_status_title, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_label_set_long_mode(phandle->format_status_target, LV_LABEL_LONG_SCROLL_CIRCULAR);
	group = lv_obj_create(panel);
	lv_obj_remove_style_all(group);
	lv_obj_set_size(group, width, 128);
	lv_obj_set_pos(group, 24, 116);
	lv_obj_clear_flag(group, LV_OBJ_FLAG_SCROLLABLE);
	phandle->format_progress_group = group;
	label = divoom_lvgl_product_test_menu_format_label(group, 0, 0, width / 2, PT_FORMAT_FONT(0, PT_FONT_BODY), 0xA4B4CC);
	lv_label_set_text(label, PT_FORMAT_TEXT("Progress", "Progress"));
	lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
	label = divoom_lvgl_product_test_menu_format_label(group, width / 2, 0, width / 2, PT_FORMAT_FONT(1, PT_FONT_TITLE), 0xFFFFFF);
	lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_RIGHT, 0);
	phandle->format_files_value = label;
	bar = lv_bar_create(group);
	lv_obj_set_size(bar, width, 10);
	lv_obj_set_pos(bar, 0, 48);
	lv_bar_set_range(bar, 0, 100);
	lv_obj_set_style_bg_color(bar, lv_color_hex(0x29364B), LV_PART_MAIN);
	lv_obj_set_style_bg_opa(bar, LV_OPA_100, LV_PART_MAIN);
	lv_obj_set_style_bg_color(bar, lv_color_hex(0x35D5E5), LV_PART_INDICATOR);
	lv_obj_set_style_radius(bar, 5, LV_PART_MAIN);
	lv_obj_set_style_radius(bar, 5, LV_PART_INDICATOR);
	phandle->format_files_bar = bar;
    current = lv_obj_create(group);
    lv_obj_remove_style_all(current);
    lv_obj_set_size(current, width, 60);
    lv_obj_set_pos(current, 0, 68);
    lv_obj_clear_flag(current, LV_OBJ_FLAG_SCROLLABLE);
    phandle->format_current_group = current;
	label = divoom_lvgl_product_test_menu_format_label(current, 0, 0, width / 2, PT_FORMAT_FONT(0, PT_FONT_BODY), 0xA4B4CC);
	lv_label_set_text(label, "Current file");
    lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
	label = divoom_lvgl_product_test_menu_format_label(current, width / 2, 0, width / 2, PT_FORMAT_FONT(1, PT_FONT_TITLE), 0x59D8C5);
	lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_RIGHT, 0);
	phandle->format_current_value = label;
	bar = lv_bar_create(current);
	lv_obj_set_size(bar, width, 8);
	lv_obj_set_pos(bar, 0, 48);
	lv_bar_set_range(bar, 0, 100);
	lv_obj_set_style_bg_color(bar, lv_color_hex(0x29364B), LV_PART_MAIN);
	lv_obj_set_style_bg_opa(bar, LV_OPA_100, LV_PART_MAIN);
	lv_obj_set_style_bg_color(bar, lv_color_hex(0x59D8C5), LV_PART_INDICATOR);
	lv_obj_set_style_radius(bar, 5, LV_PART_MAIN);
	lv_obj_set_style_radius(bar, 5, LV_PART_INDICATOR);
	phandle->format_current_bar = bar;
	phandle->format_status_message = divoom_lvgl_product_test_menu_format_label(panel, 24, 128, width, PT_FORMAT_FONT(0, PT_FONT_BODY), 0xA4B4CC);
    group = lv_obj_create(panel);
    lv_obj_remove_style_all(group);
    lv_obj_set_size(group, width, 56);
    lv_obj_set_pos(group, 24, 248);
    lv_obj_clear_flag(group, LV_OBJ_FLAG_SCROLLABLE);
    phandle->format_stats_group = group;
    for (i = 0; i < 2; ++i) {
        int card_width = (width-16)/2;
        errors = lv_obj_create(group);
        lv_obj_remove_style_all(errors);
        lv_obj_set_size(errors, card_width, 56);
        lv_obj_set_pos(errors, i*(card_width+16), 0);
        lv_obj_clear_flag(errors, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(errors, lv_color_hex(0x182538), 0);
        lv_obj_set_style_bg_opa(errors, LV_OPA_100, 0);
        lv_obj_set_style_border_width(errors, 1, 0);
        lv_obj_set_style_border_color(errors, lv_color_hex(0x30415A), 0);
        lv_obj_set_style_radius(errors, 10, 0);
        label = divoom_lvgl_product_test_menu_format_label(errors, 8, 12, card_width-16, PT_FORMAT_FONT(0, PT_FONT_BODY), 0xA4B4CC);
        lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_y(label, (56-lv_obj_get_style_text_font(label, 0)->line_height)/2);
        if (i) phandle->format_status_retries = label;
        else phandle->format_status_errors = label;
    }
	phandle->format_status_detail = divoom_lvgl_product_test_menu_format_label(panel, 24, 306, width, PT_FORMAT_FONT(0, PT_FONT_BODY), 0xBDC9DB);
	lv_obj_set_height(phandle->format_status_detail, 90);
	lv_label_set_long_mode(phandle->format_status_detail, LV_LABEL_LONG_SCROLL);
	/* 每个网络字段独占一行；长凭据横向滚动，不挤压进度与统计卡。 */
	group = lv_obj_create(panel);
	lv_obj_remove_style_all(group);
	lv_obj_set_size(group, width, 90);
	lv_obj_set_pos(group, 24, 306);
	lv_obj_clear_flag(group, LV_OBJ_FLAG_SCROLLABLE);
	phandle->format_wifi_group = group;
	label = divoom_lvgl_product_test_menu_format_label(group, 0, 0, 144, PT_FORMAT_FONT(0, PT_FONT_BODY), 0x8A9AB2);
	lv_label_set_text(label, "SSID:");
	label = divoom_lvgl_product_test_menu_format_label(group, 160, 0, width-160, PT_FORMAT_FONT(0, PT_FONT_BODY), 0xBDC9DB);
	lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
	phandle->format_wifi_ssid = label;
	label = divoom_lvgl_product_test_menu_format_label(group, 0, 45, 144, PT_FORMAT_FONT(0, PT_FONT_BODY), 0x8A9AB2);
	lv_label_set_text(label, "Password");
	label = divoom_lvgl_product_test_menu_format_label(group, 160, 45, width-160, PT_FORMAT_FONT(0, PT_FONT_BODY), 0xBDC9DB);
	lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
	phandle->format_wifi_password = label;
	phandle->format_status_hint = divoom_lvgl_product_test_menu_format_label(panel, 24, 400, width, PT_FORMAT_FONT(0, PT_FONT_BODY), 0x8A9AB2);
    lv_label_set_long_mode(phandle->format_status_hint, LV_LABEL_LONG_SCROLL_CIRCULAR);
	lv_obj_add_flag(panel, LV_OBJ_FLAG_HIDDEN);
}

/* 凭据未变时保留 label 的滚动动画，避免每秒刷新把长密码送回起点。 */
static void divoom_lvgl_product_test_menu_format_wifi_text(lv_obj_t *label, const char *text)
{
	if (strcmp(lv_label_get_text(label), text) != 0) {
		lv_label_set_text(label, text);
	}
}

static void divoom_lvgl_product_test_menu_repair_focus(Pdivoom_lvgl_product_test_menu_ctrl phandle)
{
    for (int i = 0; i < 2; ++i) {
        lv_obj_t *button = phandle->repair_buttons[i];
        lv_obj_set_style_bg_color(button, lv_color_hex(i ? 0x20375F : 0x080F32), 0);
        lv_obj_set_style_border_color(button, lv_color_hex(0x25DEFF), 0);
        lv_obj_set_style_border_width(button, phandle->repair_yes == i ? 3 : 0, 0);
    }
}

static void divoom_lvgl_product_test_menu_repair_create(Pdivoom_lvgl_product_test_menu_ctrl phandle)
{
    lv_obj_t *panel = lv_obj_create(phandle->main_obj);
    int width = DIVOOM_SCREEN_WIDTH - 72;
    phandle->repair_dialog = panel;
    lv_obj_remove_style_all(panel);
    lv_obj_set_size(panel, DIVOOM_SCREEN_WIDTH - 24, DIVOOM_SCREEN_HIGH - 48);
    lv_obj_center(panel);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(panel, lv_color_hex(0x080E32), 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_100, 0);
    lv_obj_set_style_radius(panel, 20, 0);
    lv_obj_set_style_border_color(panel, lv_color_hex(0x2796F2), 0);
    lv_obj_set_style_border_width(panel, 2, 0);

    lv_obj_t *badge = divoom_lvgl_product_test_menu_format_label(panel, 24, 20, width,
        PT_FORMAT_FONT(0, PT_FONT_BODY), 0xFFB12B);
    lv_label_set_text(badge, PT_FORMAT_TEXT("Check results", "Check results"));
    phandle->repair_title = divoom_lvgl_product_test_menu_format_label(panel, 24, 65, width,
        PT_FORMAT_FONT(1, PT_FONT_TITLE), 0xFFFFFF);
    lv_label_set_long_mode(phandle->repair_title, LV_LABEL_LONG_WRAP);

    lv_obj_t *body = lv_obj_create(panel);
    phandle->repair_body_view = body;
    lv_obj_remove_style_all(body);
    lv_obj_set_pos(body, 24, 114);
    lv_obj_set_size(body, width, DIVOOM_SCREEN_HIGH - 254);
    lv_obj_add_flag(body, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(body, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(body, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_style_bg_color(body, lv_color_hex(0x42699B), LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_opa(body, LV_OPA_100, LV_PART_SCROLLBAR);
    lv_obj_set_style_width(body, 4, LV_PART_SCROLLBAR);
    phandle->repair_body = divoom_lvgl_product_test_menu_format_label(body, 0, 0, width - 8,
        PT_FORMAT_FONT(0, PT_FONT_BODY), 0xE5EDFF);
    lv_label_set_long_mode(phandle->repair_body, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_line_space(phandle->repair_body, 2, 0);

    for (int i = 0; i < 2; ++i) {
        lv_obj_t *button = lv_btn_create(panel);
        lv_obj_set_size(button, (DIVOOM_SCREEN_WIDTH - 76) / 2, 62);
        lv_obj_align(button, i ? LV_ALIGN_BOTTOM_RIGHT : LV_ALIGN_BOTTOM_LEFT, i ? -20 : 20, -20);
        lv_obj_set_style_radius(button, 12, 0);
        lv_obj_set_style_shadow_width(button, 0, 0);
        lv_obj_t *label = lv_label_create(button);
        lv_obj_set_style_text_font(label, PT_FORMAT_FONT(0, PT_FONT_BODY), 0);
        lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_width(label, (DIVOOM_SCREEN_WIDTH - 112) / 2);
        lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
        lv_label_set_text(label, i ? PT_FORMAT_TEXT("Start repair", "Start repair") :
            PT_FORMAT_TEXT("Cancel", "Cancel"));
        lv_obj_center(label);
        phandle->repair_buttons[i] = button;
    }
}

static void divoom_lvgl_product_test_menu_repair_append(char *text, size_t size, const char *line)
{
    size_t used = strlen(text);
    snprintf(text + used, size - used, "%s%s", used ? "\n" : "", line);
}

static void divoom_lvgl_product_test_menu_repair_refresh(Pdivoom_lvgl_product_test_menu_ctrl phandle,
    const divoom_product_test_format_status *status)
{
    int created = !phandle->repair_dialog;
    if (created)
        divoom_lvgl_product_test_menu_repair_create(phandle);
    if (created || phandle->repair_request != status->repair_request) {
        const divoom_product_test_repair_plan *plan = &status->repair_plan;
        char text[768] = {0};
        char line[192];
        phandle->repair_request = status->repair_request;
        phandle->repair_yes = 0;
        lv_label_set_text(phandle->repair_title, (plan->format_mask & 1U) || plan->structure_files[0] ?
            PT_FORMAT_TEXT("Repair partitions?", "Repair partitions?") :
            PT_FORMAT_TEXT("Repair system files?", "Repair system files?"));

        for (int usr = 1; usr >= 0; --usr) {
            if (plan->format_mask & (1U << usr)) {
                divoom_lvgl_product_test_menu_repair_append(text, sizeof(text), usr ?
                    PT_FORMAT_TEXT("Format System Partition. Erase and redownload all system files.",
                        "Format System Partition. Erase and redownload all system files.") :
                    PT_FORMAT_TEXT("Format User Partition. Erase all user data.",
                        "Format User Partition. Erase all user data."));
            } else if (plan->structure_files[usr] > 0) {
                const char *partition = usr ? PT_FORMAT_TEXT("System Partition", "System Partition") :
                    PT_FORMAT_TEXT("User Partition", "User Partition");
                divoom_lvgl_product_test_menu_repair_append(text, sizeof(text), partition);
                snprintf(line, sizeof(line), PT_FORMAT_TEXT("Remove %d damaged files.",
                    "Remove %d damaged files."), plan->structure_files[usr]);
                divoom_lvgl_product_test_menu_repair_append(text, sizeof(text), line);
            }
        }
        if (!(plan->format_mask & 2U)) {
            if (plan->download_files > 0) {
                snprintf(line, sizeof(line), PT_FORMAT_TEXT("Redownload %d system files.",
                    "Redownload %d system files."), plan->download_files);
                divoom_lvgl_product_test_menu_repair_append(text, sizeof(text), line);
            }
            if (plan->check_system_files)
                divoom_lvgl_product_test_menu_repair_append(text, sizeof(text),
                    PT_FORMAT_TEXT("Continue checking system files to determine downloads.",
                        "Continue checking system files to determine downloads."));
        }
        if ((plan->format_mask & 2U) || plan->download_files > 0 || plan->check_system_files)
            divoom_lvgl_product_test_menu_repair_append(text, sizeof(text),
                PT_FORMAT_TEXT("Repair requires Wi-Fi.", "Repair requires Wi-Fi."));
        lv_label_set_text(phandle->repair_body, text);
        lv_obj_scroll_to_y(phandle->repair_body_view, 0, LV_ANIM_OFF);
        divoom_lvgl_product_test_menu_repair_focus(phandle);
    }
    divoom_product_test_wifi_input_show(phandle->main_obj, 0);
    lv_obj_add_flag(phandle->format_dialog, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(phandle->format_status_panel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(phandle->confirm_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(phandle->repair_dialog, LV_OBJ_FLAG_HIDDEN);
}

static const char *divoom_lvgl_product_test_menu_failure_step(divoom_product_test_format_phase phase)
{
    switch (phase) {
    case DIVOOM_PT_FORMAT_WAIT_NETWORK: return "Connecting saved Wi-Fi";
    case DIVOOM_PT_FORMAT_GUIDE: return "Downloading resource guide";
    case DIVOOM_PT_FORMAT_VOLUME: return "Formatting";
    case DIVOOM_PT_FORMAT_RESTORE: return "Restoring resources";
    case DIVOOM_PT_FORMAT_CHECK: return "Checking file system";
    case DIVOOM_PT_FORMAT_DONE: return "Restarting";
    default: return "Starting repair";
    }
}

static void divoom_lvgl_product_test_menu_format_refresh(Pdivoom_lvgl_product_test_menu_ctrl phandle)
{
	divoom_product_test_format_status status;
	const char *name;
	const char *phase;
	int has_progress;
	int percent;
	const char *hint = "";
    divoom_product_test_format_get_status(&status);
    if (phandle->ui_phase == PT_UI_FORMAT_STATUS && status.phase == DIVOOM_PT_FORMAT_REPAIR_CONFIRM) {
        divoom_lvgl_product_test_menu_repair_refresh(phandle, &status);
        return;
    }
    if (phandle->repair_dialog)
        lv_obj_add_flag(phandle->repair_dialog, LV_OBJ_FLAG_HIDDEN);
    phandle->repair_request = 0;
    name = phandle->format_target_usr ? PT_FORMAT_TEXT("Format /usr", "Format System Partition") :
        PT_FORMAT_TEXT("Format /userdata", "Format User Partition");
	if (phandle->ui_phase == PT_UI_FORMAT_CONFIRM) {
        divoom_product_test_wifi_input_show(phandle->main_obj, 0);
		lv_obj_add_flag(phandle->format_status_panel, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(phandle->confirm_label, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(phandle->format_dialog, LV_OBJ_FLAG_HIDDEN);
		lv_label_set_text(phandle->format_title, name);
		lv_label_set_text(phandle->format_body, phandle->format_target_usr ?
			"Erase the system partition and restore system files.\n\nConnect to saved Wi-Fi first.\nDownload guide before erasing.\nRestore all resources, then reboot." :
			"Erase all data on the user partition.\n\nYour saved data will be deleted.\nThe device will reboot when done.");
        if (phandle->format_i18n) {
            if (phandle->format_target_usr)
                lv_label_set_text_fmt(phandle->format_body, "%s\n\n%s\n%s\n%s",
                    PT_FORMAT_TEXT("Format /usr requires saved Wi-Fi", "Formatting the system partition requires saved Wi-Fi"),
                    PT_FORMAT_TEXT("Downloading resource guide", "Downloading resource guide"),
                    PT_FORMAT_TEXT("Restoring resources", "Restoring resources"),
                    PT_FORMAT_TEXT("Restarting", "Restarting"));
            else
                lv_label_set_text(phandle->format_body, PT_FORMAT_TEXT("Format /userdata and restart?", "Format the user partition and restart?"));
        }
		divoom_lvgl_product_test_menu_format_dialog_focus(phandle);
		return;
	}
	lv_obj_add_flag(phandle->format_dialog, LV_OBJ_FLAG_HIDDEN);
	lv_obj_add_flag(phandle->confirm_label, LV_OBJ_FLAG_HIDDEN);
	lv_obj_clear_flag(phandle->format_status_panel, LV_OBJ_FLAG_HIDDEN);
	divoom_product_test_format_get_status(&status);
    int restore_stage = 0, restore_failed = 0, restore_skipped = 0;
    if (status.phase == DIVOOM_PT_FORMAT_RESTORE || status.phase == DIVOOM_PT_FORMAT_DONE ||
        status.phase == DIVOOM_PT_FORMAT_CHECK_PASSED)
        divoom_update_system_file_restore_status(&restore_stage, &restore_failed, &restore_skipped);
    if (status.phase == DIVOOM_PT_FORMAT_CANCELLED && !status.busy &&
        (status.repair_declined || !divoom_product_test_format_needs_recovery())) {
        divoom_product_test_wifi_input_show(phandle->main_obj, 0);
        divoom_lvgl_product_test_menu_landing_show_home(phandle);
        return;
    }
	switch (status.phase) {
	case DIVOOM_PT_FORMAT_CHECK:
        phase = status.fs_check_stage ? PT_FORMAT_TEXT("Checking file system", "Checking file system") :
            PT_FORMAT_TEXT("Checking files", "Checking files"); break;
    case DIVOOM_PT_FORMAT_RETRY: phase = PT_FORMAT_TEXT("Recovery continues automatically", "Recovery continues automatically"); break;
	case DIVOOM_PT_FORMAT_INPUT_WIFI: phase = "Enter Wi-Fi details"; break;
	case DIVOOM_PT_FORMAT_WAIT_NETWORK: phase = PT_FORMAT_TEXT("Connecting saved Wi-Fi", "Connecting saved Wi-Fi..."); break;
	case DIVOOM_PT_FORMAT_GUIDE: phase = PT_FORMAT_TEXT("Downloading resource guide", "Downloading resource guide..."); break;
	case DIVOOM_PT_FORMAT_VOLUME: phase = PT_FORMAT_TEXT("Formatting", "Formatting..."); break;
	case DIVOOM_PT_FORMAT_RESTORE: phase = PT_FORMAT_TEXT("Restoring resources", "Restoring resources..."); break;
    case DIVOOM_PT_FORMAT_CHECK_PASSED: phase = PT_FORMAT_TEXT("Check passed", "Check passed"); break;
	case DIVOOM_PT_FORMAT_DONE: phase = PT_FORMAT_TEXT("Restarting", "Complete. Rebooting..."); break;
	case DIVOOM_PT_FORMAT_CANCELLED: phase = PT_FORMAT_TEXT("Cancelled", "Cancelled. ESC: Back"); break;
	case DIVOOM_PT_FORMAT_FAILED: phase = PT_FORMAT_TEXT("Storage recovery failed", "Storage recovery failed"); break;
	default: phase = PT_FORMAT_TEXT("Ready", "Ready"); break;
	}
    if (restore_stage == 1) phase = PT_FORMAT_TEXT("Retrying failed files", "Retrying failed files");
    else if (restore_stage == 2) phase = PT_FORMAT_TEXT("Cleaning failed files", "Cleaning failed files");
    else if (restore_stage == 3 && restore_skipped)
        phase = PT_FORMAT_TEXT("Completed with skips", "Completed with skips");
    if (status.stopped) phase = PT_FORMAT_TEXT("Recovery stopped", "Recovery stopped");
    int failed = status.phase == DIVOOM_PT_FORMAT_FAILED;
    int display_usr = failed && status.failure_line ? status.failure_target_usr :
        status.fs_check_stage ? status.fs_check_target : status.target_usr;
    lv_label_set_text(phandle->format_status_target, display_usr < 0 ?
        PT_FORMAT_TEXT("Checking file system", "Checking file system") : display_usr ?
        PT_FORMAT_TEXT("System Partition", "System Partition") : PT_FORMAT_TEXT("User Partition", "User Partition"));
	lv_label_set_text(phandle->format_status_title, phase);
    lv_obj_set_style_text_color(phandle->format_status_title,
        lv_color_hex(restore_stage == 3 && restore_skipped ? 0xFFB454 :
            status.phase == DIVOOM_PT_FORMAT_CHECK_PASSED ? 0x61D69D : 0xFFFFFF), 0);
	has_progress = !failed && status.total > 0 && (status.phase == DIVOOM_PT_FORMAT_RESTORE || status.phase == DIVOOM_PT_FORMAT_CHECK ||
		status.phase == DIVOOM_PT_FORMAT_WAIT_NETWORK || status.phase == DIVOOM_PT_FORMAT_RETRY ||
		status.phase == DIVOOM_PT_FORMAT_FAILED || status.phase == DIVOOM_PT_FORMAT_DONE ||
        status.phase == DIVOOM_PT_FORMAT_CHECK_PASSED);
	if (has_progress) {
		lv_obj_clear_flag(phandle->format_progress_group, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(phandle->format_status_message, LV_OBJ_FLAG_HIDDEN);
		lv_label_set_text_fmt(phandle->format_files_value, "%d/%d", status.done, status.total);
		percent = (int)((uint64_t)status.done * 100 / status.total);
		if (percent < 0) percent = 0;
		if (percent > 100) percent = 100;
		lv_bar_set_value(phandle->format_files_bar, percent, LV_ANIM_OFF);
		if (status.file_percent >= 0) {
			lv_label_set_text_fmt(phandle->format_current_value, "%d%%", status.file_percent);
			lv_bar_set_value(phandle->format_current_bar, status.file_percent, LV_ANIM_OFF);
		} else {
			lv_label_set_text(phandle->format_current_value, "--");
			lv_bar_set_value(phandle->format_current_bar, 0, LV_ANIM_OFF);
		}
	} else {
		lv_obj_add_flag(phandle->format_progress_group, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(phandle->format_status_message, LV_OBJ_FLAG_HIDDEN);
		lv_label_set_text(phandle->format_status_message,
			status.phase == DIVOOM_PT_FORMAT_RESTORE ? PT_FORMAT_TEXT("Restoring resources", "Reading resource list...") : "");
	}
    /* Checks have no per-file percentage. Reserve the second progress row
     * for downloads and let the summary breathe while scanning. */
    if (has_progress && status.file_percent >= 0 && status.phase != DIVOOM_PT_FORMAT_CHECK && status.phase != DIVOOM_PT_FORMAT_RETRY &&
        status.phase != DIVOOM_PT_FORMAT_CHECK_PASSED)
        lv_obj_clear_flag(phandle->format_current_group, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_add_flag(phandle->format_current_group, LV_OBJ_FLAG_HIDDEN);
    /* Failure diagnostics take the space of the counters: the 28px font has
     * a 45px line height, so five detail lines and a two-line footer need it. */
    if (failed) lv_obj_add_flag(phandle->format_stats_group, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_clear_flag(phandle->format_stats_group, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_pos(phandle->format_stats_group, 24, (status.phase == DIVOOM_PT_FORMAT_CHECK || status.phase == DIVOOM_PT_FORMAT_RETRY ||
        status.phase == DIVOOM_PT_FORMAT_CHECK_PASSED) ? 196 : 248);
    lv_obj_set_pos(phandle->format_status_detail, 24, failed ? 116 : 306);
    lv_obj_set_height(phandle->format_status_detail, failed ? 232 : 90);
    lv_obj_set_pos(phandle->format_status_hint, 24, failed ? 358 : 400);
    lv_obj_set_height(phandle->format_status_hint, failed ? 90 : 48);
    lv_label_set_long_mode(phandle->format_status_hint, failed ? LV_LABEL_LONG_WRAP : LV_LABEL_LONG_SCROLL_CIRCULAR);
    int shown_errors = restore_failed ? restore_failed : status.error_files;
    if (shown_errors >= 0)
        lv_label_set_text_fmt(phandle->format_status_errors, PT_FORMAT_TEXT("Error files: %d", "Error files: %d"), shown_errors);
    else
        lv_label_set_text(phandle->format_status_errors, "Errors: --");
    if (restore_failed || restore_skipped)
        lv_label_set_text_fmt(phandle->format_status_retries, PT_FORMAT_TEXT("Skipped files: %d", "Skipped files: %d"), restore_skipped);
    else
        lv_label_set_text_fmt(phandle->format_status_retries, PT_FORMAT_TEXT("Automatic retries: %d", "Retries: %d"), status.repair_retries + status.error_times);
    lv_obj_set_style_text_color(phandle->format_status_errors,
        lv_color_hex(shown_errors > 0 ? 0xFFB454 : 0xA4B4CC), 0);
    lv_obj_set_style_text_color(phandle->format_status_retries,
        lv_color_hex(status.repair_retries > 0 || status.error_times > 0 ? 0xFFB454 : 0xA4B4CC), 0);
	if (status.phase == DIVOOM_PT_FORMAT_WAIT_NETWORK) {
		lv_obj_clear_flag(phandle->format_wifi_group, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(phandle->format_status_detail, LV_OBJ_FLAG_HIDDEN);
		lv_obj_set_pos(phandle->format_wifi_group, 24, has_progress ? 332 : 128);
		divoom_lvgl_product_test_menu_format_wifi_text(phandle->format_wifi_ssid, status.wifi_ssid[0] ? status.wifi_ssid : PT_FORMAT_TEXT("Connecting saved Wi-Fi", "Selecting saved Wi-Fi..."));
		divoom_lvgl_product_test_menu_format_wifi_text(phandle->format_wifi_password,
			status.wifi_ssid[0] ? (status.wifi_password[0] ? status.wifi_password : "(Open network)") : "--");
	} else {
		lv_obj_add_flag(phandle->format_wifi_group, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(phandle->format_status_detail, LV_OBJ_FLAG_HIDDEN);
        const char *detail = status.error_key ? status.error_key :
            status.fs_check_stage ? divoom_product_test_integrity_stage_key(status.fs_check_stage) : "";
        int show_structure_passed = !status.error_key && status.fs_clean_mask == 3U &&
            ((status.phase == DIVOOM_PT_FORMAT_CHECK && !status.fs_check_stage) ||
                status.phase == DIVOOM_PT_FORMAT_CHECK_PASSED);
        if (show_structure_passed)
            detail = "FAT and directories OK on both partitions";
        lv_label_long_mode_t detail_mode = failed || show_structure_passed ? LV_LABEL_LONG_WRAP : LV_LABEL_LONG_SCROLL;
        if (lv_label_get_long_mode(phandle->format_status_detail) != detail_mode)
            lv_label_set_long_mode(phandle->format_status_detail, detail_mode);
        if (failed) {
            char reason[256], codes[96], stage[128];
            /* Keep the actual cause. Automatic retry is explained separately
             * by the footer, without hiding the cause behind a generic hint. */
            snprintf(reason, sizeof(reason), "%s", detail);
            char *action = strstr(reason, ". Retry");
            if (!action) action = strstr(reason, ". Restart test mode");
            if (!action) action = strstr(reason, ". Checking automatically");
            if (action) *action = 0;
            const char *cause = PT_FORMAT_TEXT(reason, reason);
            const char *step = divoom_lvgl_product_test_menu_failure_step(status.failure_phase);
            snprintf(stage, sizeof(stage), "%s%s%s", PT_FORMAT_TEXT(step, step),
                status.failure_stage ? " / " : "", status.failure_stage ? status.failure_stage : "");
            if (status.failure_has_code)
                snprintf(codes, sizeof(codes), "F%d rc=%d errno=%d", status.failure_line, status.failure_code, status.failure_errno);
            else
                snprintf(codes, sizeof(codes), "F%d errno=%d", status.failure_line, status.failure_errno);
            if (status.failure_stage && status.failure_raw_errno != status.failure_errno) {
                size_t length = strlen(codes);
                snprintf(codes + length, sizeof(codes) - length, " raw=%d", status.failure_raw_errno);
            }
            lv_label_set_text_fmt(phandle->format_status_detail, "%s\n%s\n%s",
                stage, cause, codes);
        } else
            lv_label_set_text(phandle->format_status_detail, PT_FORMAT_TEXT(detail, detail));
	}
	if (status.can_cancel)
		hint = "ESC: Cancel / Back";
    if (status.phase == DIVOOM_PT_FORMAT_FAILED)
        hint = status.stopped ? PT_FORMAT_TEXT("Recovery stopped", "Recovery stopped") :
            PT_FORMAT_TEXT("Recovery continues automatically", "Recovery continues automatically");
    if (status.phase == DIVOOM_PT_FORMAT_CHECK_PASSED) {
        hint = PT_FORMAT_TEXT("Press any key to reboot", "Press any key to reboot");
    }
	lv_label_set_text(phandle->format_status_hint, hint);
	divoom_product_test_wifi_input_show(phandle->main_obj, status.phase == DIVOOM_PT_FORMAT_INPUT_WIFI);
    if (phandle->format_i18n && status.can_cancel &&
        status.phase != DIVOOM_PT_FORMAT_FAILED && status.phase != DIVOOM_PT_FORMAT_CHECK_PASSED)
        lv_label_set_text_fmt(phandle->format_status_hint, "ESC: %s", PT_FORMAT_TEXT("Cancel", "Cancel"));
}

static void divoom_lvgl_product_test_menu_format_show(Pdivoom_lvgl_product_test_menu_ctrl phandle)
{
	phandle->format_target_usr = phandle->focus_btn == PT_BTN_FORMAT_SYS;
	phandle->format_confirm_yes = 0;
	phandle->ui_phase = PT_UI_FORMAT_CONFIRM;
	lv_obj_add_flag(phandle->version_label, LV_OBJ_FLAG_HIDDEN);
	lv_obj_add_flag(phandle->file_label, LV_OBJ_FLAG_HIDDEN);
	divoom_lvgl_product_test_menu_landing_list_set_visible(phandle, 0);
	lv_obj_clear_flag(phandle->confirm_label, LV_OBJ_FLAG_HIDDEN);
	divoom_lvgl_product_test_menu_format_refresh(phandle);
}

static void divoom_lvgl_product_test_menu_check_system(Pdivoom_lvgl_product_test_menu_ctrl phandle, int manual)
{
    divoom_lvgl_product_test_menu_landing_leave_udisk(phandle);
    phandle->format_target_usr = 1;
    phandle->ui_phase = PT_UI_FORMAT_STATUS;
    lv_obj_add_flag(phandle->version_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(phandle->file_label, LV_OBJ_FLAG_HIDDEN);
    divoom_lvgl_product_test_menu_landing_list_set_visible(phandle, 0);
    if (manual)
        divoom_product_test_format_check_start();
    else
        divoom_product_test_format_check_continue();
    divoom_lvgl_product_test_menu_format_refresh(phandle);
}

static int divoom_lvgl_product_test_menu_format_key(Pdivoom_lvgl_product_test_menu_ctrl phandle, char key)
{
	divoom_product_test_format_status status;
	if (phandle->ui_phase == PT_UI_FORMAT_CONFIRM) {
		if (key == DIVOOM_LVGL_KEY_ESC || (key == DIVOOM_LVGL_KEY_ENTER && !phandle->format_confirm_yes))
			divoom_lvgl_product_test_menu_landing_show_home(phandle);
		else if (key == DIVOOM_LVGL_KEY_LEFT || key == DIVOOM_LVGL_KEY_RIGHT ||
			key == DIVOOM_LVGL_KEY_UP || key == DIVOOM_LVGL_KEY_DOWN) {
			phandle->format_confirm_yes = !phandle->format_confirm_yes;
			divoom_lvgl_product_test_menu_format_refresh(phandle);
		} else if (key == DIVOOM_LVGL_KEY_ENTER) {
			divoom_lvgl_product_test_menu_landing_leave_udisk(phandle);
			phandle->ui_phase = PT_UI_FORMAT_STATUS;
			divoom_product_test_format_start(phandle->format_target_usr);
			divoom_lvgl_product_test_menu_format_refresh(phandle);
		}
		return 1;
	}
	divoom_product_test_format_get_status(&status);
    if (status.phase == DIVOOM_PT_FORMAT_REPAIR_CONFIRM) {
        /* A key that arrives before a new dialog is rendered may only show it. */
        if (!phandle->repair_dialog || phandle->repair_request != status.repair_request) {
            divoom_lvgl_product_test_menu_format_refresh(phandle);
            return 1;
        }
        if (key == DIVOOM_LVGL_KEY_LEFT || key == DIVOOM_LVGL_KEY_RIGHT) {
            phandle->repair_yes = key == DIVOOM_LVGL_KEY_RIGHT;
            divoom_lvgl_product_test_menu_repair_focus(phandle);
        } else if (key == DIVOOM_LVGL_KEY_UP || key == DIVOOM_LVGL_KEY_DOWN) {
            lv_obj_scroll_by(phandle->repair_body_view, 0, key == DIVOOM_LVGL_KEY_UP ? 56 : -56, LV_ANIM_OFF);
        } else if (key == DIVOOM_LVGL_KEY_ESC || key == DIVOOM_LVGL_KEY_ENTER) {
            divoom_product_test_format_submit_repair(status.repair_request,
                key == DIVOOM_LVGL_KEY_ENTER && phandle->repair_yes);
            divoom_lvgl_product_test_menu_format_refresh(phandle);
        }
        return 1;
    }
	if (status.phase == DIVOOM_PT_FORMAT_CHECK_PASSED) {
        divoom_product_test_format_check_confirm_restart();
        return 1;
    }
    if (key == DIVOOM_LVGL_KEY_ESC && status.can_cancel) {
        divoom_product_test_format_cancel();
        divoom_lvgl_product_test_menu_format_refresh(phandle);
        return 1;
    }
	if (status.phase == DIVOOM_PT_FORMAT_INPUT_WIFI)
		return divoom_product_test_wifi_input_key(key);
	if (key == DIVOOM_LVGL_KEY_ESC && !status.volume_started &&
	    !divoom_product_test_is_format_recovery() && !divoom_product_test_format_needs_recovery()) {
		if (status.busy)
			divoom_product_test_format_cancel();
		else
			divoom_lvgl_product_test_menu_landing_show_home(phandle);
	}
    /* Recovery owns automatic retry and the durable format budget. */
	return 1;
}

static int divoom_lvgl_product_test_menu_on_version_key(Pdivoom_lvgl_product_test_menu_ctrl phandle, char key)
{
	if (!phandle || phandle->landing_done)
		return 0;

	if (phandle->ui_phase >= PT_UI_FORMAT_CONFIRM)
		return divoom_lvgl_product_test_menu_format_key(phandle, key);

	if (phandle->ui_phase == PT_UI_BOOT_CONFIRM) {
		if (key == DIVOOM_LVGL_KEY_ENTER) {
			divoom_product_test_select_other();
			divoom_product_test_enter_efex_boot();
			return 1;
		}
		if (key == DIVOOM_LVGL_KEY_ESC) {
			divoom_lvgl_product_test_menu_landing_show_home(phandle);
			return 1;
		}
		return 1;
	}

	if (key == DIVOOM_LVGL_KEY_UP || key == DIVOOM_LVGL_KEY_DOWN
	    || key == DIVOOM_LVGL_KEY_LEFT || key == DIVOOM_LVGL_KEY_RIGHT) {
		if (key == DIVOOM_LVGL_KEY_UP || key == DIVOOM_LVGL_KEY_LEFT)
			phandle->focus_btn = (phandle->focus_btn + PT_BTN_COUNT - 1) % PT_BTN_COUNT;
		else
			phandle->focus_btn = (phandle->focus_btn + 1) % PT_BTN_COUNT;
		divoom_lvgl_product_test_menu_landing_refresh_focus(phandle);
		return 1;
	}

	if (key == DIVOOM_LVGL_KEY_ENTER) {
		if (phandle->focus_btn == PT_BTN_NEXT) {
			divoom_product_test_select_next();
			divoom_lvgl_product_test_menu_landing_leave(phandle);
			divoom_lvgl_product_test_menu_update_mode(phandle);
		} else if (phandle->focus_btn == PT_BTN_CHECK_SYS) {
			divoom_lvgl_product_test_menu_check_system(phandle, 1);
		} else if (phandle->focus_btn == PT_BTN_AGING) {
			divoom_lvgl_product_test_menu_enter_upgrade_aging(phandle);
		} else if (phandle->focus_btn == PT_BTN_BOOT) {
			divoom_lvgl_product_test_menu_landing_show_boot_confirm(phandle);
		} else if (phandle->focus_btn == PT_BTN_FORMAT_USER || phandle->focus_btn == PT_BTN_FORMAT_SYS) {
			divoom_lvgl_product_test_menu_format_show(phandle);
		} else {
			divoom_lvgl_product_test_menu_udisk_toggle(phandle);
			divoom_lvgl_product_test_menu_landing_refresh_focus(phandle);
		}
		return 1;
	}

	return 0;
}

static int divoom_lvgl_product_test_menu_is_insert_det_mode(int cur_mode)
{
	return cur_mode == DIVOOM_PRODUCT_TEST_MENU_UAC_OPTION
		|| cur_mode == DIVOOM_PRODUCT_TEST_MENU_TF_OPTION;
}

static int divoom_lvgl_product_test_menu_is_rgb_color_mode(int cur_mode)
{
	return cur_mode == DIVOOM_PRODUCT_TEST_MENU_ALL_WHITE_OPTION
		|| cur_mode == DIVOOM_PRODUCT_TEST_MENU_RED_LIGHT_OPTON
		|| cur_mode == DIVOOM_PRODUCT_TEST_MENU_GREEN_LIGHT_OPTION
		|| cur_mode == DIVOOM_PRODUCT_TEST_MENU_BLUE_LIGHT_OPTION;
}

static int divoom_lvgl_product_test_menu_color_hold_ready(Pdivoom_lvgl_product_test_menu_ctrl phandle)
{
	uint32 elapsed;

	if (!phandle || !divoom_lvgl_product_test_menu_is_rgb_color_mode(phandle->cur_mode))
		return 1;
	elapsed = divoom_system_get_tickms() - phandle->color_hold_enter_ms;
	return elapsed >= PT_RGB_COLOR_HOLD_MS;
}

static void divoom_lvgl_product_test_menu_color_hold_start(Pdivoom_lvgl_product_test_menu_ctrl phandle)
{
	if (!phandle)
		return;
	phandle->color_hold_enter_ms = divoom_system_get_tickms();
}

static const char *divoom_lvgl_product_test_menu_insert_name(int insert_det_type)
{
	switch (insert_det_type)
	{
		case 1: return PT_TEXT_LINE_IN;
		case 0: return PT_TEXT_USB;
		case 2: return PT_TEXT_TF;
		default: return PT_TEXT_UNKNOWN;
	}
}

static void divoom_lvgl_product_test_menu_insert_test_stop(Pdivoom_lvgl_product_test_menu_ctrl phandle)
{
	if (phandle->insert_det_timer)
	{
		lv_timer_pause(phandle->insert_det_timer);
		lv_timer_del(phandle->insert_det_timer);
		phandle->insert_det_timer = NULL;
	}
}

static void divoom_lvgl_product_test_menu_show_info_page(Pdivoom_lvgl_product_test_menu_ctrl phandle, const char *text)
{
	lv_obj_set_style_bg_color(phandle->main_obj, lv_color_hex(0x000000), 0);
	lv_obj_set_style_text_color(phandle->disp_label, lv_color_hex(0xFFFFFF), 0);
	lv_obj_clear_flag(phandle->disp_label, LV_OBJ_FLAG_HIDDEN);
	lv_label_set_text(phandle->disp_label, text ? text : "");
}

static lv_obj_t *divoom_lvgl_product_test_menu_usr_check_label(lv_obj_t *parent, int y, int color)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_style_text_font(label, lv_obj_get_style_text_font(parent, 0), 0);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    lv_obj_set_width(label, DIVOOM_SCREEN_WIDTH - 96);
    lv_obj_set_pos(label, 24, y);
    return label;
}

static void divoom_lvgl_product_test_menu_usr_check_refresh(Pdivoom_lvgl_product_test_menu_ctrl phandle)
{
    divoom_product_test_usr_check_status status;
    int percent;
    divoom_product_test_usr_check_get_status(&status);
    if (status.maintenance_required && status.phase != DIVOOM_PT_USR_CHECK_RUNNING &&
        phandle->cur_mode == DIVOOM_PRODUCT_TEST_MENU_USR_CHECK_OPTION) {
        divoom_product_test_ui_stop_aging_overlay();
        if (phandle->usr_check_panel) lv_obj_add_flag(phandle->usr_check_panel, LV_OBJ_FLAG_HIDDEN);
        divoom_lvgl_product_test_menu_check_system(phandle, 0);
        return;
    }
    if (status.phase == DIVOOM_PT_USR_CHECK_DONE && !status.error_files &&
        phandle->usr_check_pass_visible &&
        (uint32)(divoom_system_get_tickms() - phandle->usr_check_pass_ms) >= 1000) {
        divoom_lvgl_product_test_menu_update_mode(phandle);
        return;
    }
    if (!phandle->usr_check_panel) {
        lv_obj_t *panel = lv_obj_create(phandle->main_obj);
        lv_obj_t *title;
        phandle->usr_check_panel = panel;
        lv_obj_remove_style_all(panel);
        lv_obj_set_style_text_font(panel, divoom_product_test_next_font(phandle->next_font), 0);
        lv_obj_set_size(panel, DIVOOM_SCREEN_WIDTH - 48, 400);
        lv_obj_center(panel);
        lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(panel, lv_color_hex(0x111E32), 0);
        lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(panel, 20, 0);
        title = divoom_lvgl_product_test_menu_usr_check_label(panel, 28, 0x79DFFF);
        lv_label_set_text(title, PT_TEXT_CHECK_TITLE);
        phandle->usr_check_state = divoom_lvgl_product_test_menu_usr_check_label(panel, 94, 0xFFFFFF);
        phandle->usr_check_progress = divoom_lvgl_product_test_menu_usr_check_label(panel, 150, 0xFFFFFF);
        phandle->usr_check_bar = lv_bar_create(panel);
        lv_obj_set_pos(phandle->usr_check_bar, 24, 190);
        lv_obj_set_size(phandle->usr_check_bar, DIVOOM_SCREEN_WIDTH - 96, 12);
        lv_obj_set_style_bg_color(phandle->usr_check_bar, lv_color_hex(0x2B3E59), LV_PART_MAIN);
        lv_obj_set_style_bg_color(phandle->usr_check_bar, lv_color_hex(0x4DDAEA), LV_PART_INDICATOR);
        phandle->usr_check_errors = divoom_lvgl_product_test_menu_usr_check_label(panel, 244, 0xFFFFFF);
        phandle->usr_check_hint = divoom_lvgl_product_test_menu_usr_check_label(panel, 342, 0x9AAEC8);
    }
    lv_obj_add_flag(phandle->disp_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_bg_color(phandle->main_obj, lv_color_hex(0x070E1B), 0);
    lv_obj_clear_flag(phandle->usr_check_panel, LV_OBJ_FLAG_HIDDEN);
    if (phandle->font_error) lv_obj_move_foreground(phandle->font_error);
    percent = status.total > 0 ? (int)((int64_t)status.done * 100 / status.total) : 0;
    lv_label_set_text_fmt(phandle->usr_check_progress, "%d/%d  %d%%", status.done, status.total, percent);
    lv_bar_set_value(phandle->usr_check_bar, percent, LV_ANIM_OFF);
    lv_label_set_text_fmt(phandle->usr_check_errors, PT_TEXT_CHECK_ERRORS, status.error_files);
    lv_obj_set_style_text_color(phandle->usr_check_errors,
        lv_color_hex(status.error_files ? 0xFF5252 : 0xFFFFFF), 0);
    if (status.phase == DIVOOM_PT_USR_CHECK_RUNNING) {
        if (status.fs_check_stage) {
            const char *key = divoom_product_test_integrity_stage_key(status.fs_check_stage);
            lv_label_set_text(phandle->usr_check_state, PT_FORMAT_TEXT(key, key));
            lv_label_set_text(phandle->usr_check_hint, status.fs_check_target ?
                PT_FORMAT_TEXT("System Partition", "System Partition") : PT_FORMAT_TEXT("User Partition", "User Partition"));
        } else
            lv_label_set_text(phandle->usr_check_state, status.total ? PT_TEXT_CHECK_RUNNING : PT_TEXT_CHECK_READING);
        if (!status.fs_check_stage) lv_label_set_text(phandle->usr_check_hint, PT_TEXT_WAIT);
    } else if (status.phase == DIVOOM_PT_USR_CHECK_DONE && !status.error_files) {
        lv_label_set_text(phandle->usr_check_state, PT_TEXT_CHECK_PASS);
        lv_label_set_text(phandle->usr_check_hint, PT_TEXT_CHECK_NEXT);
        if (!phandle->usr_check_pass_visible) {
            phandle->usr_check_pass_ms = divoom_system_get_tickms();
            phandle->usr_check_pass_visible = 1;
        }
    } else {
        if (status.phase == DIVOOM_PT_USR_CHECK_DONE)
            lv_label_set_text(phandle->usr_check_state, PT_TEXT_CHECK_DONE);
        else {
            lv_label_set_text_fmt(phandle->usr_check_state, PT_TEXT_CHECK_FAILED, status.error_code);
            lv_obj_set_style_text_color(phandle->usr_check_state, lv_color_hex(0xFF5252), 0);
        }
        lv_label_set_text(phandle->usr_check_hint, PT_TEXT_CHECK_STOPPED);
    }
}

static void divoom_lvgl_product_test_menu_insert_update_label(Pdivoom_lvgl_product_test_menu_ctrl phandle)
{
	const char *name = divoom_lvgl_product_test_menu_insert_name(phandle->insert_det_type);
	char buf[160];

	if (phandle->insert_det_seen_insert && phandle->insert_det_seen_remove)
		snprintf(buf, sizeof(buf), PT_TEXT_INSERT_PASS, name);
	else if (phandle->insert_det_last)
		snprintf(buf, sizeof(buf), PT_TEXT_INSERT_REMOVE, name, name);
	else
		snprintf(buf, sizeof(buf), PT_TEXT_INSERT_ADD, name, name);
	divoom_lvgl_product_test_menu_show_info_page(phandle, buf);
}

static void divoom_lvgl_product_test_menu_insert_refresh(lv_timer_t *timer)
{
	Pdivoom_lvgl_product_test_menu_ctrl phandle = (Pdivoom_lvgl_product_test_menu_ctrl)lv_timer_get_user_data(timer);
	int cur_status;

	if (!phandle || !divoom_lvgl_product_test_menu_is_insert_det_mode(phandle->cur_mode))
		return;

	cur_status = divoom_product_test_get_insert_status((uint8)phandle->insert_det_type) ? 1 : 0;
	if (cur_status != phandle->insert_det_last)
	{
		if (cur_status)
			phandle->insert_det_seen_insert = 1;
		else
			phandle->insert_det_seen_remove = 1;
		phandle->insert_det_last = cur_status;
		divoom_print("product_test ui: %s %s\n",
			     divoom_lvgl_product_test_menu_insert_name(phandle->insert_det_type),
			     cur_status ? "inserted" : "removed");
		divoom_lvgl_product_test_menu_insert_update_label(phandle);
	}

	if (phandle->insert_det_seen_insert && phandle->insert_det_seen_remove)
	{
		divoom_lvgl_product_test_menu_insert_test_stop(phandle);
		divoom_lvgl_product_test_menu_update_mode(phandle);
	}
}

static void divoom_lvgl_product_test_menu_insert_test_start(Pdivoom_lvgl_product_test_menu_ctrl phandle, int insert_det_type)
{
	phandle->insert_det_type = insert_det_type;
	phandle->insert_det_seen_insert = 0;
	phandle->insert_det_seen_remove = 0;
	phandle->insert_det_last = divoom_product_test_get_insert_status((uint8)insert_det_type) ? 1 : 0;

	divoom_lvgl_product_test_menu_insert_test_stop(phandle);
	phandle->insert_det_timer = lv_timer_create(divoom_lvgl_product_test_menu_insert_refresh, 200, phandle);
	divoom_lvgl_product_test_menu_insert_update_label(phandle);
}

void divoom_lvgl_product_test_menu_noise_refresh(lv_timer_t *noise_timer)
{
	Pdivoom_lvgl_product_test_menu_ctrl phandle = (Pdivoom_lvgl_product_test_menu_ctrl)lv_timer_get_user_data(noise_timer);

#ifdef DIVOOM_TIMES_VIBE
	phandle->noise_db_value = divoom_btcore_uart_cmd_get_mic_noise_db();
#else
	phandle->noise_db_value = divoom_eq_noise_mic_fft_get_noise_value();
#endif
	{
		char buf[64];

		snprintf(buf, sizeof(buf), PT_TEXT_MIC, phandle->noise_db_value);
		divoom_lvgl_product_test_menu_show_info_page(phandle, buf);
	}
}


static void divoom_lvgl_product_test_menu_leave_cur_mode(Pdivoom_lvgl_product_test_menu_ctrl phandle)
{
	switch (phandle->cur_mode) {
	case DIVOOM_PRODUCT_TEST_MENU_DISP_VERSION_OPTION:
		if (!phandle->landing_done)
			divoom_lvgl_product_test_menu_landing_hide(phandle);
		else
			lv_obj_add_flag(phandle->disp_label, LV_OBJ_FLAG_HIDDEN);
		break;
	case DIVOOM_PRODUCT_TEST_MENU_BACK_LIGHT_OPTION:
		divoom_screen_bright(100);
		break;
	case DIVOOM_PRODUCT_TEST_MENU_BLUE_LIGHT_OPTION:
		divoom_lvgl_product_test_menu_key_light_off();
		break;
	case DIVOOM_PRODUCT_TEST_MENU_BUZZER_OPTION:
		divoom_local_music_ctrl_stop_tone();
		lv_obj_clear_flag(phandle->disp_label, LV_OBJ_FLAG_HIDDEN);
		break;
	case DIVOOM_PRODUCT_TEST_MENU_MIC_OPTION:
		if (phandle->noise_timer) {
			lv_timer_pause(phandle->noise_timer);
			lv_timer_del(phandle->noise_timer);
			phandle->noise_timer = NULL;
		}
#ifndef DIVOOM_TIMES_VIBE
		divoom_eq_noise_mic_fft_stop();
#endif
		break;
	case DIVOOM_PRODUCT_TEST_MENU_USR_CHECK_OPTION:
		if (phandle->usr_check_panel)
			lv_obj_add_flag(phandle->usr_check_panel, LV_OBJ_FLAG_HIDDEN);
		break;
	case DIVOOM_PRODUCT_TEST_MENU_UAC_OPTION:
	case DIVOOM_PRODUCT_TEST_MENU_TF_OPTION:
		divoom_lvgl_product_test_menu_insert_test_stop(phandle);
		break;
	default:
		break;
	}
}

/* Called only on the LVGL thread. The read-only checker keeps running while
 * every other test is left and its timer/device activity is stopped. */
static int divoom_lvgl_product_test_menu_usr_check_interrupt(Pdivoom_lvgl_product_test_menu_ctrl phandle)
{
    divoom_product_test_usr_check_status status;
    if ((!phandle->landing_done && !divoom_product_test_is_upgrade_aging()) ||
        phandle->ui_phase >= PT_UI_FORMAT_CONFIRM ||
        phandle->cur_mode == DIVOOM_PRODUCT_TEST_MENU_USR_CHECK_OPTION) return 0;
    divoom_product_test_usr_check_get_status(&status);
    if (!status.error_files && status.phase != DIVOOM_PT_USR_CHECK_FAILED) return 0;
    if (!phandle->landing_done) {
        divoom_lvgl_product_test_menu_landing_hide(phandle);
        phandle->landing_done = 1;
    }
    divoom_product_test_ui_stop_aging_overlay();
    divoom_lvgl_product_test_menu_leave_cur_mode(phandle);
    divoom_lvgl_product_test_menu_key_light_off();
    divoom_disp_ambient_ctrl_set_rgb_on_off(0);
    phandle->cur_mode = DIVOOM_PRODUCT_TEST_MENU_USR_CHECK_OPTION;
    divoom_lvgl_product_test_menu_usr_check_refresh(phandle);
    return 1;
}

static void divoom_lvgl_product_test_menu_enter_wifi(Pdivoom_lvgl_product_test_menu_ctrl phandle)
{
	divoom_product_test_usr_check_status status;
	divoom_product_test_usr_check_get_status(&status);
	divoom_lvgl_product_test_menu_key_light_off();
	/* NEXT checks in the background; only wait here if it is still pending.
	 * IDLE belongs to the separate shortcut that does not request a scan. */
	if (status.phase == DIVOOM_PT_USR_CHECK_RUNNING ||
	    status.phase == DIVOOM_PT_USR_CHECK_FAILED || status.error_files) {
		phandle->cur_mode = DIVOOM_PRODUCT_TEST_MENU_USR_CHECK_OPTION;
		phandle->usr_check_pass_visible = 0;
		divoom_lvgl_product_test_menu_usr_check_refresh(phandle);
		return;
	}
	phandle->cur_mode = DIVOOM_PRODUCT_TEST_MENU_WIFI_OPTION;
	lv_obj_set_style_bg_color(phandle->main_obj, lv_color_black(), 0);
	divoom_lvgl_product_test_menu_landing_list_set_visible(phandle, 0);
	if (phandle->confirm_label)
		lv_obj_add_flag(phandle->confirm_label, LV_OBJ_FLAG_HIDDEN);
	if (phandle->format_dialog)
		lv_obj_add_flag(phandle->format_dialog, LV_OBJ_FLAG_HIDDEN);
    if (phandle->repair_dialog)
        lv_obj_add_flag(phandle->repair_dialog, LV_OBJ_FLAG_HIDDEN);
	if (phandle->format_status_panel)
		lv_obj_add_flag(phandle->format_status_panel, LV_OBJ_FLAG_HIDDEN);
	divoom_product_test_on_wifi_page();
	divoom_lvgl_product_test_menu_refresh_wifi_ui(phandle);
}

static void divoom_lvgl_product_test_menu_goto_wifi(Pdivoom_lvgl_product_test_menu_ctrl phandle)
{
	if (!phandle || phandle->cur_mode == DIVOOM_PRODUCT_TEST_MENU_WIFI_OPTION)
		return;

	divoom_lvgl_product_test_menu_leave_cur_mode(phandle);
	divoom_lvgl_product_test_menu_enter_wifi(phandle);
	divoom_print("product_test: long-press play -> wifi\n");
}

void divoom_lvgl_product_test_menu_update_mode(Pdivoom_lvgl_product_test_menu_ctrl phandle)
{
	if (divoom_lvgl_product_test_menu_usr_check_interrupt(phandle))
		return;
	if (phandle->cur_mode == DIVOOM_PRODUCT_TEST_MENU_WIFI_OPTION)
		return;
	if (phandle->cur_mode == DIVOOM_PRODUCT_TEST_MENU_USR_CHECK_OPTION) {
		divoom_product_test_usr_check_status status;
		divoom_product_test_usr_check_get_status(&status);
		if (status.phase != DIVOOM_PT_USR_CHECK_DONE || status.error_files)
			return;
		divoom_lvgl_product_test_menu_leave_cur_mode(phandle);
		divoom_lvgl_product_test_menu_enter_wifi(phandle);
		return;
	}
	if (!divoom_lvgl_product_test_menu_color_hold_ready(phandle))
		return;

	divoom_lvgl_product_test_menu_leave_cur_mode(phandle);

	phandle->cur_mode++;
	/* The result page is an error/wait gate, not a sequential test item. */
	if (phandle->cur_mode == DIVOOM_PRODUCT_TEST_MENU_USR_CHECK_OPTION)
		phandle->cur_mode++;
	if (phandle->cur_mode >= DIVOOM_PRODUCT_TEST_MENU_END_OPTION
	    || phandle->cur_mode == DIVOOM_PRODUCT_TEST_MENU_WIFI_OPTION) {
		divoom_lvgl_product_test_menu_enter_wifi(phandle);
		return;
	}

	switch(phandle->cur_mode)
	{
		case DIVOOM_PRODUCT_TEST_MENU_ALL_WHITE_OPTION:
			divoom_product_test_ensure_rgb();
			divoom_lvgl_product_test_menu_key_light_sync(PT_FACTORY_KEY_LIGHT_WHITE);
			phandle->rgb_info.rgb_light_list[0].select_effect = DIVOOM_DISP_DRAW_CTRL_SIGNAL_COLOR - DIVOOM_DISP_DRAW_CTRL_MULTICOLOR_END -1;
			phandle->rgb_info.brightness = 100;
			phandle->rgb_info.color[0] = 0xFF;
			phandle->rgb_info.color[1] = 0xFF;
			phandle->rgb_info.color[2] = 0xFF;
			phandle->rgb_info.color_cyc_flag = 0;
			divoom_disp_ambient_ctrl_set_disp_info(&phandle->rgb_info, 1);
			lv_obj_set_style_bg_color(phandle->main_obj, lv_color_hex(0xFFFFFF), 0);
			break;

		case DIVOOM_PRODUCT_TEST_MENU_RED_LIGHT_OPTON:
			divoom_lvgl_product_test_menu_key_light_sync(PT_FACTORY_KEY_LIGHT_RED);
			phandle->rgb_info.rgb_light_list[0].select_effect = DIVOOM_DISP_DRAW_CTRL_SIGNAL_COLOR - DIVOOM_DISP_DRAW_CTRL_MULTICOLOR_END -1;
			phandle->rgb_info.brightness = 100;
			phandle->rgb_info.color[0] = 0xFF;
			phandle->rgb_info.color[1] = 0;
			phandle->rgb_info.color[2] = 0;
			phandle->rgb_info.color_cyc_flag = 0;
			divoom_disp_ambient_ctrl_set_disp_info(&phandle->rgb_info, 1);
			lv_obj_set_style_bg_color(phandle->main_obj, lv_color_hex(0xFF0000), 0);
			break;

		case DIVOOM_PRODUCT_TEST_MENU_GREEN_LIGHT_OPTION:
			divoom_lvgl_product_test_menu_key_light_sync(PT_FACTORY_KEY_LIGHT_GREEN);
			phandle->rgb_info.rgb_light_list[0].select_effect = DIVOOM_DISP_DRAW_CTRL_SIGNAL_COLOR - DIVOOM_DISP_DRAW_CTRL_MULTICOLOR_END -1;
			phandle->rgb_info.brightness = 100;
			phandle->rgb_info.color[0] = 0;
			phandle->rgb_info.color[1] = 0xFF;
			phandle->rgb_info.color[2] = 0;
			phandle->rgb_info.color_cyc_flag = 0;
			divoom_disp_ambient_ctrl_set_disp_info(&phandle->rgb_info, 1);
			lv_obj_set_style_bg_color(phandle->main_obj, lv_color_hex(0x00FF00), 0);
			break;

		case DIVOOM_PRODUCT_TEST_MENU_BLUE_LIGHT_OPTION:
			divoom_lvgl_product_test_menu_key_light_sync(PT_FACTORY_KEY_LIGHT_BLUE);
			phandle->rgb_info.rgb_light_list[0].select_effect = DIVOOM_DISP_DRAW_CTRL_SIGNAL_COLOR - DIVOOM_DISP_DRAW_CTRL_MULTICOLOR_END -1;
			phandle->rgb_info.brightness = 100;
			phandle->rgb_info.color[0] = 0;
			phandle->rgb_info.color[1] = 0;
			phandle->rgb_info.color[2] = 0xFF;
			phandle->rgb_info.color_cyc_flag = 0;
			divoom_disp_ambient_ctrl_set_disp_info(&phandle->rgb_info, 1);
			lv_obj_set_style_bg_color(phandle->main_obj, lv_color_hex(0x0000FF), 0);
			break;

		case DIVOOM_PRODUCT_TEST_MENU_BACK_LIGHT_OPTION:
			phandle->rgb_info.rgb_light_list[0].select_effect = DIVOOM_DISP_DRAW_CTRL_SIGNAL_COLOR - DIVOOM_DISP_DRAW_CTRL_MULTICOLOR_END -1;
			phandle->rgb_info.brightness = 100;
			phandle->rgb_info.color[0] = 0xFF;
			phandle->rgb_info.color[1] = 0xFF;
			phandle->rgb_info.color[2] = 0xFF;
			phandle->rgb_info.color_cyc_flag = 0;
			divoom_disp_ambient_ctrl_set_disp_info(&phandle->rgb_info, 1);
			
			lv_obj_set_style_bg_color(phandle->main_obj, lv_color_hex(0xFFFFFF), 0);
			lv_obj_set_style_text_color(phandle->disp_label, lv_color_hex(0x0), 0);
			lv_obj_clear_flag(phandle->disp_label, LV_OBJ_FLAG_HIDDEN);
			lv_label_set_text(phandle->disp_label,
				PT_TEXT_BACKLIGHT);
			break;

		case DIVOOM_PRODUCT_TEST_MENU_BUZZER_OPTION:
			{
				char file_name[128];

				divoom_product_test_ensure_buzzer();
				divoom_voice_read_get_filename(DIVOOM_VOICE_ID_CLOCK_PIANO, file_name);
				if (!divoom_local_music_ctrl_tone_play(file_name, divoom_product_test_get_volume(), NULL, 300))
					divoom_print("product_test buzzer: tone_play failed %s\n", file_name);

				divoom_lvgl_product_test_menu_show_info_page(phandle, PT_TEXT_SPEAKER);
			}
			break;

		case DIVOOM_PRODUCT_TEST_MENU_MIC_OPTION:
#ifndef DIVOOM_TIMES_VIBE
			divoom_product_test_ensure_mic();
			divoom_eq_noise_mic_fft_start(NULL, DIVOOM_FFT_MIC_NOISE);
#endif
			phandle->noise_timer = lv_timer_create(divoom_lvgl_product_test_menu_noise_refresh, 200, phandle);
			divoom_lvgl_product_test_menu_noise_refresh(phandle->noise_timer);
			break;

		case DIVOOM_PRODUCT_TEST_MENU_KEY_OPTION:
			phandle->key_index = 0;
			divoom_lvgl_product_test_menu_show_info_page(phandle, PT_TEXT_KEY_FIRST);
			break;

		case DIVOOM_PRODUCT_TEST_MENU_UAC_OPTION:
			divoom_product_test_ensure_insert();
			divoom_lvgl_product_test_menu_insert_test_start(phandle, 0);
			break;

		case DIVOOM_PRODUCT_TEST_MENU_TF_OPTION:
			divoom_product_test_ensure_insert();
			divoom_lvgl_product_test_menu_insert_test_start(phandle, 2);
			break;

		case DIVOOM_PRODUCT_TEST_MENU_USR_CHECK_OPTION:
			divoom_lvgl_product_test_menu_usr_check_refresh(phandle);
			break;

		case DIVOOM_PRODUCT_TEST_MENU_WIFI_OPTION:
			divoom_lvgl_product_test_menu_enter_wifi(phandle);
			break;

		default:
			break;
	}

	if (divoom_lvgl_product_test_menu_is_rgb_color_mode(phandle->cur_mode))
		divoom_lvgl_product_test_menu_color_hold_start(phandle);
}

void divoom_lvgl_product_test_menu_check_key(Pdivoom_lvgl_product_test_menu_ctrl phandle, char key_value)
{
	/* 电源键、摇杆确认在落地页/前序测试里已按过，暂不测 */
	char key_value_table[] = {
		// (char)kbPOWER,
		DIVOOM_KEY_MUSIC_MODE,
		DIVOOM_KEY_MUSIC_PREV,
		DIVOOM_KEY_PREV,
		DIVOOM_KEY_MUSIC_NEXT,
		DIVOOM_KEY_NEXT,
		// DIVOOM_KEY_PLAY,
		DIVOOM_KEY_ESC,
		DIVOOM_KEY_LIGHT_RIGHT,
		DIVOOM_KEY_MUSIC_PLAY,
		DIVOOM_KEY_LIGHT_LEFT,
    };

	const char *key_disp_table[] = {
        PT_TEXT_KEY_MODE, PT_TEXT_KEY_LEFT, PT_TEXT_KEY_UP,
        PT_TEXT_KEY_RIGHT, PT_TEXT_KEY_DOWN, PT_TEXT_KEY_BACK,
        "+", PT_TEXT_KEY_PLAY, "-",
    };

	int key_num = sizeof(key_value_table);

	if (phandle->cur_mode != DIVOOM_PRODUCT_TEST_MENU_KEY_OPTION)
		return;

	if (key_value_table[phandle->key_index] != key_value)
		return;

	phandle->key_index++;
	if (phandle->key_index < key_num) {
		char buf[64];

		snprintf(buf, sizeof(buf), PT_TEXT_KEY, key_disp_table[phandle->key_index]);
		divoom_lvgl_product_test_menu_show_info_page(phandle, buf);
	}
	else
		divoom_lvgl_product_test_menu_update_mode(phandle);
}

void divoom_lvgl_product_test_menu_check_back_light(Pdivoom_lvgl_product_test_menu_ctrl phandle, char key_value)
{
	if(phandle->cur_mode == DIVOOM_PRODUCT_TEST_MENU_BACK_LIGHT_OPTION)
	{
		if (key_value == DIVOOM_KEY_LIGHT_LEFT)
		{
			phandle->backlight_turn_flag = 1;
			phandle->bright = (phandle->bright > 5) ? (phandle->bright - 5) : (0);
			divoom_product_test_set_brightness(phandle->bright);
			divoom_screen_bright(phandle->bright);

			phandle->rgb_bright = (phandle->rgb_bright > 5) ? (phandle->rgb_bright - 5) : (0);
			divoom_oneline_led_set_bright(phandle->rgb_bright);
		}
		else if (key_value == DIVOOM_KEY_LIGHT_RIGHT)
		{
			phandle->backlight_turn_flag = 1;
			phandle->bright = (phandle->bright < 95) ? (phandle->bright + 5) : (100);
			divoom_product_test_set_brightness(phandle->bright);
			divoom_screen_bright(phandle->bright);

			phandle->rgb_bright = (phandle->rgb_bright < 95) ? (phandle->rgb_bright + 5) : (100);
			divoom_oneline_led_set_bright(phandle->rgb_bright);
		}
		else if(key_value == DIVOOM_LVGL_KEY_ENTER)
		{
			if(phandle->backlight_turn_flag == 1)
			{
				divoom_lvgl_product_test_menu_update_mode(phandle);
			}
		}
	}
}

int divoom_product_test_screen_on_key(void *handle, uint8_t key, uint8_t press_type)
{
	Pdivoom_lvgl_product_test_menu_ctrl phandle =
		(Pdivoom_lvgl_product_test_menu_ctrl)handle;

	if (phandle == NULL)
		return 0;

	phandle->idle_time = 0;
	if (divoom_lvgl_product_test_menu_usr_check_interrupt(phandle))
		return 1;

	if (divoom_product_test_get_download_file_flag() == DIVOOM_PT_DL_FLASHING ||
	    divoom_product_test_get_download_file_flag() == DIVOOM_PT_DL_JL_UPDATING)
		return 1;

	if (phandle->ui_phase >= PT_UI_FORMAT_CONFIRM) {
        if (press_type == DIVOOM_PRESS_LONG && divoom_product_test_format_check_confirm_restart())
            return 1;
		if (press_type == DIVOOM_PRESS_SHORT || press_type == DIVOOM_PRESS_UP)
			divoom_lvgl_product_test_menu_format_key(phandle, key);
		return 1;
	}

	if (!phandle->landing_done
	    && press_type == DIVOOM_PRESS_LONG
	    && key == DIVOOM_KEY_MUSIC_PLAY) {
		divoom_product_test_select_other();
		divoom_lvgl_product_test_menu_landing_leave(phandle);
		divoom_lvgl_product_test_menu_goto_wifi(phandle);
		return 1;
	}

	if (press_type != DIVOOM_PRESS_SHORT && press_type != DIVOOM_PRESS_UP)
		return 1;

	if (phandle->cur_mode == DIVOOM_PRODUCT_TEST_MENU_KEY_OPTION) {
		divoom_lvgl_product_test_menu_check_key(phandle, key);
		return 1;
	}

	if (phandle->cur_mode == DIVOOM_PRODUCT_TEST_MENU_BACK_LIGHT_OPTION) {
		divoom_lvgl_product_test_menu_check_back_light(phandle, key);
		return 1;
	}

	if (divoom_lvgl_product_test_menu_is_insert_det_mode(phandle->cur_mode))
		return 1;

	if (phandle->cur_mode == DIVOOM_PRODUCT_TEST_MENU_USR_CHECK_OPTION)
		return 1; /* Only a completed, clean scan may advance automatically. */

	if (phandle->cur_mode == DIVOOM_PRODUCT_TEST_MENU_DISP_VERSION_OPTION
	    && !phandle->landing_done) {
		if (divoom_lvgl_product_test_menu_on_version_key(phandle, key))
			return 1;
	}

	if (key == DIVOOM_LVGL_KEY_ENTER)
		divoom_lvgl_product_test_menu_update_mode(phandle);

	return 1;
}

void divoom_lvgl_product_test_menu_time_handle(struct _lv_timer_t *user_data)
{
    Pdivoom_lvgl_product_test_menu_ctrl phandle = (Pdivoom_lvgl_product_test_menu_ctrl)lv_timer_get_user_data(user_data);

    phandle->idle_time++;
	if (divoom_lvgl_product_test_menu_usr_check_interrupt(phandle))
		return;
	if (phandle->ui_phase >= PT_UI_FORMAT_CONFIRM) {
		divoom_lvgl_product_test_menu_format_refresh(phandle);
		return;
	}
#if defined(DIVOOM_TIME_FRAME_SPEAKER) && defined(DIVOOM_TIMES_VIBE)
	if (phandle->idle_time >= 2)
		divoom_product_test_try_boot_done();
#endif
	if (!phandle->landing_done)
		divoom_lvgl_product_test_menu_refresh_version_ui(phandle);
	else if (phandle->cur_mode == DIVOOM_PRODUCT_TEST_MENU_WIFI_OPTION)
		divoom_lvgl_product_test_menu_refresh_wifi_ui(phandle);
	else if (phandle->cur_mode == DIVOOM_PRODUCT_TEST_MENU_USR_CHECK_OPTION)
		divoom_lvgl_product_test_menu_usr_check_refresh(phandle);
#ifdef DIVOOM_TIMES_VIBE
	if (!phandle->landing_done && !phandle->jl_version_ok && phandle->idle_time <= 5)
		divoom_lvgl_product_test_menu_refresh_version_ui(phandle);
#endif
}


/* LVGL task only. Flush a resource-free first frame before reading the large
 * font or entering FreeType; neither preparation nor recovery needs a new task. */
static void divoom_lvgl_product_test_menu_prepare_fonts(Pdivoom_lvgl_product_test_menu_ctrl phandle)
{
    int recovery = divoom_product_test_is_format_recovery();
    lv_obj_t *loading = lv_label_create(phandle->main_obj);
    uint32 font_start_ms;
    if (loading) {
        lv_obj_set_style_text_font(loading, LV_FONT_DEFAULT, 0);
        lv_obj_set_style_text_color(loading, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_align(loading, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_line_space(loading, 12, 0);
        lv_obj_set_width(loading, DIVOOM_SCREEN_WIDTH - 48);
        lv_label_set_text(loading, recovery ?
            "SYSTEM FILE CHECK\n\nPreparing...\nPlease wait" :
            "TEST MODE\n\nPreparing...\nPlease wait");
        lv_obj_center(loading);
    }
    lv_scr_load(phandle->main_obj);
    lv_refr_now(NULL);
    divoom_print("product_test: preparing screen displayed\n");
    font_start_ms = divoom_system_get_tickms();
    if (!recovery)
        phandle->format_i18n = divoom_product_test_factory_fonts_create(&phandle->next_font);
    else if (divoom_product_test_format_is_settings_check() != 0)
        /* Settings or an unreadable recovery policy may require a destructive
         * confirmation. Cache its translations before any volume is erased. */
        phandle->format_i18n = divoom_product_test_format_menu_fonts_create();
    divoom_print("product_test: fonts prepared format=%d next=%d elapsed_ms=%u\n",
        phandle->format_i18n != NULL, phandle->next_font != NULL,
        (unsigned)(divoom_system_get_tickms() - font_start_ms));
    if (loading) lv_obj_del(loading);
}

void *divoom_product_test_screen_create(void)
{
	Pdivoom_lvgl_product_test_menu_ctrl phandle = (Pdivoom_lvgl_product_test_menu_ctrl)divoom_system_malloc(sizeof(divoom_lvgl_product_test_menu_ctrl));

	if(phandle == NULL)
	{
		return NULL;
	}
	
	divoom_product_test_format_init();
	memset(phandle, 0, sizeof(divoom_lvgl_product_test_menu_ctrl));
	phandle->cur_mode = DIVOOM_PRODUCT_TEST_MENU_DISP_VERSION_OPTION;
	phandle->bright = divoom_product_test_get_brightness();
	phandle->backlight_turn_flag = 0;
	phandle->rgb_bright = 100;
	phandle->idle_time = 0;
	phandle->landing_done = 0;
	phandle->ui_phase = PT_UI_VERSION_HOME;
	phandle->focus_btn = PT_BTN_NEXT;

	memset(&phandle->rgb_info, 0, sizeof(phandle->rgb_info));
	
	phandle->main_obj = lv_btn_create(NULL);

	lv_obj_remove_style_all(phandle->main_obj);
	lv_obj_set_style_pad_all(phandle->main_obj, 0, 0);
	lv_obj_set_style_border_width(phandle->main_obj, 0, 0);
	lv_obj_set_style_outline_width(phandle->main_obj, 0, 0);
	lv_obj_set_style_shadow_width(phandle->main_obj, 0, 0);
	lv_obj_set_style_bg_color(phandle->main_obj, lv_color_black(), 0);
	lv_obj_set_style_bg_opa(phandle->main_obj, LV_OPA_100, 0);
	lv_obj_set_size(phandle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
	lv_obj_set_pos(phandle->main_obj, 0, 0);
	divoom_lvgl_product_test_menu_prepare_fonts(phandle);
	lv_obj_set_style_text_font(phandle->main_obj, PT_FONT_BODY, 0);

	phandle->version_label = lv_label_create(phandle->main_obj);
	lv_obj_set_style_text_font(phandle->version_label, PT_FONT_TITLE, 0);
	lv_obj_set_style_text_align(phandle->version_label, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_style_text_color(phandle->version_label, lv_color_hex(0xFFFFFF), 0);
	lv_obj_set_width(phandle->version_label, DIVOOM_SCREEN_WIDTH - 32);
	lv_obj_align(phandle->version_label, LV_ALIGN_TOP_MID, 0, 0);

	phandle->file_label = lv_label_create(phandle->main_obj);
	lv_obj_set_style_text_font(phandle->file_label, PT_FONT_BODY, 0);
	lv_obj_set_style_text_align(phandle->file_label, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_style_text_color(phandle->file_label, lv_color_hex(0xFFFFFF), 0);
	lv_obj_set_width(phandle->file_label, DIVOOM_SCREEN_WIDTH - 32);
	lv_obj_align(phandle->file_label, LV_ALIGN_TOP_MID, 0, 46);

	divoom_lvgl_product_test_menu_refresh_version_ui(phandle);

	phandle->btn_list = lv_obj_create(phandle->main_obj);
	lv_obj_remove_style_all(phandle->btn_list);
	lv_obj_set_style_bg_opa(phandle->btn_list, LV_OPA_TRANSP, 0);
	lv_obj_set_style_border_width(phandle->btn_list, 0, 0);
	lv_obj_set_style_pad_all(phandle->btn_list, 0, 0);
	lv_obj_set_style_pad_row(phandle->btn_list, PT_LANDING_BTN_GAP, 0);
	lv_obj_set_size(phandle->btn_list, PT_LANDING_BTN_W,
		PT_BTN_COUNT * PT_LANDING_BTN_H + (PT_BTN_COUNT - 1) * PT_LANDING_BTN_GAP);
	lv_obj_align(phandle->btn_list, LV_ALIGN_BOTTOM_MID, 0, -PT_LANDING_LIST_BOTTOM);
	lv_obj_set_flex_flow(phandle->btn_list, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(phandle->btn_list, LV_FLEX_ALIGN_CENTER,
		LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	{
		int i;

		for (i = 0; i < PT_BTN_COUNT; i++) {
			lv_obj_t *btn = lv_btn_create(phandle->btn_list);

			lv_obj_set_size(btn, PT_LANDING_BTN_W, PT_LANDING_BTN_H);
			lv_obj_set_style_radius(btn, 6, 0);
			{
				lv_obj_t *lbl = lv_label_create(btn);

                const char *text = s_landing_btn_texts[i];
                const lv_font_t *font = PT_LANDING_FONT_BTN;
                if ((i == PT_BTN_NEXT || i == PT_BTN_AGING) &&
                    (phandle->next_font || phandle->format_i18n)) {
                    text = i == PT_BTN_NEXT ? PT_FACTORY_NEXT_TEXT : PT_FACTORY_AGING_TEXT;
                } else if (i == PT_BTN_FORMAT_USER || i == PT_BTN_FORMAT_SYS || i == PT_BTN_CHECK_SYS) {
                    text = PT_FORMAT_TEXT(i == PT_BTN_CHECK_SYS ? "Check System File" :
                        i == PT_BTN_FORMAT_SYS ? "Format /usr" : "Format /userdata", text);
                }
                lv_label_set_text(lbl, text);
                lv_obj_set_style_text_font(lbl, font, 0);
                lv_obj_set_style_text_line_space(lbl, 0, 0);
                lv_obj_set_width(lbl, PT_LANDING_BTN_W - 20);
                lv_label_set_long_mode(lbl, LV_LABEL_LONG_SCROLL_CIRCULAR);
                lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
				lv_obj_center(lbl);
			}
			phandle->landing_btns[i] = btn;
		}
	}

	phandle->confirm_label = lv_label_create(phandle->main_obj);
	lv_obj_set_style_text_font(phandle->confirm_label, PT_LANDING_FONT_CONFIRM, 0);
	lv_obj_set_style_text_align(phandle->confirm_label, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_style_text_color(phandle->confirm_label, lv_color_hex(0xFFFFFF), 0);
	lv_obj_set_width(phandle->confirm_label, DIVOOM_SCREEN_WIDTH - 32);
	lv_label_set_text(phandle->confirm_label, "Enter Boot mode?");
	lv_obj_center(phandle->confirm_label);
	lv_obj_add_flag(phandle->confirm_label, LV_OBJ_FLAG_HIDDEN);

	phandle->format_dialog = lv_obj_create(phandle->main_obj);
	lv_obj_remove_style_all(phandle->format_dialog);
	lv_obj_set_size(phandle->format_dialog, DIVOOM_SCREEN_WIDTH - 64, 384);
	lv_obj_center(phandle->format_dialog);
	lv_obj_clear_flag(phandle->format_dialog, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_bg_color(phandle->format_dialog, lv_color_hex(0x151E2D), 0);
	lv_obj_set_style_bg_opa(phandle->format_dialog, LV_OPA_100, 0);
	lv_obj_set_style_border_width(phandle->format_dialog, 1, 0);
	lv_obj_set_style_border_color(phandle->format_dialog, lv_color_hex(0x35445D), 0);
	lv_obj_set_style_radius(phandle->format_dialog, 20, 0);
	{
		lv_obj_t *warning = lv_label_create(phandle->format_dialog);
		lv_obj_t *hint = lv_label_create(phandle->format_dialog);
		int i;
		lv_obj_set_style_text_font(warning, PT_FORMAT_FONT(0, PT_FONT_BODY), 0);
		lv_obj_set_style_text_color(warning, lv_color_hex(0xFFB454), 0);
		lv_label_set_text(warning, PT_FORMAT_TEXT("FormatSDConfig", "CAUTION  /  ERASE PARTITION"));
		lv_obj_set_width(warning, DIVOOM_SCREEN_WIDTH - 112);
		lv_label_set_long_mode(warning, LV_LABEL_LONG_SCROLL_CIRCULAR);
		lv_obj_set_style_text_align(warning, LV_TEXT_ALIGN_CENTER, 0);
		lv_obj_align(warning, LV_ALIGN_TOP_MID, 0, 12);
		phandle->format_title = lv_label_create(phandle->format_dialog);
		lv_obj_set_width(phandle->format_title, DIVOOM_SCREEN_WIDTH - 112);
		lv_obj_set_style_text_font(phandle->format_title, PT_FORMAT_FONT(1, PT_FONT_TITLE), 0);
		lv_obj_set_style_text_color(phandle->format_title, lv_color_hex(0xFFFFFF), 0);
		lv_obj_set_style_text_align(phandle->format_title, LV_TEXT_ALIGN_CENTER, 0);
		lv_obj_align(phandle->format_title, LV_ALIGN_TOP_MID, 0, 62);
		phandle->format_body = lv_label_create(phandle->format_dialog);
		lv_obj_set_width(phandle->format_body, DIVOOM_SCREEN_WIDTH - 112);
		lv_obj_set_style_text_font(phandle->format_body, PT_FORMAT_FONT(0, PT_FONT_BODY), 0);
		lv_obj_set_style_text_color(phandle->format_body, lv_color_hex(0xBDC9DB), 0);
		lv_obj_set_style_text_align(phandle->format_body, LV_TEXT_ALIGN_CENTER, 0);
		lv_obj_set_style_text_line_space(phandle->format_body, 6, 0);
		lv_obj_align(phandle->format_body, LV_ALIGN_TOP_MID, 0, 112);
		lv_obj_set_height(phandle->format_body, 148);
		lv_label_set_long_mode(phandle->format_body, LV_LABEL_LONG_SCROLL);
		for (i = 0; i < 2; ++i) {
			lv_obj_t *btn = lv_btn_create(phandle->format_dialog);
			lv_obj_t *label = lv_label_create(btn);
			lv_obj_set_size(btn, (DIVOOM_SCREEN_WIDTH - 128) / 2, 48);
			lv_obj_align(btn, i ? LV_ALIGN_BOTTOM_RIGHT : LV_ALIGN_BOTTOM_LEFT, i ? -24 : 24, -70);
			lv_obj_set_style_radius(btn, 10, 0);
			lv_obj_set_style_shadow_width(btn, 0, 0);
			lv_obj_set_style_text_font(label, PT_FORMAT_FONT(1, PT_FONT_TITLE), 0);
			lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
			lv_label_set_text(label, i ? PT_FORMAT_TEXT("Confirm", "Confirm") : PT_FORMAT_TEXT("Cancel", "Cancel"));
			lv_obj_set_width(label, (DIVOOM_SCREEN_WIDTH - 128) / 2 - 16);
			lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
			lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
			lv_obj_center(label);
			phandle->format_buttons[i] = btn;
		}
		lv_obj_set_style_text_font(hint, PT_FORMAT_FONT(0, PT_FONT_BODY), 0);
		lv_obj_set_style_text_color(hint, lv_color_hex(0x8A9AB2), 0);
		lv_label_set_text(hint, "LEFT / RIGHT: Select   ENTER: OK");
		if (phandle->format_i18n)
            lv_label_set_text_fmt(hint, "LEFT / RIGHT: %s   ENTER: %s",
                PT_FORMAT_TEXT("Select", "Select"), PT_FORMAT_TEXT("Confirm", "OK"));
        lv_obj_set_width(hint, DIVOOM_SCREEN_WIDTH - 112);
        lv_label_set_long_mode(hint, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -12);
	}
	lv_obj_add_flag(phandle->format_dialog, LV_OBJ_FLAG_HIDDEN);
	divoom_lvgl_product_test_menu_format_status_create(phandle);
	divoom_lvgl_product_test_menu_landing_show_home(phandle);

	phandle->disp_label = lv_label_create(phandle->main_obj);
	lv_obj_set_style_text_font(phandle->disp_label, PT_FONT_TITLE, 0);
	
	lv_obj_set_style_text_align(phandle->disp_label, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_style_text_color(phandle->disp_label, lv_color_hex(0xFFFFFF), 0);
	lv_label_set_recolor(phandle->disp_label, true);
	lv_obj_add_flag(phandle->disp_label, LV_OBJ_FLAG_HIDDEN);
	lv_obj_center(phandle->disp_label);

	if (divoom_product_test_is_format_recovery()) {
		phandle->format_target_usr = !divoom_boot_health_userdata_mount_failed();
		phandle->ui_phase = PT_UI_FORMAT_STATUS;
		lv_obj_add_flag(phandle->version_label, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(phandle->file_label, LV_OBJ_FLAG_HIDDEN);
		divoom_lvgl_product_test_menu_landing_list_set_visible(phandle, 0);
		divoom_product_test_format_resume();
		divoom_lvgl_product_test_menu_format_refresh(phandle);
	}
	phandle->menu_idle_timer = lv_timer_create(divoom_lvgl_product_test_menu_time_handle, 1000, phandle);
		
	return phandle;
}

void divoom_product_test_screen_show(void *handle)
{
	Pdivoom_lvgl_product_test_menu_ctrl phandle = (Pdivoom_lvgl_product_test_menu_ctrl)handle;
	lv_group_t *group = divoom_product_test_ui_get_group();

	if (phandle == NULL || phandle->main_obj == NULL)
		return;

	if (group) {
		lv_group_add_obj(group, phandle->main_obj);
		lv_group_focus_obj(phandle->main_obj);
	}
	lv_scr_load(phandle->main_obj);
    if (divoom_product_test_is_upgrade_aging()) {
        /* Keep the font-owning screen, but still stop the old test's devices
         * and timers as the previous close/recreate transition did. */
        divoom_lvgl_product_test_menu_key_light_off();
        divoom_lvgl_product_test_menu_landing_leave_udisk(phandle);
        divoom_lvgl_product_test_menu_insert_test_stop(phandle);
        if (phandle->noise_timer) {
            lv_timer_del(phandle->noise_timer);
            phandle->noise_timer = NULL;
#ifndef DIVOOM_TIMES_VIBE
            divoom_eq_noise_mic_fft_stop();
#endif
        }
    }
	phandle->idle_time = 0;
	if (divoom_lvgl_product_test_menu_usr_check_interrupt(phandle))
		return;
	if (!phandle->landing_done)
		divoom_lvgl_product_test_menu_refresh_version_ui(phandle);
	else if (phandle->cur_mode == DIVOOM_PRODUCT_TEST_MENU_WIFI_OPTION)
		divoom_lvgl_product_test_menu_refresh_wifi_ui(phandle);
}

void divoom_product_test_screen_destroy(void *handle)
{
	Pdivoom_lvgl_product_test_menu_ctrl phandle = (Pdivoom_lvgl_product_test_menu_ctrl)handle;

	if (phandle == NULL)
		return;

	divoom_lvgl_product_test_menu_key_light_off();
	divoom_lvgl_product_test_menu_landing_leave_udisk(phandle);
	divoom_lvgl_product_test_menu_insert_test_stop(phandle);
	/* 切换到老化升级 UI 不是退出产测，保留正在进行的 AP 连接。 */
	if (!divoom_product_test_is_upgrade_aging())
		divoom_product_test_wifi_cleanup();

	if (phandle->noise_timer) {
		lv_timer_del(phandle->noise_timer);
		phandle->noise_timer = NULL;
	}
	if (phandle->menu_idle_timer) {
		lv_timer_del(phandle->menu_idle_timer);
		phandle->menu_idle_timer = NULL;
	}
#ifndef DIVOOM_TIMES_VIBE
	divoom_eq_noise_mic_fft_stop();
#endif
    divoom_product_test_wifi_input_destroy();
    if (phandle->main_obj)
        lv_obj_del(phandle->main_obj);

	divoom_product_test_format_i18n_destroy(phandle->format_i18n);
	divoom_product_test_format_i18n_destroy(phandle->next_font);
	divoom_system_free(phandle);
}

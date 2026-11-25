#include "divoom.h"
#include "divoom_product_test.h"
#include "divoom_product_test_usr_check.h"
#include "divoom_product_test_ui.h"
#include "divoom_disp_ctrl_app.h"
#include "divoom_btcore_uart_cmd.h"
#include "divoom_led_control.h"
#include "divoom_read_pcm.h"
#include "divoom_local_music_ctrl.h"
#include "divoom_mic_ctrl.h"
#include "divoom_eq_noise_mic_fft.h"
#include "divoom_disp_ambient_ctrl.h"
#include "divoom_screen_driver.h"
#include "divoom_wifi_con.h"

enum {
	PT_LAZY_VERSION_LEFT = 1 << 0,
	PT_LAZY_BUZZER       = 1 << 1,
	PT_LAZY_MIC          = 1 << 2,
	PT_LAZY_RGB          = 1 << 3,
	PT_LAZY_WIFI         = 1 << 4,
	PT_LAZY_INSERT       = 1 << 5,
};

static uint8_t s_pt_lazy_inited;
static uint8_t s_pt_cold_base_inited;
static volatile int s_pt_boot_state;
static volatile uint8_t s_pt_boot_mode = DIVOOM_PRODUCT_TEST_BOOT_NORMAL;
static volatile int s_pt_aging_audio_retry_pending;
static volatile int s_pt_aging_audio_stopped;
static uint32_t s_pt_aging_audio_started_at;
static uint8_t s_pt_aging_audio_timer_started;
#if defined(DIVOOM_TIME_FRAME_SPEAKER) && defined(DIVOOM_TIMES_VIBE)
static uint8_t s_pt_wait_boot_done;
#endif

static void divoom_product_test_ensure_minimal_dirs(void)
{
	divoom_app_com_mk_dir(DIVOOM_TMP_DIR);
}

static int divoom_product_test_lazy_begin(uint8_t flag)
{
	if (s_pt_lazy_inited & flag)
		return 0;
	s_pt_lazy_inited |= flag;
	return 1;
}

int divoom_product_test_is_format_recovery(void)
{
	return s_pt_boot_mode == DIVOOM_PRODUCT_TEST_BOOT_FORMAT_RECOVERY;
}

void divoom_product_test_leave_format_recovery(void)
{
    if (s_pt_boot_mode == DIVOOM_PRODUCT_TEST_BOOT_FORMAT_RECOVERY)
        s_pt_boot_mode = DIVOOM_PRODUCT_TEST_BOOT_NORMAL;
}

int divoom_product_test_is_upgrade_aging(void)
{
	return s_pt_boot_mode == DIVOOM_PRODUCT_TEST_BOOT_UPGRADE_AGING;
}

void divoom_product_test_try_boot_done(void)
{
#if defined(DIVOOM_TIME_FRAME_SPEAKER) && defined(DIVOOM_TIMES_VIBE)
	if (!s_pt_wait_boot_done || divoom_btcore_uart_cmd_is_boot_done_notified())
		return;
	divoom_product_test_post(DIVOOM_PT_MSG_BOOT_DONE, 0);
#else
	return;
#endif
}

void divoom_product_test_on_leave_version(void)
{
	if (!divoom_product_test_lazy_begin(PT_LAZY_VERSION_LEFT))
		return;
	divoom_microtask_init();
	divoom_decode_pic_init();
	divoom_system_font_init_without_check();
}

void divoom_product_test_ensure_buzzer(void)
{
	if (!divoom_product_test_lazy_begin(PT_LAZY_BUZZER))
		return;
	divoom_local_music_ctrl_music_init();
	if (divoom_pcm_init(32000) != 0)
		divoom_print("product_test ensure_buzzer: pcm init failed\n");
	divoom_pcm_apply_capture_gain();
}

static int divoom_product_test_aging_check_failed(void)
{
    divoom_product_test_usr_check_status status;
    divoom_product_test_usr_check_get_status(&status);
    return status.error_files || status.phase == DIVOOM_PT_USR_CHECK_FAILED;
}

void divoom_product_test_stop_aging_audio(void)
{
    if (divoom_product_test_is_upgrade_aging() &&
        __sync_bool_compare_and_swap(&s_pt_aging_audio_stopped, 0, 1))
        divoom_local_music_ctrl_stop_tone();
}

static void divoom_product_test_start_aging_audio(void);

static void *divoom_product_test_aging_audio_retry_task(void *arg)
{
	(void)arg;
	divoom_system_delay_ms(2000);
	s_pt_aging_audio_retry_pending = 0;
	divoom_product_test_start_aging_audio();
	return NULL;
}

static void divoom_product_test_schedule_aging_audio_retry(void)
{
	if (!divoom_product_test_is_upgrade_aging() || s_pt_aging_audio_stopped
		|| divoom_product_test_aging_check_failed()
		|| !__sync_bool_compare_and_swap(&s_pt_aging_audio_retry_pending, 0, 1))
		return;
	while (!divoom_system_create_task(divoom_product_test_aging_audio_retry_task,
		"pt_audio_retry", NULL, DIVOOM_TASK_STACK_PRODUCT_TEST)) {
		divoom_print("product_test: create aging audio retry task failed\n");
		divoom_system_delay_ms(2000);
	}
}

static int divoom_product_test_aging_audio_done(char *music_name, int status)
{
	(void)music_name;
	if (!status)
		divoom_product_test_schedule_aging_audio_retry();
	return 0;
}

static int divoom_product_test_aging_audio_remaining(void)
{
	uint32_t now = divoom_system_get_ticks();
	uint32_t elapsed;

	if (!s_pt_aging_audio_timer_started) {
		s_pt_aging_audio_started_at = now;
		__sync_synchronize();
		s_pt_aging_audio_timer_started = 1;
	}

	elapsed = now - s_pt_aging_audio_started_at;
	if (elapsed >= DIVOOM_PRODUCT_TEST_AGING_AUDIO_MAX_SECONDS)
		return 0;
	return (int)(DIVOOM_PRODUCT_TEST_AGING_AUDIO_MAX_SECONDS - elapsed);
}

static void divoom_product_test_start_aging_audio(void)
{
	int play_seconds;

	if (!divoom_product_test_is_upgrade_aging())
		return;
	if (s_pt_aging_audio_stopped || divoom_product_test_aging_check_failed()) {
		divoom_product_test_stop_aging_audio();
		return;
	}
	play_seconds = divoom_product_test_aging_audio_remaining();
	if (play_seconds <= 0) {
		divoom_print("product_test: aging audio reached 8-hour limit\n");
		return;
	}

	divoom_product_test_ensure_buzzer();
	if (!divoom_local_music_ctrl_tone_play(
		(char *)DIVOOM_PRODUCT_TEST_AGING_AUDIO_PATH,
		DIVOOM_PRODUCT_TEST_AGING_AUDIO_VOLUME,
		divoom_product_test_aging_audio_done, play_seconds)) {
		divoom_print("product_test: aging audio start failed: %s\n",
			DIVOOM_PRODUCT_TEST_AGING_AUDIO_PATH);
		divoom_product_test_schedule_aging_audio_retry();
	}
	/* A concurrent check may have failed while the PLAY request was queued. */
	if (s_pt_aging_audio_stopped || divoom_product_test_aging_check_failed()) {
		divoom_product_test_stop_aging_audio();
		divoom_local_music_ctrl_stop_tone();
	}
}

int divoom_product_test_enter_upgrade_aging(void)
{
	if (s_pt_boot_state != 2)
		return 0;
	if (divoom_product_test_is_upgrade_aging())
		return 1;

	if (!divoom_product_test_start_usr_check())
		return 0;
	s_pt_boot_mode = DIVOOM_PRODUCT_TEST_BOOT_UPGRADE_AGING;
	__sync_synchronize();
	/* 复用当前产测实例，只开放老化升级刷写并切换 UI/音频。 */
	divoom_product_test_on_wifi_page();
	divoom_product_test_start_aging_audio();
	divoom_product_test_ui_show();
	divoom_print("product_test: entered upgrade-aging from menu\n");
	return 1;
}

void divoom_product_test_ensure_mic(void)
{
	if (!divoom_product_test_lazy_begin(PT_LAZY_MIC))
		return;
#ifndef DIVOOM_TIMES_VIBE
	divoom_mic_ctrl_init();
	divoom_eq_noise_mic_fft_init();
#endif
}

void divoom_product_test_ensure_rgb(void)
{
	if (!divoom_product_test_lazy_begin(PT_LAZY_RGB))
		return;
	divoom_disp_ambient_ctrl_init(1);
}

int divoom_product_test_ensure_wifi(void)
{
	if (!divoom_product_test_lazy_begin(PT_LAZY_WIFI))
		return divoom_wifi_is_ready();
	if (!divoom_product_test_wifi_model_init()) {
		s_pt_lazy_inited &= (uint8_t)~PT_LAZY_WIFI;
		divoom_print("product_test: wifi_model_init failed\n");
		return 0;
	}
	return 1;
}

void divoom_product_test_ensure_insert(void)
{
	if (!divoom_product_test_lazy_begin(PT_LAZY_INSERT))
		return;
}

int divoom_product_test_boot(uint8 boot_mode)
{
	divoom_sys_setting_info sys_setting = {0};
	int cold_init_needed = !s_pt_cold_base_inited;
	int ui_init_ok;

	if (!__sync_bool_compare_and_swap(&s_pt_boot_state, 0, 1)) {
		divoom_print("divoom_product_test_boot: already started state=%d\n",
			s_pt_boot_state);
		return s_pt_boot_state == 2;
	}
	if (boot_mode != DIVOOM_PRODUCT_TEST_BOOT_UPGRADE_AGING &&
	    boot_mode != DIVOOM_PRODUCT_TEST_BOOT_FORMAT_RECOVERY)
		boot_mode = DIVOOM_PRODUCT_TEST_BOOT_NORMAL;
	s_pt_boot_mode = boot_mode;

	if (cold_init_needed) {
		divoom_log_start();
		divoom_product_test_ensure_minimal_dirs();
		divoom_system_init_system_time();
		divoom_memory_pool_init(DIVOOM_RAM_POOL_TOTAL_RAM_NUM * DIVOOM_RAM_POOL_BLOCK_RAM_SIZE,
			DIVOOM_RAM_POOL_BLOCK_RAM_SIZE,
			DIVOOM_RAM_POOL_TOTAL_MAX_RAM_NUM * DIVOOM_RAM_POOL_BLOCK_MAX_RAM_SIZE,
			DIVOOM_RAM_POOL_BLOCK_MAX_RAM_SIZE);
	}
	divoom_print("divoom_product_test_boot: start mode=%u\n", (unsigned int)s_pt_boot_mode);

#ifndef DIVOOM_SIMULATOR
#ifdef DIVOOM_TIMES_VIBE
	if (cold_init_needed && !divoom_btcore_uart_cmd_is_ready())
		divoom_btcore_uart_cmd_init(NULL);
#endif
#endif

	if (cold_init_needed)
		divoom_led_control_init();
	divoom_product_test_set_brightness(DIVOOM_PT_BRIGHTNESS_DEFAULT);
	divoom_product_test_set_volume(DIVOOM_PT_VOLUME_DEFAULT);

	sys_setting.brightness = divoom_product_test_get_brightness();
	if (cold_init_needed)
		divoom_disp_ctrl_app_init(&sys_setting);
	divoom_screen_bright(sys_setting.brightness);
	if (cold_init_needed)
		s_pt_cold_base_inited = 1;

	if (!divoom_product_test_task_start())
		goto init_failed;
	divoom_get_lvgl_lock();
	ui_init_ok = divoom_product_test_ui_init();
	divoom_free_lvgl_lock();
	if (!ui_init_ok)
		goto init_failed;

	/* 产测模式先置 active，WiFi 初始化沿用原有产测分支。 */
	divoom_product_test_set_active(1);
	if (!divoom_product_test_init())
		goto init_failed;
	divoom_product_test_post(DIVOOM_PT_MSG_CHECK_LINK, 0);
	if (!divoom_product_test_is_format_recovery())
		divoom_product_test_start_aging_audio();

#if defined(DIVOOM_TIME_FRAME_SPEAKER) && defined(DIVOOM_TIMES_VIBE)
	s_pt_wait_boot_done = 1;
#endif

	__sync_synchronize();
	s_pt_boot_state = 2;
	divoom_print("divoom_product_test_boot: done mode=%u\n",
		(unsigned int)s_pt_boot_mode);
	return 1;

init_failed:
	divoom_product_test_set_active(0);
	s_pt_boot_mode = DIVOOM_PRODUCT_TEST_BOOT_NORMAL;
	__sync_synchronize();
	s_pt_boot_state = 0;
	divoom_print("divoom_product_test_boot: init failed, retry allowed\n");
	return 0;
}

/*
 * AstroToo LVGL entry: mutual-exclusive dispatch to normal app or display test.
 * Normal app: divoom_lvgl_app_init() + divoom_main() or divoom_product_test_boot()
 * Test app:   divoom_lvgl_display_boot(mode) only (display_test.c unchanged)
 */
#ifdef CONFIG_COMPONENTS_LVGL9

#include "divoom_lvgl_init.h"
#include "divoom_lvgl_display_test.h"
#include "divoom_system.h"
#include "divoom_product_test.h"
#include "divoom_product_test_format.h"
#include "divoom_boot_health.h"
#include "divoom_product_test_wifi_fw.h"
#include "divoom_onoff.h"
#include "divoom_paths.h"
#include "../storage_stress_test.h"
#include "../middle/include/cJSON.h"

#include <console.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "lvgl.h"
#include "lv_sunxi_port_disp.h"
#include "divoom_port_os.h"

#if defined(CONFIG_DISP2_SUNXI)
#include <video/sunxi_display2.h>
extern int disp_ioctl(int cmd, void *arg);
extern s32 bsp_disp_device_switch(int disp, enum disp_output_type output_type,
				  enum disp_tv_mode mode);
extern s32 bsp_disp_get_output_type(u32 disp);
extern s32 bsp_disp_get_lcd_registered(u32 disp);
#endif

#ifndef configAPPLICATION_NORMAL_PRIORITY
#define configAPPLICATION_NORMAL_PRIORITY (15)
#endif

extern void divoom_lvgl_top_audio_tip_menu_poll(void);
extern void divoom_lvgl_top_battery_menu_poll(void);
extern void divoom_lvgl_top_phone_call_menu_poll(void);
extern void divoom_lvgl_top_toast_poll(void);
extern void divoom_tf_music_poll_songlist_refresh(void);
extern void divoom_main(void);
extern void divoom_user_key_init(void);
extern void divoom_user_key_read(lv_indev_t *indev, lv_indev_data_t *data);
extern const lv_img_dsc_t divoom_boot_logo_img;

#define LVGL_APP_DEFAULT_BRIGHTNESS	80

static lv_obj_t *s_early_boot_scr;
static lv_obj_t *s_early_boot_logo;

void divoom_boot_logo_setup(lv_obj_t *parent, lv_obj_t *logo_img)
{
	if (parent != NULL) {
		lv_obj_set_style_bg_color(parent, lv_color_hex(0x000000), 0);
		lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
	}
	if (logo_img != NULL) {
		lv_obj_set_style_bg_opa(logo_img, LV_OPA_TRANSP, 0);
		lv_obj_set_style_border_width(logo_img, 0, 0);
		lv_obj_set_style_pad_all(logo_img, 0, 0);
	}
}

void divoom_boot_logo_discard_early(void)
{
	lv_disp_t *disp = lv_disp_get_default();

	if (s_early_boot_logo != NULL) {
		lv_obj_delete(s_early_boot_logo);
		s_early_boot_logo = NULL;
	}
	if (s_early_boot_scr != NULL) {
		divoom_boot_logo_setup(s_early_boot_scr, NULL);
		lv_obj_invalidate(s_early_boot_scr);
	}
	if (disp != NULL)
		lv_refr_now(disp);
}

static SemaphoreHandle_t s_lvgl_mutex;
static SemaphoreHandle_t s_aw_pic_decoder_mutex;
static TaskHandle_t s_handler_task;
static int s_app_ready;

#if defined(CONFIG_DISP2_SUNXI)
static int divoom_lvgl_app_wait_lcd_ready(void)
{
	int i;

	for (i = 0; i < 200; i++) {
		if (bsp_disp_get_lcd_registered(0) &&
		    bsp_disp_get_output_type(0) == DISP_OUTPUT_TYPE_LCD) {
			divoom_port_delay_ms(200);
			return 0;
		}
		divoom_port_delay_ms(50);
	}

	if (bsp_disp_get_lcd_registered(0)) {
		int ret = bsp_disp_device_switch(0, DISP_OUTPUT_TYPE_LCD, 0);

		if (bsp_disp_get_output_type(0) == DISP_OUTPUT_TYPE_LCD) {
			(void)ret;
			divoom_port_delay_ms(200);
			return 0;
		}
		printf("[LVGL] LCD fallback switch failed ret=%d\n", ret);
	} else {
		printf("[LVGL] LCD driver not registered\n");
	}
	return -1;
}
#else
static int divoom_lvgl_app_wait_lcd_ready(void)
{
	return 0;
}
#endif

static void divoom_lvgl_handler_task(void *param)
{
	(void)param;

	while (1) {
		uint32_t delay;

		divoom_get_lvgl_lock();
		delay = lv_timer_handler();
		divoom_lvgl_top_audio_tip_menu_poll();
		divoom_lvgl_top_battery_menu_poll();
		divoom_lvgl_top_phone_call_menu_poll();
		divoom_tf_music_poll_songlist_refresh();
		divoom_lvgl_top_toast_poll();
		divoom_free_lvgl_lock();

		if (delay < 1)
			delay = 1;
		divoom_port_delay_ms(delay);
	}
}

static int divoom_lvgl_app_start_handler_task(void)
{
	if (s_handler_task != NULL)
		return 0;

	if (xTaskCreate(divoom_lvgl_handler_task, "lvgl_hdl",
			128 * 1024 / sizeof(StackType_t), NULL,
			DIVOOM_LVGL_HANDLER_TASK_PRIO,
			&s_handler_task) != pdPASS) {
		printf("[LVGL] handler task create failed\n");
		return -1;
	}
	return 0;
}

int divoom_get_lvgl_try_lock(void)
{
	if (s_lvgl_mutex == NULL)
		return -1;
	return xSemaphoreTakeRecursive(s_lvgl_mutex, 0) == pdTRUE ? 0 : -1;
}

void divoom_get_lvgl_lock(void)
{
	if (s_lvgl_mutex != NULL)
		xSemaphoreTakeRecursive(s_lvgl_mutex, portMAX_DELAY);
}

void divoom_free_lvgl_lock(void)
{
	if (s_lvgl_mutex != NULL)
		xSemaphoreGiveRecursive(s_lvgl_mutex);
}

static int divoom_aw_pic_decoder_mutex_init(void)
{
	SemaphoreHandle_t mutex;

	if (s_aw_pic_decoder_mutex != NULL)
		return 0;

	mutex = xSemaphoreCreateRecursiveMutex();
	if (mutex == NULL)
		return -1;

	taskENTER_CRITICAL();
	if (s_aw_pic_decoder_mutex == NULL) {
		s_aw_pic_decoder_mutex = mutex;
		mutex = NULL;
	}
	taskEXIT_CRITICAL();

	if (mutex != NULL)
		vSemaphoreDelete(mutex);

	return 0;
}

// 启动阶段显式创建图片解码互斥量，避免首帧与后台任务并发初始化。
int divoom_aw_pic_decoder_global_init(void)
{
	return divoom_aw_pic_decoder_mutex_init();
}

int divoom_aw_pic_decoder_lock(void)
{
	if (divoom_aw_pic_decoder_mutex_init() != 0)
		return -1;

	return xSemaphoreTakeRecursive(s_aw_pic_decoder_mutex, portMAX_DELAY) == pdTRUE ? 0 : -1;
}

void divoom_aw_pic_decoder_unlock(void)
{
	if (s_aw_pic_decoder_mutex != NULL)
		xSemaphoreGiveRecursive(s_aw_pic_decoder_mutex);
}

int divoom_lvgl_app_is_ready(void)
{
	return s_app_ready;
}
extern void divoom_main_del_config(void);
extern void divoom_light_app_del_config(void);

/* AstroToo 启动默认背光（与 divoom_screen_bright 共用 hw 实现） */
extern void divoom_screen_hw_backlight_set(int brightness);

#if CONFIG_DIVOOM_MAIN_PORT_READY
static int divoom_lvgl_boot_screen_is_on(void)
{
	FILE *fp;
	long length;
	char *json;
	size_t count;
	int close_result;
	int screen_on = 1;
	divoom_cJSON *root, *item;

	/* 主模块尚未初始化：直接读文件，不访问 light/main 句柄或调用业务接口。 */
	fp = fopen(DIVOOM_LIGHT_APP_BASE_LIGHT_SAVE_PATH, "rb");
	if (fp == NULL)
		return 1;
	if (fseek(fp, 0, SEEK_END) != 0) {
		fclose(fp);
		return 1;
	}
	length = ftell(fp);
	if (length <= 0 || (unsigned long)length >= SIZE_MAX || fseek(fp, 0, SEEK_SET) != 0) {
		fclose(fp);
		return 1;
	}
	json = malloc((size_t)length + 1);
	if (json == NULL) {
		fclose(fp);
		return 1;
	}
	count = fread(json, 1, (size_t)length, fp);
	close_result = fclose(fp);
	if (count != (size_t)length || close_result != 0) {
		free(json);
		return 1;
	}
	json[count] = '\0';
	root = divoom_cJSON_Parse(json);
	free(json);
	item = divoom_cJSON_GetObjectItem(root, "light_switch");
	/* 与主模块加载规则一致：只有明确保存的数字 0 表示关屏。 */
	if (item != NULL && item->type == divoom_cJSON_Number && item->valuedouble == 0)
		screen_on = 0;
	divoom_cJSON_Delete(root);
	return screen_on;
}
#endif

int divoom_lvgl_app_init(void)
{
	lv_indev_t *kp;

	if (s_app_ready)
		return 0;

	if (divoom_lvgl_app_wait_lcd_ready() != 0)
		printf("[LVGL] continue without confirmed LCD switch\n");

	if (lv_display_get_default() == NULL) {
		lv_init();
		lv_sunxifb_port_disp_init();
	}

	if (s_lvgl_mutex == NULL) {
		s_lvgl_mutex = xSemaphoreCreateRecursiveMutex();
		if (s_lvgl_mutex == NULL) {
			printf("[LVGL] mutex create failed\n");
			return -1;
		}
	}
	if (divoom_aw_pic_decoder_mutex_init() != 0) {
		printf("[LVGL] pic decoder mutex create failed\n");
		return -1;
	}

	divoom_user_key_init();
	kp = lv_indev_create();
	if (kp != NULL) {
		lv_indev_set_type(kp, LV_INDEV_TYPE_KEYPAD);
		lv_indev_set_read_cb(kp, divoom_user_key_read);
	}

    /* 背光前先刷黑底 + 内嵌开机 logo，避免开机动画加载前长时间黑屏 */
    divoom_get_lvgl_lock();
    {
        lv_obj_t *scr = lv_scr_act();
        if (scr) {
            lv_obj_t *logo_img;

            s_early_boot_scr = scr;
            s_early_boot_logo = NULL;
            logo_img = divoom_product_test_format_needs_recovery() ? NULL : lv_img_create(scr);
            if (logo_img != NULL) {
                s_early_boot_logo = logo_img;
                divoom_boot_logo_setup(scr, logo_img);
                lv_img_set_src(logo_img, &divoom_boot_logo_img);
                lv_obj_center(logo_img);
            } else {
                divoom_boot_logo_setup(scr, NULL);
            }
            lv_obj_invalidate(scr);
        }
        lv_task_handler();
    }
    divoom_free_lvgl_lock();

#if CONFIG_DIVOOM_MAIN_PORT_READY
	/* LOGO 和后续开机视频沿用同一开关屏状态，避免先亮再灭再亮。 */
	divoom_screen_hw_backlight_set(divoom_lvgl_boot_screen_is_on()
		? LVGL_APP_DEFAULT_BRIGHTNESS : 0);
#else
	divoom_screen_hw_backlight_set(LVGL_APP_DEFAULT_BRIGHTNESS);
#endif

	if (divoom_lvgl_app_start_handler_task() != 0)
		return -1;

	s_app_ready = 1;
	return 0;
}

static void divoom_lvgl_app_boot_task(void *param)
{
	int poweron_mode = 1;	// DIVOOM_ONOFF_MODE_NORMAL. 见 DIVOOM_ONOFF_MODE_TYPE 定义
	int product_test_reboot = 0;
	uint8_t product_test_boot_mode = DIVOOM_PRODUCT_TEST_BOOT_NORMAL;
	
	(void)param;

	if (divoom_lvgl_app_init() != 0) {
		printf("[LVGL] app boot: init failed\n");
		vTaskDelete(NULL);
		return;
	}

#if DIVOOM_STORAGE_STRESS_TEST
	/* Recovery retains priority over the diagnostic mode. */
	if (!divoom_product_test_format_needs_recovery()) {
		extern int divoom_storage_test_ui_start(void);
		divoom_get_lvgl_lock();
		divoom_boot_logo_discard_early();
		int ui_ready = divoom_storage_test_ui_start();
		divoom_free_lvgl_lock();
		divoom_screen_hw_backlight_set(LVGL_APP_DEFAULT_BRIGHTNESS);
		if (ui_ready)
			storage_stress_test_start();
		else
			printf("[storage_test] UI init failed; test not started\n");
		vTaskDelete(NULL);
		return;
	}
#endif

    /* A physical ESC/test boot always opens the test menu, even with an old
     * C/P/F/M/R marker or mount failure. Keep durable repair records intact. */
#if defined(DIVOOM_SIMULATOR) && DIVOOM_SIMULATOR
    poweron_mode = 1;
#else
    poweron_mode = divoom_onoff_query_jl_product_test_boot();
#endif
    if (poweron_mode == 2) {
        uint8_t ignored;
        (void)divoom_onoff_take_product_test_boot(&ignored);
        product_test_boot_mode = DIVOOM_PRODUCT_TEST_BOOT_NORMAL;
    } else if (divoom_product_test_format_needs_recovery()) {
        poweron_mode = 2;
        product_test_boot_mode = DIVOOM_PRODUCT_TEST_BOOT_FORMAT_RECOVERY;
    } else {
#if !defined(DIVOOM_SIMULATOR) || !DIVOOM_SIMULATOR
        product_test_reboot = divoom_onoff_take_product_test_boot(&product_test_boot_mode);
        if (product_test_reboot) poweron_mode = 2;
#endif
    }

#if CONFIG_DIVOOM_MAIN_PORT_READY
	if (poweron_mode == 2) { 	// DIVOOM_ONOFF_MODE_TEST
#if !DIVOOM_PRODUCT_TEST_NO_SAVE_CONFIG
		if (product_test_boot_mode != DIVOOM_PRODUCT_TEST_BOOT_FORMAT_RECOVERY &&
            !divoom_product_test_format_needs_recovery()) {
			divoom_main_del_config();
			divoom_light_app_del_config();
		}
#endif
		printf("[LVGL] enter product-test mode=%u, reboot_marker=%d\n",
			(unsigned)product_test_boot_mode, product_test_reboot);
		while (!divoom_product_test_boot(product_test_boot_mode)) {
			printf("[LVGL] product-test boot failed; retry in 2s\n");
			divoom_system_delay_ms(2000);
		}
	}
    else if (divoom_boot_health_begin() != 0) {
        while (!divoom_product_test_boot(DIVOOM_PRODUCT_TEST_BOOT_FORMAT_RECOVERY))
            divoom_system_delay_ms(2000);
    }
    else {
#if defined(CONFIG_DRIVERS_AIC8800D80)
		/* Firmware maintenance belongs only to the selected normal-app path. */
		if (!divoom_product_test_wifi_fw_restore())
			printf("[wifi_fw] boot repair failed; recovery backup retained\n");
		if (!divoom_product_test_wifi_fw_maintain_backup())
			printf("[wifi_fw] boot backup unavailable\n");
		divoom_product_test_wifi_fw_recovery_resume();
#endif
		divoom_main();
	}
#else
	printf("[LVGL] app init done; divoom_main skipped (CONFIG_DIVOOM_MAIN_PORT_READY=0)\n");
#endif
	vTaskDelete(NULL);
}

void divoom_lvgl_boot(void)
{
#if DIVOOM_LVGL_ENABLE_TEST
	divoom_lvgl_display_boot(4);
	return;
#endif

#if DIVOOM_LVGL_ENABLE_APP
	if (xTaskCreate(divoom_lvgl_app_boot_task, "lvgl_app", 0x8000, NULL,
			DIVOOM_LVGL_HANDLER_TASK_PRIO,
			NULL) != pdPASS)
		printf("[LVGL] app boot task create failed\n");
#endif

}

static int divoom_lvgl_boot_app_cmd(int argc, char **argv)
{
	(void)argc;
	(void)argv;
#if DIVOOM_LVGL_ENABLE_APP
	divoom_lvgl_boot();
#else
	printf("[LVGL] app path disabled (DIVOOM_LVGL_ENABLE_APP=0)\n");
#endif
	return 0;
}

FINSH_FUNCTION_EXPORT_CMD(divoom_lvgl_boot_app_cmd, lv_boot,
			  Boot LVGL normal app (divoom_main));

#endif /* CONFIG_COMPONENTS_LVGL9 */

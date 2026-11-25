/*
 * AstroToo 480x480 LVGL display bring-up tests.
 * No external assets; run from FinSH after LCD is enabled.
 */
#ifdef CONFIG_COMPONENTS_LVGL9

#include "divoom_finsh_autotest.h"
#if DIVOOM_FINSH_AUTOTEST

#include "divoom_lvgl_display_test.h"

#include <console.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "lvgl.h"
#include "lv_sunxi_port_disp.h"
#include "divoom_lvgl_flip_calendar.h"
#include "divoom_silent_snow_bg.h"

#if defined(CONFIG_DRIVERS_SUNXI_PWM)
#include <string.h>
#include <sunxi_hal_pwm_ng.h>
#endif
#if defined(CONFIG_DISP2_SUNXI)
#include <video/sunxi_display2.h>
extern int disp_ioctl(int cmd, void *arg);
extern s32 bsp_disp_device_switch(int disp, enum disp_output_type output_type,
				  enum disp_tv_mode mode);
extern s32 bsp_disp_get_output_type(u32 disp);
extern s32 bsp_disp_get_lcd_registered(u32 disp);
#endif

#define LVGL_TEST_BL_PWM_PORT		0
#define LVGL_TEST_BL_PWM_CH		2
#define LVGL_TEST_BL_PWM_PERIOD_NS	20000
#define LVGL_TEST_BL_PWM_POLARITY	1

static void lvgl_test_backlight_on(int brightness)
{
#if defined(CONFIG_DRIVERS_SUNXI_PWM)
	struct pwm_config cfg;
	unsigned int duty_ns;
	static int pwm_inited;

	if (brightness < 0)
		brightness = 0;
	if (brightness > 100)
		brightness = 100;

	duty_ns = (unsigned int)brightness * LVGL_TEST_BL_PWM_PERIOD_NS / 100;
	if (!pwm_inited) {
		hal_pwm_init(LVGL_TEST_BL_PWM_PORT);
		pwm_inited = 1;
	}
	memset(&cfg, 0, sizeof(cfg));
	cfg.period_ns = LVGL_TEST_BL_PWM_PERIOD_NS;
	cfg.duty_ns = duty_ns;
	cfg.polarity = LVGL_TEST_BL_PWM_POLARITY;
	cfg.output_mode = false;
	hal_pwm_control(LVGL_TEST_BL_PWM_PORT, LVGL_TEST_BL_PWM_CH, &cfg);
#else
	(void)brightness;
#endif

#if defined(CONFIG_DISP2_SUNXI)
	{
		unsigned long args[2];
		int level = (brightness * 255 + 50) / 100;

		args[0] = 0;
		args[1] = (unsigned long)level;
		disp_ioctl(DISP_LCD_SET_BRIGHTNESS, args);
	}
#endif
}

#ifndef configAPPLICATION_NORMAL_PRIORITY
#define configAPPLICATION_NORMAL_PRIORITY (15)
#endif

#define LVGL_TEST_QUEUE_LEN	4

static TaskHandle_t s_lv_task;
static QueueHandle_t s_mode_queue;
static lv_timer_t *s_test_timer;
static int s_color_idx;
static lv_obj_t *s_anim_box;
static lv_obj_t *s_info_label;
static lv_obj_t *s_flip_calendar;
static lv_obj_t *s_silent_snow_bg;
static int s_flip_day;

static void lvgl_test_print_help(void)
{
	printf("=== LVGL display test (480x480) ===\n");
	printf("  boot default: mode 4 (moving box)\n");
	printf("  lv_test          list tests\n");
	printf("  lv_test 1        color bars (RGBW)\n");
	printf("  lv_test 2        solid color cycle\n");
	printf("  lv_test 3        basic widgets\n");
	printf("  lv_test 4        moving box animation\n");
	printf("  lv_test 5        checkerboard grid\n");
	printf("  lv_test 6        corner markers + center cross\n");
	printf("  lv_test 7        full-screen label stress\n");
	printf("  lv_test 8        flip calendar control demo\n");
	printf("  lv_test 9        Silent Snow background widget\n");
}

static void lvgl_test_clear_screen(void)
{
	if (s_test_timer) {
		lv_timer_del(s_test_timer);
		s_test_timer = NULL;
	}
	lv_anim_delete_all();
	s_anim_box = NULL;
	s_info_label = NULL;
	s_flip_calendar = NULL;
	s_silent_snow_bg = NULL;
	s_flip_day = 0;
	lv_obj_clean(lv_screen_active());
}

static lv_obj_t *lvgl_test_add_bar(lv_obj_t *parent, int x, int y, int w, int h,
				   uint32_t color)
{
	lv_obj_t *obj = lv_obj_create(parent);

	lv_obj_remove_style_all(obj);
	lv_obj_set_size(obj, w, h);
	lv_obj_set_pos(obj, x, y);
	lv_obj_set_style_bg_color(obj, lv_color_hex(color), 0);
	lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
	lv_obj_set_style_border_width(obj, 0, 0);
	lv_obj_set_style_radius(obj, 0, 0);
	return obj;
}

static lv_obj_t *lvgl_test_add_label(lv_obj_t *parent, const char *text,
				     lv_align_t align, int x_ofs, int y_ofs)
{
	lv_obj_t *label = lv_label_create(parent);

	lv_label_set_text(label, text);
	lv_obj_set_style_text_color(label, lv_color_white(), 0);
	lv_obj_align(label, align, x_ofs, y_ofs);
	return label;
}

static void lvgl_test_show_resolution_banner(const char *title)
{
	char buf[96];
	lv_display_t *disp = lv_display_get_default();
	int32_t w = disp ? lv_display_get_horizontal_resolution(disp) : 0;
	int32_t h = disp ? lv_display_get_vertical_resolution(disp) : 0;

	snprintf(buf, sizeof(buf), "%s  %ldx%ld", title, (long)w, (long)h);
	s_info_label = lvgl_test_add_label(lv_screen_active(), buf,
					   LV_ALIGN_TOP_MID, 0, 8);
	lv_obj_set_style_text_color(s_info_label, lv_color_black(), 0);
}

static void lvgl_test_create_color_bars(void)
{
	static const uint32_t colors[] = {
		0xFF0000, 0x00FF00, 0x0000FF, 0xFFFFFF, 0x000000, 0x808080,
	};
	lv_display_t *disp = lv_display_get_default();
	int32_t w = lv_display_get_horizontal_resolution(disp);
	int32_t h = lv_display_get_vertical_resolution(disp);
	int32_t bar_h = h / (int32_t)(sizeof(colors) / sizeof(colors[0]));
	int i;

	for (i = 0; i < (int)(sizeof(colors) / sizeof(colors[0])); i++)
		lvgl_test_add_bar(lv_screen_active(), 0, i * bar_h, w, bar_h,
				  colors[i]);

	lvgl_test_show_resolution_banner("Color bars");
}

static void lvgl_test_color_cycle_cb(lv_timer_t *timer)
{
	static const uint32_t colors[] = {
		0xFF0000, 0x00FF00, 0x0000FF, 0xFFFF00, 0xFF00FF, 0x00FFFF,
		0xFFFFFF, 0x000000,
	};
	lv_obj_t *scr = lv_screen_active();
	char buf[32];

	(void)timer;
	lv_obj_set_style_bg_color(scr, lv_color_hex(colors[s_color_idx]), 0);
	lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

	if (s_info_label) {
		snprintf(buf, sizeof(buf), "Solid color #%d", s_color_idx + 1);
		lv_label_set_text(s_info_label, buf);
		lv_obj_set_style_text_color(s_info_label,
			colors[s_color_idx] == 0x000000 ? lv_color_white()
							: lv_color_black(), 0);
	}
	s_color_idx = (s_color_idx + 1) % (int)(sizeof(colors) / sizeof(colors[0]));
}

static void lvgl_test_create_solid_cycle(void)
{
	lv_obj_t *scr = lv_screen_active();

	lv_obj_set_style_bg_color(scr, lv_color_hex(0xFF0000), 0);
	lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
	s_color_idx = 0;
	s_info_label = lvgl_test_add_label(scr, "Solid color #1",
					   LV_ALIGN_CENTER, 0, 0);
	s_test_timer = lv_timer_create(lvgl_test_color_cycle_cb, 1000, NULL);
}

static void lvgl_test_btn_event_cb(lv_event_t *e)
{
	lv_obj_t *label = lv_event_get_user_data(e);

	if (lv_event_get_code(e) == LV_EVENT_CLICKED && label)
		lv_label_set_text(label, "Button clicked!");
}

static void lvgl_test_create_widgets(void)
{
	lv_obj_t *scr = lv_screen_active();
	lv_obj_t *title;
	lv_obj_t *btn;
	lv_obj_t *btn_label;
	lv_obj_t *slider;
	lv_obj_t *arc;
	lv_obj_t *status;

	lv_obj_set_style_bg_color(scr, lv_color_hex(0x202020), 0);
	lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

	title = lvgl_test_add_label(scr, "LVGL Widget Test", LV_ALIGN_TOP_MID, 0, 12);

	btn = lv_button_create(scr);
	lv_obj_set_size(btn, 140, 44);
	lv_obj_align(btn, LV_ALIGN_LEFT_MID, 24, -40);
	status = lv_label_create(scr);
	lv_label_set_text(status, "Tap button");
	lv_obj_set_style_text_color(status, lv_color_white(), 0);
	lv_obj_align(status, LV_ALIGN_BOTTOM_MID, 0, -16);
	btn_label = lv_label_create(btn);
	lv_label_set_text(btn_label, "OK");
	lv_obj_center(btn_label);
	lv_obj_add_event_cb(btn, lvgl_test_btn_event_cb, LV_EVENT_CLICKED, status);

	slider = lv_slider_create(scr);
	lv_obj_set_width(slider, 180);
	lv_slider_set_value(slider, 60, LV_ANIM_OFF);
	lv_obj_align(slider, LV_ALIGN_RIGHT_MID, -24, -40);

	arc = lv_arc_create(scr);
	lv_obj_set_size(arc, 120, 120);
	lv_arc_set_value(arc, 75);
	lv_obj_align(arc, LV_ALIGN_CENTER, 0, 36);

	(void)title;
}

static void lvgl_test_anim_exec_x(void *obj, int32_t v)
{
	lv_obj_set_x((lv_obj_t *)obj, v);
}

static void lvgl_test_create_animation(void)
{
	lv_display_t *disp = lv_display_get_default();
	int32_t w = lv_display_get_horizontal_resolution(disp);
	int32_t h = lv_display_get_vertical_resolution(disp);
	lv_anim_t anim;

	lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x001040), 0);
	lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_COVER, 0);

	s_anim_box = lvgl_test_add_bar(lv_screen_active(), 0, h / 2 - 30, 60, 60,
				       0xFFD000);
	lvgl_test_add_label(lv_screen_active(), "Animation", LV_ALIGN_TOP_MID, 0, 8);

	lv_anim_init(&anim);
	lv_anim_set_var(&anim, s_anim_box);
	lv_anim_set_exec_cb(&anim, lvgl_test_anim_exec_x);
	lv_anim_set_values(&anim, 0, w - 60);
	lv_anim_set_duration(&anim, 2000);
	lv_anim_set_playback_duration(&anim, 2000);
	lv_anim_set_repeat_count(&anim, LV_ANIM_REPEAT_INFINITE);
	lv_anim_start(&anim);
}

static void lvgl_test_create_checkerboard(void)
{
	lv_display_t *disp = lv_display_get_default();
	int32_t w = lv_display_get_horizontal_resolution(disp);
	int32_t h = lv_display_get_vertical_resolution(disp);
	int32_t cell = 40;
	int cols = w / cell;
	int rows = h / cell;
	int r, c;

	lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0xFFFFFF), 0);
	lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_COVER, 0);

	for (r = 0; r < rows; r++) {
		for (c = 0; c < cols; c++) {
			uint32_t color = ((r + c) & 1) ? 0xFFFFFF : 0x303030;

			lvgl_test_add_bar(lv_screen_active(), c * cell, r * cell,
					  cell, cell, color);
		}
	}
	lvgl_test_show_resolution_banner("Checkerboard");
}

static void lvgl_test_create_corner_markers(void)
{
	lv_display_t *disp = lv_display_get_default();
	int32_t w = lv_display_get_horizontal_resolution(disp);
	int32_t h = lv_display_get_vertical_resolution(disp);
	int32_t m = 24;

	lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x404040), 0);
	lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_COVER, 0);

	lvgl_test_add_bar(lv_screen_active(), 0, 0, m, m, 0xFF0000);
	lvgl_test_add_bar(lv_screen_active(), w - m, 0, m, m, 0x00FF00);
	lvgl_test_add_bar(lv_screen_active(), 0, h - m, m, m, 0x0000FF);
	lvgl_test_add_bar(lv_screen_active(), w - m, h - m, m, m, 0xFFFF00);
	lvgl_test_add_bar(lv_screen_active(), w / 2 - 2, 0, 4, h, 0xFFFFFF);
	lvgl_test_add_bar(lv_screen_active(), 0, h / 2 - 2, w, 4, 0xFFFFFF);
	lvgl_test_show_resolution_banner("Alignment");
}

static void lvgl_test_create_label_stress(void)
{
	lv_obj_t *scr = lv_screen_active();
	static const uint32_t colors[] = {
		0x00FF88, 0xFF8800, 0x88AAFF, 0xFF4488, 0xFFFF00,
		0x00FFFF, 0xFF00FF, 0xFFFFFF,
	};
	char line[64];
	int i;

	lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0);
	lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

	for (i = 0; i < 8; i++) {
		lv_obj_t *label = lv_label_create(scr);

		snprintf(line, sizeof(line), "Line %d: LVGL9 AstroToo 480x480", i + 1);
		lv_label_set_text(label, line);
		lv_obj_set_style_text_color(label, lv_color_hex(colors[i]), 0);
		lv_obj_align(label, LV_ALIGN_TOP_LEFT, 8, 8 + i * 28);
	}
}

static void lvgl_test_flip_calendar_cb(lv_timer_t *timer)
{
	char buf[3];

	(void)timer;
	if (!s_flip_calendar || !lv_obj_is_valid(s_flip_calendar))
	{
		return;
	}

	s_flip_day++;
	if (s_flip_day > 31)
	{
		s_flip_day = 1;
	}
	if (s_flip_day < 1)
	{
		s_flip_day = 1;
	}
	/* 日期只显示 01..31，两字符手写可避免真机 -Wformat-truncation 误判。 */
	buf[0] = (char)('0' + (s_flip_day / 10));
	buf[1] = (char)('0' + (s_flip_day % 10));
	buf[2] = '\0';
	divoom_lvgl_flip_calendar_flip_to(s_flip_calendar, buf);
}

static void lvgl_test_create_flip_calendar(void)
{
	lv_obj_t *scr = lv_screen_active();
	lv_obj_t *title;

	lv_obj_set_style_bg_color(scr, lv_color_hex(0x0B0D0F), 0);
	lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

	title = lvgl_test_add_label(scr, "Flip Calendar", LV_ALIGN_TOP_MID, 0, 30);
	lv_obj_set_style_text_color(title, lv_color_hex(0xE8C77E), 0);

	s_flip_calendar = divoom_lvgl_flip_calendar_create(scr);
	if (!s_flip_calendar)
		return;

	divoom_lvgl_flip_calendar_set_size(s_flip_calendar, 160, 120);
	divoom_lvgl_flip_calendar_set_text(s_flip_calendar, "11");
	lv_obj_align(s_flip_calendar, LV_ALIGN_CENTER, 0, -8);

	s_flip_day = 11;
	s_test_timer = lv_timer_create(lvgl_test_flip_calendar_cb, 3000, NULL);
	lv_timer_ready(s_test_timer);
}

static void lvgl_test_create_silent_snow_bg(void)
{
	lv_obj_t *scr = lv_screen_active();

	lv_obj_set_style_bg_color(scr, lv_color_hex(0x0D1422), 0);
	lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

	/* 仅展示背景控件本体，避免时间/天气/文案干扰雪花和底部雪丘验收。 */
	s_silent_snow_bg = divoom_silent_snow_bg_create(scr);
	if (!s_silent_snow_bg) {
		return;
	}

	lv_obj_set_size(s_silent_snow_bg, lv_pct(100), lv_pct(100));
	lv_obj_align(s_silent_snow_bg, LV_ALIGN_CENTER, 0, 0);
	divoom_silent_snow_bg_set_seed(s_silent_snow_bg, 0x74007401u);
}

static void lvgl_test_build_scene(int mode)
{
	lvgl_test_clear_screen();

	switch (mode) {
	case 1:
		lvgl_test_create_color_bars();
		break;
	case 2:
		lvgl_test_create_solid_cycle();
		break;
	case 3:
		lvgl_test_create_widgets();
		break;
	case 4:
		lvgl_test_create_animation();
		break;
	case 5:
		lvgl_test_create_checkerboard();
		break;
	case 6:
		lvgl_test_create_corner_markers();
		break;
	case 7:
		lvgl_test_create_label_stress();
		break;
	case 8:
		lvgl_test_create_flip_calendar();
		break;
	case 9:
		lvgl_test_create_silent_snow_bg();
		break;
	default:
		printf("[LVGL_TEST] unknown mode %d\n", mode);
		lvgl_test_print_help();
		break;
	}
}

static void lvgl_test_drain_mode_queue(void)
{
	int mode, latest = -1;

	if (!s_mode_queue)
		return;

	while (xQueueReceive(s_mode_queue, &mode, 0) == pdTRUE)
		latest = mode;

	if (latest < 0)
		return;

	lvgl_test_build_scene(latest);
	lv_obj_invalidate(lv_screen_active());
	printf("[LVGL_TEST] running mode %d\n", latest);
}

static int lvgl_test_queue_mode(int mode)
{
	if (!s_mode_queue || !s_lv_task)
		return -1;

	if (xQueueSend(s_mode_queue, &mode, 0) != pdTRUE) {
		xQueueReset(s_mode_queue);
		if (xQueueSend(s_mode_queue, &mode, 0) != pdTRUE)
			return -1;
	}

	xTaskNotifyGive(s_lv_task);
	return 0;
}

static void lvgl_test_task(void *param)
{
	(void)param;

	while (1) {
		lvgl_test_drain_mode_queue();

		uint32_t delay = lv_timer_handler();

		lvgl_test_drain_mode_queue();

		if (delay < 1)
			delay = 1;
		/* Wake immediately when FinSH queues a new mode */
		ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(delay));
	}
}

static int lvgl_test_display_init(void)
{
	if (lv_display_get_default() != NULL)
		return 0;

	lv_init();
	lv_sunxifb_port_disp_init();
	printf("[LVGL_TEST] lv_init + disp port done\n");
	return 0;
}

static int lvgl_test_start_handler_task(void)
{
	if (s_lv_task != NULL)
		return 0;

	if (!s_mode_queue)
		s_mode_queue = xQueueCreate(LVGL_TEST_QUEUE_LEN, sizeof(int));

	if (!s_mode_queue) {
		printf("[LVGL_TEST] mode queue create failed\n");
		return -1;
	}

	if (xTaskCreate(lvgl_test_task, "lvgl_test", 0xC000, NULL,
			configAPPLICATION_NORMAL_PRIORITY,
			&s_lv_task) != pdPASS) {
		printf("[LVGL_TEST] create task failed\n");
		return -1;
	}
	return 0;
}

#if defined(CONFIG_DISP2_SUNXI)
static int lvgl_test_wait_lcd_ready(void)
{
	int i;

	/* Wait for disp boot work; do not switch unless timeout */
	for (i = 0; i < 200; i++) {
		if (bsp_disp_get_lcd_registered(0) &&
		    bsp_disp_get_output_type(0) == DISP_OUTPUT_TYPE_LCD) {
			printf("[LVGL] LCD ready (try=%d)\n", i);
			usleep(200 * 1000);
			return 0;
		}
		usleep(50 * 1000);
	}

	if (bsp_disp_get_lcd_registered(0)) {
		int ret = bsp_disp_device_switch(0, DISP_OUTPUT_TYPE_LCD, 0);

		if (bsp_disp_get_output_type(0) == DISP_OUTPUT_TYPE_LCD) {
			printf("[LVGL] LCD fallback switch ret=%d\n", ret);
			usleep(200 * 1000);
			return 0;
		}
		printf("[LVGL] LCD fallback switch failed ret=%d\n", ret);
	} else {
		printf("[LVGL] LCD driver not registered\n");
	}
	return -1;
}
#endif

static void lvgl_boot_task(void *param)
{
	int mode = (int)(intptr_t)param;

#if defined(CONFIG_DISP2_SUNXI)
	if (lvgl_test_wait_lcd_ready() != 0)
		printf("[LVGL] continue without confirmed LCD switch\n");
#endif

	divoom_lvgl_display_test_run(mode);
	vTaskDelete(NULL);
}

void divoom_lvgl_display_boot(int mode)
{
	TaskHandle_t task;

	if (mode < 1 || mode > 9) {
		mode = 1;
	}

	if (xTaskCreate(lvgl_boot_task, "lvgl_boot", 0x8000,
			(void *)(intptr_t)mode, configAPPLICATION_NORMAL_PRIORITY,
			&task) != pdPASS)
		printf("[LVGL] boot task create failed\n");
}

int divoom_lvgl_display_test_run(int mode)
{
	if (mode < 1 || mode > 9) {
		lvgl_test_print_help();
		return 0;
	}

	if (lvgl_test_display_init() != 0)
		return -1;

	lvgl_test_backlight_on(80);

	if (s_lv_task != NULL) {
		if (lvgl_test_queue_mode(mode) != 0) {
			printf("[LVGL_TEST] queue mode %d failed\n", mode);
			return -1;
		}
		printf("[LVGL_TEST] switch to mode %d\n", mode);
		return 0;
	}

	lvgl_test_build_scene(mode);

	if (lvgl_test_start_handler_task() != 0)
		return -1;

	printf("[LVGL_TEST] running mode %d\n", mode);
	return 0;
}

static int lvgl_test_arg_to_mode(int argc, char **argv)
{
	if (argc >= 2 && argv[1] && argv[1][0])
		return atoi(argv[1]);
	if (argc >= 1 && argv[0] && argv[0][0] >= '0' && argv[0][0] <= '9')
		return atoi(argv[0]);
	return 0;
}

int divoom_lvgl_display_test_main(int argc, char **argv)
{
	int mode = lvgl_test_arg_to_mode(argc, argv);

	if (mode < 1 || mode > 9) {
		lvgl_test_print_help();
		return 0;
	}

	return divoom_lvgl_display_test_run(mode);
}

FINSH_FUNCTION_EXPORT_CMD(divoom_lvgl_display_test_main, lv_test,
			  LVGL 480x480 display tests);

#endif /* DIVOOM_FINSH_AUTOTEST */
#endif /* CONFIG_COMPONENTS_LVGL9 */

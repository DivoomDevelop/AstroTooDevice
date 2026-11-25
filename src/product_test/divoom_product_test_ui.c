#include "divoom.h"
#include "divoom_product_test.h"
#include "divoom_product_test_usr_check.h"
#include "divoom_product_test_ui.h"
#include "divoom_menu_control.h"
#include "divoom_lvgl_product_test_menu.h"
#include "divoom_pt_ui.h"
#include "divoom_lvgl_poweron.h"
#include "divoom_local_movie_play.h"
#include "lvgl.h"

#ifdef DIVOOM_SIMULATOR
/* 产测页面共享业务代码，仿真 LVGL8 仅转换显示接口。 */
typedef lv_disp_t lv_display_t;
#define lv_display_get_default lv_disp_get_default
#define lv_screen_active lv_scr_act
#define lv_display_get_horizontal_resolution lv_disp_get_hor_res
#define lv_display_get_vertical_resolution lv_disp_get_ver_res
#endif

#define DIVOOM_PT_AGING_VIDEO_RETRY_MS 2000u

static lv_group_t *s_pt_ui_group;
static void *s_pt_screen;
static lv_obj_t *s_pt_aging_overlay;
static lv_obj_t *s_pt_aging_receive_label;
static lv_timer_t *s_pt_aging_receive_timer;
static lv_timer_t *s_pt_aging_video_timer;
static lv_obj_t *s_pt_aging_video_parent;
static int s_pt_aging_overlay_active;
static int s_pt_aging_video_requested;
static int s_pt_aging_video_stopping;
static int s_pt_aging_video_mode;
static int s_pt_aging_video_retry_set;
static uint32_t s_pt_aging_video_retry_ms;
/* Static owner survives screen destruction and a deferred pipeline shutdown. */
static char s_pt_aging_video_owner;
static volatile int s_pt_aging_video_replay;
static volatile int s_pt_aging_video_paused;
static volatile int s_pt_aging_video_starting;

static int divoom_product_test_ui_upgrade_in_progress(void)
{
	int flag = divoom_product_test_get_download_file_flag();

	return flag == DIVOOM_PT_DL_DOWNLOADING ||
		flag == DIVOOM_PT_DL_UNZIPPING ||
		flag == DIVOOM_PT_DL_FLASHING ||
		flag == DIVOOM_PT_DL_JL_UPDATING;
}


static void divoom_product_test_ui_setup_group(void)
{
	lv_indev_t *indev;

	if (s_pt_ui_group)
		return;

	s_pt_ui_group = lv_group_create();
	lv_group_set_default(s_pt_ui_group);

	indev = lv_indev_get_next(NULL);
	while (indev) {
		if (lv_indev_get_type(indev) == LV_INDEV_TYPE_KEYPAD)
			lv_indev_set_group(indev, s_pt_ui_group);
		indev = lv_indev_get_next(indev);
	}
}

int divoom_product_test_ui_init(void)
{
	return divoom_pt_ui_init();
}

static void divoom_product_test_ui_refresh_receive_label(lv_timer_t *timer)
{
	int cur = 0;
	int total = 0;
	int percent;
	int flag;

	(void)timer;
	if (!s_pt_aging_receive_label)
		return;
	flag = divoom_product_test_get_download_file_flag();
	divoom_product_test_get_download_progress(&cur, &total);
	if (!divoom_product_test_ui_upgrade_in_progress()) {
		lv_obj_add_flag(s_pt_aging_receive_label, LV_OBJ_FLAG_HIDDEN);
		return;
	}

	if (flag == DIVOOM_PT_DL_DOWNLOADING &&
	    divoom_product_test_is_customize_updating()) {
		if (total > 0) {
			if (cur < 0)
				cur = 0;
			if (cur > total)
				cur = total;
			lv_label_set_text_fmt(s_pt_aging_receive_label,
				"Updating files\n%d/%d", cur, total);
		} else {
			lv_label_set_text(s_pt_aging_receive_label,
				"Preparing file update");
		}
	} else {
		percent = divoom_product_test_get_firmware_percent();
		if (percent < 0)
			percent = 0;
		if (percent > 100)
			percent = 100;
		if (flag == DIVOOM_PT_DL_UNZIPPING)
			lv_label_set_text_fmt(s_pt_aging_receive_label,
				"Extracting firmware\n%d%%", percent);
		else if (flag == DIVOOM_PT_DL_FLASHING)
			lv_label_set_text_fmt(s_pt_aging_receive_label,
				"Installing firmware\n%d%%", percent);
		else if (flag == DIVOOM_PT_DL_JL_UPDATING)
			lv_label_set_text_fmt(s_pt_aging_receive_label,
				"Updating JL firmware\n%d%%", percent);
		else
			lv_label_set_text_fmt(s_pt_aging_receive_label,
				"Downloading firmware\n%d%%", percent);
	}
	lv_obj_clear_flag(s_pt_aging_receive_label, LV_OBJ_FLAG_HIDDEN);
	lv_obj_move_foreground(s_pt_aging_receive_label);
}

/* Runs under the player's lock: never call LVGL or the player API here. */
static int divoom_product_test_ui_aging_video_done(void *owner, int status)
{
    (void)owner;
    return status == 0 && s_pt_aging_video_replay ? 0 : 1;
}

/* Resource-update task: close the start gate, then drain only our owner.
 * Playback completion takes neither LVGL nor this gate, so waiting is safe. */
int divoom_product_test_ui_pause_aging_video(unsigned int timeout_ms)
{
    uint32_t started = (uint32_t)divoom_system_get_tickms();
    uint32_t elapsed;
    __sync_lock_test_and_set(&s_pt_aging_video_paused, 1);
    s_pt_aging_video_replay = 0;
    __sync_synchronize();
    while (__sync_fetch_and_add(&s_pt_aging_video_starting, 0)) {
        if ((uint32_t)(divoom_system_get_tickms() - started) >= timeout_ms) return 0;
        divoom_system_delay_ms(10);
    }
    elapsed = (uint32_t)(divoom_system_get_tickms() - started);
    return divoom_local_movie_play_stop_owner(&s_pt_aging_video_owner,
        elapsed < timeout_ms ? timeout_ms - elapsed : 0) == 0;
}

void divoom_product_test_ui_resume_aging_video(void)
{
    __sync_lock_test_and_set(&s_pt_aging_video_paused, 0);
}

static int divoom_product_test_ui_stop_aging_video(unsigned int timeout_ms)
{
    s_pt_aging_video_replay = 0;
    __sync_synchronize();
    if (s_pt_aging_video_requested) {
        s_pt_aging_video_stopping =
            divoom_local_movie_play_stop_owner(&s_pt_aging_video_owner, timeout_ms) != 0;
        if (!s_pt_aging_video_stopping) s_pt_aging_video_requested = 0;
    }
    /* Restore immediately for progress/errors, and again after the decoder
     * exits. A different active screen owns its own display mode. */
    if (s_pt_aging_video_mode && s_pt_aging_video_parent &&
        s_pt_aging_video_parent == lv_screen_active())
        divoom_menu_control_change_disp_mode(s_pt_aging_video_parent, 0);
    if (!s_pt_aging_video_stopping) s_pt_aging_video_mode = 0;
    return !s_pt_aging_video_stopping;
}

static void divoom_product_test_ui_refresh_aging_video(lv_timer_t *timer)
{
    divoom_product_test_usr_check_status status;
    uint32_t now;
    (void)timer;
    if (!s_pt_aging_overlay_active || !divoom_product_test_is_upgrade_aging() ||
        !s_pt_aging_video_parent || s_pt_aging_video_parent != lv_screen_active()) {
        if (divoom_product_test_ui_stop_aging_video(0) && s_pt_aging_video_timer) {
            if (!s_pt_screen) {
                lv_timer_del(s_pt_aging_video_timer);
                s_pt_aging_video_timer = NULL;
            } else lv_timer_pause(s_pt_aging_video_timer);
        }
        return;
    }
    divoom_product_test_usr_check_get_status(&status);
    if (status.error_files || status.phase == DIVOOM_PT_USR_CHECK_FAILED) {
        divoom_product_test_ui_stop_aging_overlay();
        return;
    }
    if (s_pt_aging_video_stopping && !divoom_product_test_ui_stop_aging_video(0))
        return;
    if (s_pt_aging_video_paused || divoom_product_test_ui_upgrade_in_progress()) {
        divoom_product_test_ui_stop_aging_video(0);
        lv_obj_set_style_bg_opa(s_pt_aging_overlay, LV_OPA_COVER, 0);
        return;
    }
    if (s_pt_aging_video_requested) {
        if (!divoom_local_movie_play_is_idle()) return;
        /* Open/decoder failure: return to LVGL and retry with a bounded rate. */
        s_pt_aging_video_requested = 0;
        divoom_product_test_ui_stop_aging_video(0);
    }
    if (!divoom_local_movie_play_is_idle()) return;
    now = (uint32_t)divoom_system_get_tickms();
    if (s_pt_aging_video_retry_set &&
        (uint32_t)(now - s_pt_aging_video_retry_ms) < DIVOOM_PT_AGING_VIDEO_RETRY_MS)
        return;
    s_pt_aging_video_retry_set = 1;
    s_pt_aging_video_retry_ms = now;
    if (!divoom_port_file_exists(DIVOOM_LVGL_POWER_ON_FILE_NAME)) {
        divoom_print("product_test: aging video missing: %s\n", DIVOOM_LVGL_POWER_ON_FILE_NAME);
        return;
    }
    /* Announce before checking the task's pause gate. A writer that observed
     * no start in flight has already closed the gate before this check. */
    __sync_lock_test_and_set(&s_pt_aging_video_starting, 1);
    if (s_pt_aging_video_paused || divoom_product_test_ui_upgrade_in_progress()) {
        __sync_lock_test_and_set(&s_pt_aging_video_starting, 0);
        return;
    }
    s_pt_aging_video_replay = 1;
    __sync_synchronize();
    s_pt_aging_video_requested = 1;
    s_pt_aging_video_mode = 1;
    lv_obj_set_style_bg_opa(s_pt_aging_overlay, LV_OPA_TRANSP, 0);
    divoom_menu_control_change_disp_mode(s_pt_aging_video_parent, 1);
    lv_refr_now(lv_display_get_default());
    /* EOF callback requests in-place replay forever. Keep existing aging audio. */
    divoom_local_movie_play_play(DIVOOM_LVGL_POWER_ON_FILE_NAME, 0xFF, 0,
        divoom_product_test_ui_aging_video_done, &s_pt_aging_video_owner);
    __sync_lock_test_and_set(&s_pt_aging_video_starting, 0);
    divoom_print("product_test: aging video loop: %s\n", DIVOOM_LVGL_POWER_ON_FILE_NAME);
}

void divoom_product_test_ui_stop_aging_overlay(void)
{
    /* LVGL thread only. Keep polling only if the decoder still owns resources. */
    s_pt_aging_overlay_active = 0;
    if (divoom_product_test_ui_stop_aging_video(0)) {
        if (s_pt_aging_video_timer) lv_timer_pause(s_pt_aging_video_timer);
    } else if (s_pt_aging_video_timer) lv_timer_resume(s_pt_aging_video_timer);
    if (s_pt_aging_receive_timer) lv_timer_pause(s_pt_aging_receive_timer);
    if (s_pt_aging_overlay) lv_obj_add_flag(s_pt_aging_overlay, LV_OBJ_FLAG_HIDDEN);
}

static void divoom_product_test_ui_show_aging_overlay(void)
{
	lv_display_t *disp = lv_display_get_default();
	lv_obj_t *parent = lv_screen_active();
	int32_t width;
	int32_t height;
	divoom_product_test_usr_check_status status;

	divoom_product_test_usr_check_get_status(&status);
	if (status.error_files || status.phase == DIVOOM_PT_USR_CHECK_FAILED) {
		divoom_product_test_ui_stop_aging_overlay();
		return;
	}
	if (s_pt_aging_overlay) {
        if (!s_pt_aging_video_timer)
            s_pt_aging_video_timer = lv_timer_create(divoom_product_test_ui_refresh_aging_video, 100, NULL);
        s_pt_aging_overlay_active = s_pt_aging_video_timer != NULL;
        if (s_pt_aging_video_timer) lv_timer_resume(s_pt_aging_video_timer);
        if (s_pt_aging_receive_timer) lv_timer_resume(s_pt_aging_receive_timer);
        lv_obj_clear_flag(s_pt_aging_overlay, LV_OBJ_FLAG_HIDDEN);
		lv_obj_move_foreground(s_pt_aging_overlay);
        if (s_pt_aging_video_timer) divoom_product_test_ui_refresh_aging_video(NULL);
		return;
	}
	if (!disp || !parent)
		return;

	width = lv_display_get_horizontal_resolution(disp);
	height = lv_display_get_vertical_resolution(disp);
	if (width <= 0 || height <= 0)
		return;

	s_pt_aging_overlay = lv_obj_create(parent);
	lv_obj_remove_style_all(s_pt_aging_overlay);
	lv_obj_set_pos(s_pt_aging_overlay, 0, 0);
	lv_obj_set_size(s_pt_aging_overlay, width, height);
	lv_obj_set_style_bg_color(s_pt_aging_overlay, lv_color_black(), 0);
	lv_obj_set_style_bg_opa(s_pt_aging_overlay, LV_OPA_COVER, 0);
	lv_obj_clear_flag(s_pt_aging_overlay, LV_OBJ_FLAG_SCROLLABLE);

	s_pt_aging_overlay_active = 1;
    s_pt_aging_video_parent = parent;
    if (!s_pt_aging_video_timer)
        s_pt_aging_video_timer = lv_timer_create(divoom_product_test_ui_refresh_aging_video, 100, NULL);
    if (s_pt_aging_video_timer) lv_timer_resume(s_pt_aging_video_timer);
    else {
        s_pt_aging_overlay_active = 0;
        divoom_print("product_test: aging video timer create failed\n");
    }

	s_pt_aging_receive_label = lv_label_create(s_pt_aging_overlay);
	lv_obj_set_style_text_font(s_pt_aging_receive_label,
		LV_FONT_DEFAULT, 0);
	lv_obj_set_width(s_pt_aging_receive_label, 320);
	lv_obj_set_style_text_align(s_pt_aging_receive_label, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_style_text_color(s_pt_aging_receive_label, lv_color_white(), 0);
	lv_obj_set_style_bg_color(s_pt_aging_receive_label, lv_color_black(), 0);
	lv_obj_set_style_bg_opa(s_pt_aging_receive_label, LV_OPA_80, 0);
	lv_obj_set_style_pad_all(s_pt_aging_receive_label, 12, 0);
	lv_obj_align(s_pt_aging_receive_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_add_flag(s_pt_aging_receive_label, LV_OBJ_FLAG_HIDDEN);
	s_pt_aging_receive_timer = lv_timer_create(
		divoom_product_test_ui_refresh_receive_label, 200, NULL);
	divoom_product_test_ui_refresh_receive_label(NULL);
    if (s_pt_aging_video_timer) divoom_product_test_ui_refresh_aging_video(NULL);
	lv_obj_move_foreground(s_pt_aging_overlay);
}

lv_group_t *divoom_product_test_ui_get_group(void)
{
	return s_pt_ui_group;
}

void *divoom_product_test_ui_get_screen(void)
{
	return s_pt_screen;
}

void *divoom_product_test_ui_lvgl_show(void)
{
	divoom_product_test_ui_setup_group();

	if (s_pt_screen == NULL)
		s_pt_screen = divoom_product_test_screen_create();
	if (s_pt_screen)
		divoom_product_test_screen_show(s_pt_screen);
	if (s_pt_screen && divoom_product_test_is_upgrade_aging())
		divoom_product_test_ui_show_aging_overlay();

	return s_pt_screen;
}

void divoom_product_test_ui_lvgl_close(void)
{
    s_pt_aging_overlay_active = 0;
    /* This owner callback takes no UI locks, so bounded shutdown cannot wait
     * for LVGL. If the backend is still busy, its timer finishes cleanup. */
    if (divoom_product_test_ui_stop_aging_video(500)) {
        if (s_pt_aging_video_timer) lv_timer_del(s_pt_aging_video_timer);
        s_pt_aging_video_timer = NULL;
    } else if (s_pt_aging_video_timer) {
        lv_timer_resume(s_pt_aging_video_timer);
        divoom_print("product_test: aging video shutdown pending\n");
    }
	if (s_pt_aging_receive_timer) {
		lv_timer_del(s_pt_aging_receive_timer);
		s_pt_aging_receive_timer = NULL;
	}
    s_pt_aging_video_parent = NULL;
    s_pt_aging_video_retry_set = 0;
	s_pt_aging_receive_label = NULL;
    s_pt_aging_overlay = NULL;
	if (s_pt_screen) {
		divoom_product_test_screen_destroy(s_pt_screen);
		s_pt_screen = NULL;
	}
}

void divoom_product_test_ui_show(void)
{
	divoom_pt_ui_post(PT_UI_SHOW, 0);
}

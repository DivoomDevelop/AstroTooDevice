#include "../storage_stress_test.h"

#if DIVOOM_STORAGE_STRESS_TEST
#include <stdio.h>
#include "lvgl.h"
#include "libs/freetype/lv_freetype.h"

/* Font resource ID 28 is mapped by the font manager to image ID 29. */
#define STORAGE_FONT_PATH "/usr/share/divoom_app/divoom/21/29.bin"

static lv_obj_t *cycle_label, *phase_label, *detail_label, *free_label;
static lv_obj_t *test_label[2], *progress_bar, *status_dot;
static lv_font_t *font_body, *font_large;

static lv_obj_t *make_label(lv_obj_t *parent, int x, int y, const lv_font_t *font,
                            uint32_t color, const char *text)
{
    lv_obj_t *label = lv_label_create(parent);
    if (!label) return NULL;
    lv_obj_set_pos(label, x, y);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    lv_label_set_text(label, text);
    return label;
}

static lv_obj_t *make_card(lv_obj_t *parent, int x)
{
    lv_obj_t *card = lv_obj_create(parent);
    if (!card) return NULL;
    lv_obj_set_pos(card, x, 177);
    lv_obj_set_size(card, 194, 110);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x10202a), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x20d4d4), 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_pad_all(card, 0, 0);
    return card;
}

static void storage_ui_refresh(lv_timer_t *timer)
{
    static const char *const phases[] = {
        "WAITING", "WRITING", "VERIFYING", "DELETING", "CHECKING FAT", "COMPLETE", "STOPPED"
    };
    storage_stress_status s;
    uint32_t phase, completed;
    (void)timer;
    storage_stress_get_status(&s);
    phase = s.phase <= STORAGE_STRESS_ERROR ? s.phase : STORAGE_STRESS_ERROR;
    completed = phase == STORAGE_STRESS_DONE ? STORAGE_STRESS_CYCLES : (s.cycle ? s.cycle - 1U : 0);
    lv_label_set_text_fmt(cycle_label, "%lu / %u", (unsigned long)s.cycle, STORAGE_STRESS_CYCLES);
    lv_bar_set_value(progress_bar, (int32_t)completed, LV_ANIM_OFF);
    lv_label_set_text_fmt(test_label[0], "%lu files", (unsigned long)s.files[0]);
    lv_label_set_text_fmt(test_label[1], "%lu files", (unsigned long)s.files[1]);
    lv_label_set_text(phase_label, phases[phase]);
    lv_obj_set_style_text_color(phase_label,
        lv_color_hex(phase == STORAGE_STRESS_ERROR ? 0xff6868 : 0xa2f43b), 0);
    lv_obj_set_style_bg_color(status_dot,
        lv_color_hex(phase == STORAGE_STRESS_ERROR ? 0xff6868 : 0xa2f43b), 0);
    if (phase == STORAGE_STRESS_ERROR)
        lv_label_set_text_fmt(detail_label, "Error %d  -  see serial log", s.error);
    else if (phase == STORAGE_STRESS_VERIFY || phase == STORAGE_STRESS_DELETE)
        lv_label_set_text_fmt(detail_label, "%lu / %lu files", (unsigned long)s.done, (unsigned long)s.total);
    else
        lv_label_set_text(detail_label, phase == STORAGE_STRESS_DONE ? "All cycles passed" : "TEST + TEST1");
    lv_label_set_text_fmt(free_label, "FREE SPACE   %lu MB", (unsigned long)(s.free_kib / 1024U));
}

/* Called with the LVGL lock held, before any normal app or product test starts. */
int divoom_storage_test_ui_start(void)
{
    lv_obj_t *root = lv_screen_active(), *card0, *card1;
    const lv_font_t *body, *large;
    lv_timer_t *timer;
    if (!root) return 0;
    font_body = lv_freetype_font_create(STORAGE_FONT_PATH, LV_FREETYPE_FONT_RENDER_MODE_BITMAP,
                                         22, LV_FREETYPE_FONT_STYLE_NORMAL);
    font_large = lv_freetype_font_create(STORAGE_FONT_PATH, LV_FREETYPE_FONT_RENDER_MODE_BITMAP,
                                          32, LV_FREETYPE_FONT_STYLE_NORMAL);
    if (!font_body || !font_large)
        printf("[storage_test] font ID 28 missing or cannot load: %s; using built-in fallback\n", STORAGE_FONT_PATH);
    else
        printf("[storage_test] font ID 28 loaded: %s\n", STORAGE_FONT_PATH);
    body = font_body ? font_body : &lv_font_montserrat_16;
    large = font_large ? font_large : &lv_font_montserrat_32;
    lv_obj_clean(root);
    lv_obj_remove_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(root, lv_color_hex(0x030a13), 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    status_dot = lv_obj_create(root);
    if (!status_dot) return 0;
    lv_obj_set_pos(status_dot, 40, 35);
    lv_obj_set_size(status_dot, 12, 12);
    lv_obj_set_style_radius(status_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(status_dot, 0, 0);
    if (!make_label(root, 62, 28, body, 0xffffff, "STORAGE TEST")) return 0;
    cycle_label = make_label(root, 40, 76, large, 0xffffff, "0 / 2000");
    progress_bar = lv_bar_create(root);
    if (!cycle_label || !progress_bar) return 0;
    lv_obj_set_pos(progress_bar, 40, 135);
    lv_obj_set_size(progress_bar, 400, 12);
    lv_bar_set_range(progress_bar, 0, STORAGE_STRESS_CYCLES);
    lv_obj_set_style_bg_color(progress_bar, lv_color_hex(0x1b323a), LV_PART_MAIN);
    lv_obj_set_style_bg_color(progress_bar, lv_color_hex(0x9cec39), LV_PART_INDICATOR);
    card0 = make_card(root, 40);
    card1 = make_card(root, 246);
    if (!card0 || !card1) return 0;
    if (!make_label(card0, 18, 15, body, 0x35e1df, "TEST") ||
        !make_label(card1, 18, 15, body, 0x35e1df, "TEST1")) return 0;
    test_label[0] = make_label(card0, 18, 55, body, 0xffffff, "0 files");
    test_label[1] = make_label(card1, 18, 55, body, 0xffffff, "0 files");
    if (!make_label(root, 40, 320, body, 0x9caeb6, "PHASE") ||
        !test_label[0] || !test_label[1]) return 0;
    phase_label = make_label(root, 154, 320, body, 0xa2f43b, "WAITING");
    detail_label = make_label(root, 40, 360, body, 0xffffff, "TEST + TEST1");
    free_label = make_label(root, 40, 418, body, 0xffffff, "FREE SPACE   -- MB");
    if (!phase_label || !detail_label || !free_label) return 0;
    timer = lv_timer_create(storage_ui_refresh, 250, NULL);
    if (!timer) return 0;
    storage_ui_refresh(timer);
    return 1;
}
#endif

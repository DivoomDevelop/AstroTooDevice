#include <string.h>
#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_astrootoo_theme.h"

#define RESET_WAIT_SPINNER_SZ    80
#define RESET_WAIT_SPINNER_Y     168
#define RESET_WAIT_SPINNER_W     8
#define RESET_WAIT_SPINNER_MS    900
#define RESET_WAIT_SPINNER_ARC   80
#define RESET_WAIT_LABEL_X       40
#define RESET_WAIT_LABEL_W       400
#define RESET_WAIT_LABEL_H       80
#define RESET_WAIT_LABEL_GAP     24

typedef struct _divoom_lvgl_reseting_device_ctrl
{
    int reset_posted;
    void *main_obj;
}divoom_lvgl_reseting_device_ctrl, *Pdivoom_lvgl_reseting_device_ctrl;

static void divoom_lvgl_reseting_device_spinner_anim_cb(void *obj, int32_t v)
{
    lv_arc_set_rotation((lv_obj_t *)obj, v);
}

void *divoom_lvgl_reseting_device_init(void *user_data, int menu_id)
{
    lv_obj_t *spinner;
    lv_obj_t *label_obj;
    Pdivoom_lvgl_reseting_device_ctrl phandle;
    lv_anim_t a;
    int spinner_x;
    int label_y;

    (void)user_data;
    phandle = divoom_system_malloc(sizeof(divoom_lvgl_reseting_device_ctrl));
    memset(phandle, 0, sizeof(divoom_lvgl_reseting_device_ctrl));
    phandle->main_obj = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(phandle->main_obj, lv_color_hex(DIVOOM_ASTRO_BG), LV_PART_MAIN);
    lv_obj_set_style_pad_all(phandle->main_obj, 0, 0);
    lv_obj_set_style_border_width(phandle->main_obj, 0, 0);
    lv_obj_clear_flag(phandle->main_obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(phandle->main_obj, 0, 0);
    lv_obj_set_size(phandle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);

    spinner_x = (DIVOOM_SCREEN_WIDTH - RESET_WAIT_SPINNER_SZ) / 2;
    spinner = lv_arc_create(phandle->main_obj);
    lv_obj_set_pos(spinner, spinner_x, RESET_WAIT_SPINNER_Y);
    lv_obj_set_size(spinner, RESET_WAIT_SPINNER_SZ, RESET_WAIT_SPINNER_SZ);
    lv_arc_set_bg_angles(spinner, 0, 360);
    lv_arc_set_angles(spinner, 0, RESET_WAIT_SPINNER_ARC);
    lv_arc_set_rotation(spinner, 0);
    lv_obj_remove_style(spinner, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(spinner, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_width(spinner, RESET_WAIT_SPINNER_W, LV_PART_MAIN);
    lv_obj_set_style_arc_width(spinner, RESET_WAIT_SPINNER_W, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(spinner, lv_color_hex(DIVOOM_ASTRO_STROKE), LV_PART_MAIN);
    lv_obj_set_style_arc_color(spinner, lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS), LV_PART_INDICATOR);

    lv_anim_init(&a);
    lv_anim_set_var(&a, spinner);
    lv_anim_set_exec_cb(&a, divoom_lvgl_reseting_device_spinner_anim_cb);
    lv_anim_set_values(&a, 0, 360);
    lv_anim_set_time(&a, RESET_WAIT_SPINNER_MS);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&a, lv_anim_path_linear);
    lv_anim_start(&a);

    label_y = RESET_WAIT_SPINNER_Y + RESET_WAIT_SPINNER_SZ + RESET_WAIT_LABEL_GAP;
    label_obj = lv_label_create(phandle->main_obj);
    lv_obj_set_pos(label_obj, RESET_WAIT_LABEL_X, label_y);
    lv_obj_set_size(label_obj, RESET_WAIT_LABEL_W, RESET_WAIT_LABEL_H);
    lv_label_set_long_mode(label_obj, LV_LABEL_LONG_WRAP);
    lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("ResetingDevice"));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(24, menu_id), 0);

    lv_obj_add_flag(phandle->main_obj, LV_OBJ_FLAG_CLICKABLE);

    return phandle;
}

void divoom_lvgl_reseting_device_disp(void *handle, void *active_obj)
{
    Pdivoom_lvgl_reseting_device_ctrl phandle = (Pdivoom_lvgl_reseting_device_ctrl)handle;

    (void)active_obj;
    lv_scr_load(phandle->main_obj);
    lv_refr_now(lv_disp_get_default());
    if (!phandle->reset_posted)
    {
        phandle->reset_posted = 1;
        divoom_print("factory reset: wait page shown, post RESET_FACTORY\n");
        divoom_main_push_model_event_id(DIVOOM_MODEL_MAIN, DIVOOM_MAIN_EVENT_RESET_FACTORY_SETTING);
    }
}

void divoom_lvgl_reseting_device_exit(void *handle)
{
    Pdivoom_lvgl_reseting_device_ctrl phandle = (Pdivoom_lvgl_reseting_device_ctrl)handle;

    lv_obj_del(phandle->main_obj);
    divoom_system_free(phandle);
}

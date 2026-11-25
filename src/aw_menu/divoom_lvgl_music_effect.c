/*
 * LVGL pseudo-3D (slice) effect
 *
 * 方案A（切条）：
 * - 一张图切成 N 条竖条，每条一个 lv_img
 * - 通过 offset_x 只显示对应条带
 * - 对每条做不同 zoom + y 偏移 +（可选）轻微旋转，形成“立方体侧面/翻转”伪3D
 *
 * 说明：
 * - 你们这套工程里 LVGL + WebPEncodeBGRA 的缓冲区实际是 BGRA 字节序（也常被叫 ABGR8888）
 * - 本实现不做像素拷贝/转换，直接引用外部 BGRA/RGBA 缓冲区（调用者保证生命周期）
 */

#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_pixel_format.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    lv_obj_t *cont;
    lv_obj_t **slice;
    uint16_t *slice_base_w; /* 不含重叠的源条宽，用于算 x_acc */
    uint16_t slice_cnt;

    uint16_t src_w;
    uint16_t src_h;
    uint16_t slice_nom_w; /* ceil(src_w / slice_cnt) */

    lv_img_dsc_t img;
    const uint8_t *buf;
    uint8_t *owned_buf;

    /* 3D 增强：阴影强度（0..255），值越大远端越暗 */
    uint8_t shade_max;
} pseudo3d_img_t;

static inline uint16_t u16_min(uint16_t a, uint16_t b) { return a < b ? a : b; }
static inline int32_t i32_clamp(int32_t v, int32_t lo, int32_t hi) { return v < lo ? lo : (v > hi ? hi : v); }

/* 直观参数：用一个“梯形(立方体侧面)”来描述目标形状
 * 约定（更贴近“近大远小”的立体感觉）：
 * - top_len:    远端上边长度（一般更短）
 * - bottom_len: 近端下边长度（一般更长）
 * - left_len:   远端左边长度（一般更短）
 * - right_len:  近端右边长度（一般更长）
 *
 * 这些长度不需要严格满足投影几何，只作为“直观目标尺寸”，内部会做合理映射。
 */
typedef struct {
    lv_coord_t top_len;
    lv_coord_t bottom_len;
    lv_coord_t left_len;
    lv_coord_t right_len;
} pseudo3d_edges_t;

/* 等腰梯形（三参数）：
 * - far_h:  远端“底”(左边)高度
 * - near_h: 近端“底”(右边)高度
 * - span_w: 两底之间的距离（梯形的“高”，沿 X 方向）
 *
 * 说明：这里的“上下底”按你们的使用习惯是左右两条平行边（都是竖边），span_w 是它们之间的距离。
 */
typedef struct {
    lv_coord_t far_h;
    lv_coord_t near_h;
    lv_coord_t span_w;
} pseudo3d_iso_trap3_t;

/* 增加条带重叠，掩盖缩放/取整造成的缝隙 */
#ifndef PSEUDO3D_OVERLAP_PX
#define PSEUDO3D_OVERLAP_PX 3
#endif

/* ========== Beat Creatures 音乐律动透明控件 ========== */
#define DIVOOM_BEAT_CREATURES_LEVEL_NUM       3
#define DIVOOM_BEAT_CREATURES_BAR_NUM         10
#define DIVOOM_BEAT_CREATURES_PARTICLE_NUM    18
#define DIVOOM_BEAT_CREATURES_SPRITE_SIZE     78
#define DIVOOM_BEAT_CREATURES_TIMER_MS        80

typedef enum {
    DIVOOM_BEAT_CREATURES_LEVEL_LOW = 0,
    DIVOOM_BEAT_CREATURES_LEVEL_MID,
    DIVOOM_BEAT_CREATURES_LEVEL_HIGH
} divoom_beat_creatures_level_t;

typedef struct {
    uint8_t low;
    uint8_t mid;
    uint8_t high;
} divoom_lvgl_beat_creatures_energy_t;

typedef struct {
    lv_obj_t *root;
    lv_obj_t *floor_line[DIVOOM_BEAT_CREATURES_LEVEL_NUM];
    lv_obj_t *bar[DIVOOM_BEAT_CREATURES_LEVEL_NUM][DIVOOM_BEAT_CREATURES_BAR_NUM];
    lv_obj_t *hero[DIVOOM_BEAT_CREATURES_LEVEL_NUM];
    lv_obj_t *particle[DIVOOM_BEAT_CREATURES_PARTICLE_NUM];
    lv_timer_t *timer;
    lv_coord_t width;
    lv_coord_t height;
    divoom_lvgl_beat_creatures_energy_t target;
    divoom_lvgl_beat_creatures_energy_t current;
    uint8_t auto_demo;
    uint8_t tick;
    uint8_t sprite_frame[DIVOOM_BEAT_CREATURES_LEVEL_NUM];
} divoom_lvgl_beat_creatures_ctrl_t;

static const char *g_beat_creatures_idle_img[DIVOOM_BEAT_CREATURES_LEVEL_NUM] = {
    "128/beat_creatures/beat_low_idle.png",
    "128/beat_creatures/beat_mid_idle.png",
    "128/beat_creatures/beat_high_idle.png"
};

static const char *g_beat_creatures_jump_img[DIVOOM_BEAT_CREATURES_LEVEL_NUM] = {
    "128/beat_creatures/beat_low_jump.png",
    "128/beat_creatures/beat_mid_jump.png",
    "128/beat_creatures/beat_high_jump.png"
};

static const uint32_t g_beat_creatures_color[DIVOOM_BEAT_CREATURES_LEVEL_NUM] = {
    0xff4f91,
    0xffd600,
    0x25d9f2
};

static const uint8_t g_beat_creatures_amp[DIVOOM_BEAT_CREATURES_LEVEL_NUM] = {
    14,
    24,
    34
};

static void divoom_lvgl_beat_creatures_apply_transparent_obj(lv_obj_t *obj)
{
    if (obj == NULL) {
        return;
    }

    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_outline_width(obj, 0, 0);
    lv_obj_set_style_shadow_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
}

static uint8_t divoom_lvgl_beat_creatures_clamp_energy(uint8_t value)
{
    if (value > 100) {
        return 100;
    }

    return value;
}

static uint8_t divoom_lvgl_beat_creatures_wave(uint8_t tick, uint8_t period, uint8_t min_value, uint8_t max_value)
{
    uint8_t pos;
    uint8_t half;
    uint16_t span;
    uint16_t value;

    if (period < 2) {
        return min_value;
    }

    pos = tick % period;
    half = period / 2;
    span = (uint16_t)(max_value - min_value);

    if (pos < half) {
        value = (uint16_t)min_value + span * pos / half;
    } else {
        value = (uint16_t)min_value + span * (period - pos) / (period - half);
    }

    return (uint8_t)value;
}

static uint8_t divoom_lvgl_beat_creatures_approach_u8(uint8_t current, uint8_t target)
{
    int16_t diff;
    int16_t step;

    if (current == target) {
        return current;
    }

    diff = (int16_t)target - (int16_t)current;
    step = diff / 3;

    if (step == 0) {
        if (diff > 0) {
            step = 1;
        } else {
            step = -1;
        }
    }

    return (uint8_t)((int16_t)current + step);
}

static uint8_t divoom_lvgl_beat_creatures_get_energy(
    const divoom_lvgl_beat_creatures_energy_t *energy, uint8_t level)
{
    if (level == DIVOOM_BEAT_CREATURES_LEVEL_LOW) {
        return energy->low;
    }

    if (level == DIVOOM_BEAT_CREATURES_LEVEL_MID) {
        return energy->mid;
    }

    return energy->high;
}

static void divoom_lvgl_beat_creatures_set_shape_style(lv_obj_t *obj, uint32_t color, lv_opa_t opa, lv_coord_t radius)
{
    if (obj == NULL) {
        return;
    }

    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(obj, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(obj, opa, 0);
    lv_obj_set_style_radius(obj, radius, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_outline_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
}

static void divoom_lvgl_beat_creatures_update_auto_energy(divoom_lvgl_beat_creatures_ctrl_t *ctrl)
{
    if (ctrl == NULL) {
        return;
    }

    if (ctrl->auto_demo == 0) {
        return;
    }

    ctrl->target.low = divoom_lvgl_beat_creatures_wave(ctrl->tick, 28, 16, 86);
    ctrl->target.mid = divoom_lvgl_beat_creatures_wave((uint8_t)(ctrl->tick + 7), 22, 20, 100);
    ctrl->target.high = divoom_lvgl_beat_creatures_wave((uint8_t)(ctrl->tick + 13), 16, 12, 92);
}

static void divoom_lvgl_beat_creatures_update_sprite(
    divoom_lvgl_beat_creatures_ctrl_t *ctrl, uint8_t level, uint8_t energy)
{
    uint8_t period;
    uint8_t frame;

    if ((ctrl == NULL) || (level >= DIVOOM_BEAT_CREATURES_LEVEL_NUM)) {
        return;
    }

    period = (uint8_t)(5 - level);
    frame = 0;

    if (energy > (uint8_t)(38 + level * 10)) {
        if (((ctrl->tick / period) & 0x01) == 0) {
            frame = 1;
        }
    }

    if (ctrl->sprite_frame[level] != frame) {
        ctrl->sprite_frame[level] = frame;
        if (frame != 0) {
            divoom_menu_control_lv_img_set_src(ctrl->hero[level], g_beat_creatures_jump_img[level]);
        } else {
            divoom_menu_control_lv_img_set_src(ctrl->hero[level], g_beat_creatures_idle_img[level]);
        }
    }
}

static void divoom_lvgl_beat_creatures_update_bars(
    divoom_lvgl_beat_creatures_ctrl_t *ctrl, uint8_t level, uint8_t energy,
    lv_coord_t center_x, lv_coord_t floor_y)
{
    lv_coord_t bar_w;
    lv_coord_t gap;
    lv_coord_t total_w;
    uint8_t i;

    if ((ctrl == NULL) || (level >= DIVOOM_BEAT_CREATURES_LEVEL_NUM)) {
        return;
    }

    bar_w = 6;
    gap = 3;
    total_w = DIVOOM_BEAT_CREATURES_BAR_NUM * bar_w + (DIVOOM_BEAT_CREATURES_BAR_NUM - 1) * gap;

    for (i = 0; i < DIVOOM_BEAT_CREATURES_BAR_NUM; i++) {
        uint8_t phase;
        uint8_t pulse;
        lv_coord_t bar_h;
        lv_coord_t x;
        lv_coord_t y;
        lv_opa_t opa;

        phase = (uint8_t)((ctrl->tick * (level + 2) + i * 11 + level * 9) % 32);
        if (phase > 16) {
            pulse = (uint8_t)(32 - phase);
        } else {
            pulse = phase;
        }

        bar_h = (lv_coord_t)(8 + (energy * (pulse + 10)) / 42);
        if (bar_h > ctrl->height - DIVOOM_BEAT_CREATURES_SPRITE_SIZE - 16) {
            bar_h = ctrl->height - DIVOOM_BEAT_CREATURES_SPRITE_SIZE - 16;
        }

        if (bar_h < 4) {
            bar_h = 4;
        }

        x = center_x - total_w / 2 + i * (bar_w + gap);
        y = floor_y - DIVOOM_BEAT_CREATURES_SPRITE_SIZE - bar_h / 2;
        opa = (lv_opa_t)(50 + energy * 150 / 100);

        lv_obj_set_size(ctrl->bar[level][i], bar_w, bar_h);
        lv_obj_set_pos(ctrl->bar[level][i], x, y);
        lv_obj_set_style_bg_opa(ctrl->bar[level][i], opa, 0);
    }
}

static void divoom_lvgl_beat_creatures_update_particles(
    divoom_lvgl_beat_creatures_ctrl_t *ctrl, uint8_t level, uint8_t energy,
    lv_coord_t center_x, lv_coord_t floor_y)
{
    static const int8_t x_offset[6] = { -42, -24, -9, 13, 29, 43 };
    uint8_t i;

    if ((ctrl == NULL) || (level >= DIVOOM_BEAT_CREATURES_LEVEL_NUM)) {
        return;
    }

    for (i = 0; i < 6; i++) {
        uint8_t index;
        uint8_t drift;
        uint8_t travel;
        lv_coord_t size;
        lv_coord_t x;
        lv_coord_t y;
        lv_opa_t opa;

        index = (uint8_t)(level * 6 + i);
        drift = (uint8_t)((ctrl->tick * (level + 2) + i * 7) % 9);
        travel = (uint8_t)((ctrl->tick * (level + 3) + i * 19) % (46 + g_beat_creatures_amp[level]));
        size = (lv_coord_t)(2 + energy / 45);
        x = center_x + x_offset[i] + drift - 4;
        y = floor_y - 18 - travel - energy / 4;
        opa = (lv_opa_t)(30 + energy * 150 / 100);

        lv_obj_set_size(ctrl->particle[index], size, size);
        lv_obj_set_pos(ctrl->particle[index], x, y);
        lv_obj_set_style_bg_opa(ctrl->particle[index], opa, 0);
    }
}

static void divoom_lvgl_beat_creatures_refresh(divoom_lvgl_beat_creatures_ctrl_t *ctrl)
{
    lv_coord_t lane_w;
    lv_coord_t floor_y;
    uint8_t level;

    if ((ctrl == NULL) || (ctrl->root == NULL)) {
        return;
    }

    lane_w = ctrl->width / DIVOOM_BEAT_CREATURES_LEVEL_NUM;
    floor_y = ctrl->height - 8;

    for (level = 0; level < DIVOOM_BEAT_CREATURES_LEVEL_NUM; level++) {
        uint8_t energy;
        lv_coord_t center_x;
        lv_coord_t jump_y;
        lv_coord_t sway_x;
        lv_coord_t line_x;
        lv_coord_t line_w;
        uint16_t zoom;
        lv_opa_t line_opa;

        energy = divoom_lvgl_beat_creatures_get_energy(&ctrl->current, level);
        center_x = lane_w * level + lane_w / 2;
        jump_y = (lv_coord_t)(energy * g_beat_creatures_amp[level] / 100);
        sway_x = (lv_coord_t)(((ctrl->tick + level * 5) % 7) - 3);
        zoom = (uint16_t)(238 + energy * (30 + level * 8) / 100);
        line_x = lane_w * level + 12;
        line_w = lane_w - 24;
        line_opa = (lv_opa_t)(90 + energy * 120 / 100);

        if (line_w < 20) {
            line_w = 20;
        }

        divoom_lvgl_beat_creatures_update_sprite(ctrl, level, energy);
        divoom_lvgl_beat_creatures_update_bars(ctrl, level, energy, center_x, floor_y);

        lv_obj_set_size(ctrl->floor_line[level], line_w, 4);
        lv_obj_set_pos(ctrl->floor_line[level], line_x, floor_y);
        lv_obj_set_style_bg_opa(ctrl->floor_line[level], line_opa, 0);
        lv_obj_set_style_shadow_color(ctrl->floor_line[level], lv_color_hex(g_beat_creatures_color[level]), 0);
        lv_obj_set_style_shadow_width(ctrl->floor_line[level], (lv_coord_t)(7 + energy / 8), 0);
        lv_obj_set_style_shadow_opa(ctrl->floor_line[level], (lv_opa_t)(60 + energy), 0);

        lv_img_set_zoom(ctrl->hero[level], zoom);
        lv_obj_set_pos(ctrl->hero[level],
            center_x - DIVOOM_BEAT_CREATURES_SPRITE_SIZE / 2 + sway_x,
            floor_y - DIVOOM_BEAT_CREATURES_SPRITE_SIZE - jump_y);

        divoom_lvgl_beat_creatures_update_particles(ctrl, level, energy, center_x, floor_y);
    }
}

static void divoom_lvgl_beat_creatures_timer_cb(lv_timer_t *timer)
{
    divoom_lvgl_beat_creatures_ctrl_t *ctrl;

    if (timer == NULL) {
        return;
    }

    ctrl = (divoom_lvgl_beat_creatures_ctrl_t *)timer->user_data;
    if (ctrl == NULL) {
        return;
    }

    ctrl->tick++;
    divoom_lvgl_beat_creatures_update_auto_energy(ctrl);
    ctrl->current.low = divoom_lvgl_beat_creatures_approach_u8(ctrl->current.low, ctrl->target.low);
    ctrl->current.mid = divoom_lvgl_beat_creatures_approach_u8(ctrl->current.mid, ctrl->target.mid);
    ctrl->current.high = divoom_lvgl_beat_creatures_approach_u8(ctrl->current.high, ctrl->target.high);
    divoom_lvgl_beat_creatures_refresh(ctrl);
}

static void divoom_lvgl_beat_creatures_delete_event_cb(lv_event_t *event)
{
    divoom_lvgl_beat_creatures_ctrl_t *ctrl;

    if (event == NULL) {
        return;
    }

    ctrl = (divoom_lvgl_beat_creatures_ctrl_t *)lv_event_get_user_data(event);
    if (ctrl == NULL) {
        return;
    }

    if (ctrl->timer != NULL) {
        lv_timer_del(ctrl->timer);
        ctrl->timer = NULL;
    }

    ctrl->root = NULL;
    divoom_system_free(ctrl);
}

void *divoom_lvgl_beat_creatures_create(lv_obj_t *parent, lv_coord_t x, lv_coord_t y,
    lv_coord_t w, lv_coord_t h)
{
    divoom_lvgl_beat_creatures_ctrl_t *ctrl;
    uint8_t level;
    uint8_t i;

    if ((parent == NULL) || (w <= 0) || (h <= 0)) {
        return NULL;
    }

    ctrl = (divoom_lvgl_beat_creatures_ctrl_t *)divoom_system_malloc(sizeof(divoom_lvgl_beat_creatures_ctrl_t));
    if (ctrl == NULL) {
        return NULL;
    }

    memset(ctrl, 0, sizeof(divoom_lvgl_beat_creatures_ctrl_t));
    ctrl->width = w;
    ctrl->height = h;
    ctrl->auto_demo = 1;
    ctrl->target.low = 35;
    ctrl->target.mid = 58;
    ctrl->target.high = 46;

    /* 根对象只负责承载动画层，保持透明底，便于叠在表盘背景或音乐页面上。 */
    ctrl->root = lv_obj_create(parent);
    if (ctrl->root == NULL) {
        divoom_system_free(ctrl);
        return NULL;
    }

    lv_obj_set_pos(ctrl->root, x, y);
    lv_obj_set_size(ctrl->root, w, h);
    divoom_lvgl_beat_creatures_apply_transparent_obj(ctrl->root);
    lv_obj_add_event_cb(ctrl->root, divoom_lvgl_beat_creatures_delete_event_cb, LV_EVENT_DELETE, ctrl);

    for (level = 0; level < DIVOOM_BEAT_CREATURES_LEVEL_NUM; level++) {
        ctrl->floor_line[level] = lv_obj_create(ctrl->root);
        divoom_lvgl_beat_creatures_set_shape_style(ctrl->floor_line[level],
            g_beat_creatures_color[level], LV_OPA_COVER, 2);

        for (i = 0; i < DIVOOM_BEAT_CREATURES_BAR_NUM; i++) {
            ctrl->bar[level][i] = lv_obj_create(ctrl->root);
            divoom_lvgl_beat_creatures_set_shape_style(ctrl->bar[level][i],
                g_beat_creatures_color[level], LV_OPA_70, 2);
        }
    }

    for (i = 0; i < DIVOOM_BEAT_CREATURES_PARTICLE_NUM; i++) {
        uint8_t level_index;

        level_index = i / 6;
        ctrl->particle[i] = lv_obj_create(ctrl->root);
        divoom_lvgl_beat_creatures_set_shape_style(ctrl->particle[i],
            g_beat_creatures_color[level_index], LV_OPA_60, LV_RADIUS_CIRCLE);
    }

    for (level = 0; level < DIVOOM_BEAT_CREATURES_LEVEL_NUM; level++) {
        ctrl->hero[level] = lv_img_create(ctrl->root);
        ctrl->sprite_frame[level] = 0;
        divoom_menu_control_lv_img_set_src(ctrl->hero[level], g_beat_creatures_idle_img[level]);
        lv_img_set_pivot(ctrl->hero[level],
            DIVOOM_BEAT_CREATURES_SPRITE_SIZE / 2, DIVOOM_BEAT_CREATURES_SPRITE_SIZE);
    }

    ctrl->timer = lv_timer_create(divoom_lvgl_beat_creatures_timer_cb,
        DIVOOM_BEAT_CREATURES_TIMER_MS, ctrl);
    divoom_lvgl_beat_creatures_refresh(ctrl);

    return ctrl;
}

lv_obj_t *divoom_lvgl_beat_creatures_get_obj(void *handle)
{
    divoom_lvgl_beat_creatures_ctrl_t *ctrl;

    ctrl = (divoom_lvgl_beat_creatures_ctrl_t *)handle;
    if (ctrl == NULL) {
        return NULL;
    }

    return ctrl->root;
}

void divoom_lvgl_beat_creatures_set_energy(void *handle,
    const divoom_lvgl_beat_creatures_energy_t *energy)
{
    divoom_lvgl_beat_creatures_ctrl_t *ctrl;

    ctrl = (divoom_lvgl_beat_creatures_ctrl_t *)handle;
    if ((ctrl == NULL) || (energy == NULL)) {
        return;
    }

    ctrl->target.low = divoom_lvgl_beat_creatures_clamp_energy(energy->low);
    ctrl->target.mid = divoom_lvgl_beat_creatures_clamp_energy(energy->mid);
    ctrl->target.high = divoom_lvgl_beat_creatures_clamp_energy(energy->high);

    if (ctrl->timer != NULL) {
        lv_timer_ready(ctrl->timer);
    }
}

void divoom_lvgl_beat_creatures_set_auto_demo(void *handle, uint8_t enable)
{
    divoom_lvgl_beat_creatures_ctrl_t *ctrl;

    ctrl = (divoom_lvgl_beat_creatures_ctrl_t *)handle;
    if (ctrl == NULL) {
        return;
    }

    if (enable != 0) {
        ctrl->auto_demo = 1;
    } else {
        ctrl->auto_demo = 0;
    }
}

void divoom_lvgl_beat_creatures_delete(void *handle)
{
    divoom_lvgl_beat_creatures_ctrl_t *ctrl;

    ctrl = (divoom_lvgl_beat_creatures_ctrl_t *)handle;
    if (ctrl == NULL) {
        return;
    }

    if (ctrl->root != NULL) {
        lv_obj_del(ctrl->root);
    } else {
        if (ctrl->timer != NULL) {
            lv_timer_del(ctrl->timer);
            ctrl->timer = NULL;
        }

        divoom_system_free(ctrl);
    }
}

static pseudo3d_img_t *pseudo3d_img_create_common(lv_obj_t *parent,
                                                  const uint8_t *buf,
                                                  uint16_t w, uint16_t h,
                                                  uint16_t slice_cnt)
{
    if (!parent || !buf || w == 0 || h == 0 || slice_cnt < 4) return NULL;

    pseudo3d_img_t *p = (pseudo3d_img_t *)calloc(1, sizeof(*p));
    if (!p) return NULL;

    p->buf = buf;
    p->src_w = w;
    p->src_h = h;
    p->slice_cnt = slice_cnt;
    p->slice_nom_w = (uint16_t)((w + slice_cnt - 1) / slice_cnt);
    p->shade_max = 0;

    p->slice = (lv_obj_t **)calloc(slice_cnt, sizeof(lv_obj_t *));
    p->slice_base_w = (uint16_t *)calloc(slice_cnt, sizeof(uint16_t));
    if (!p->slice || !p->slice_base_w) {
        free(p->slice);
        free(p->slice_base_w);
        free(p);
        return NULL;
    }

    memset(&p->img, 0, sizeof(p->img));
DIVOOM_LV_IMG_HEADER_INIT(&    p->img.header);
    p->img.header.w = w;
    p->img.header.h = h;
    p->img.header.cf = LV_COLOR_FORMAT_RGB565A8;
    p->img.data = buf;
    p->img.data_size = divoom_pixel_format_size(w, h, LV_COLOR_FORMAT_RGB565A8);

    p->cont = lv_obj_create(parent);
    lv_obj_clear_flag(p->cont, LV_OBJ_FLAG_SCROLLABLE);
    /* LVGL8：默认裁剪子对象；确保不要允许溢出绘制 */
    lv_obj_clear_flag(p->cont, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_set_style_bg_opa(p->cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(p->cont, 0, 0);
    lv_obj_set_style_pad_all(p->cont, 0, 0);
    lv_obj_set_size(p->cont, w, h);

    /* 创建切片 */
    for (uint16_t i = 0; i < slice_cnt; i++) {
        uint32_t start_x = (uint32_t)i * (uint32_t)p->slice_nom_w;
        uint16_t base_w = 1;
        if (start_x < (uint32_t)p->src_w) {
            uint32_t remain = (uint32_t)p->src_w - start_x;
            base_w = (uint16_t)u16_min((uint16_t)remain, p->slice_nom_w);
        }
        p->slice_base_w[i] = base_w;

        /* 右侧可用的额外像素，用于重叠显示（最后一条可能为 0） */
        uint16_t overlap_right = 0;
        if (start_x + base_w < p->src_w) {
            uint16_t avail = (uint16_t)(p->src_w - (uint16_t)start_x - base_w);
            overlap_right = u16_min((uint16_t)PSEUDO3D_OVERLAP_PX, avail);
        }

        lv_obj_t *img = lv_img_create(p->cont);
        lv_img_set_src(img, &p->img);

        /* 增加重叠宽度：掩盖缩放/取整缝隙（显示内容向右多取 overlap_right 像素） */
        lv_coord_t obj_w = (lv_coord_t)base_w + (lv_coord_t)overlap_right;
        if (obj_w < 1) obj_w = 1;
        lv_obj_set_size(img, obj_w, h);

        lv_img_set_offset_x(img, -(lv_coord_t)start_x);
        lv_img_set_offset_y(img, 0);

        lv_obj_set_style_transform_pivot_x(img, obj_w / 2, 0);
        lv_obj_set_style_transform_pivot_y(img, h / 2, 0);

        p->slice[i] = img;
    }

    return p;
}

/* 外部接口：输入 BGRA8888，控件内部转换并持有 RGB565A8 图像数据。 */
void *pseudo3d_img_create_from_bgra(lv_obj_t *parent,
                                    const uint8_t *bgra,
                                    uint16_t w, uint16_t h,
                                    uint16_t slice_cnt)
{
    uint8_t *lv_buf;
    pseudo3d_img_t *p;

    if (!bgra || w == 0 || h == 0) return NULL;

    lv_buf = (uint8_t *)divoom_system_malloc(divoom_pixel_format_size(w, h, LV_COLOR_FORMAT_RGB565A8));
    if (!lv_buf) return NULL;

    divoom_bgra8888_to_rgb565a8_buf(bgra, lv_buf, (uint32_t)w * (uint32_t)h);
    p = pseudo3d_img_create_common(parent, lv_buf, w, h, slice_cnt);
    if (!p) {
        divoom_system_free(lv_buf);
        return NULL;
    }
    p->owned_buf = lv_buf;
    return (void *)p;
}

void *pseudo3d_img_create_from_rgb565a8(lv_obj_t *parent,
                                        const uint8_t *rgb565a8,
                                        uint16_t w, uint16_t h,
                                        uint16_t slice_cnt)
{
    uint8_t *lv_buf;
    pseudo3d_img_t *p;
    uint32_t size;

    if (!rgb565a8 || w == 0 || h == 0) return NULL;

    size = divoom_pixel_format_size(w, h, LV_COLOR_FORMAT_RGB565A8);
    lv_buf = (uint8_t *)divoom_system_malloc(size);
    if (!lv_buf) return NULL;

    memcpy(lv_buf, rgb565a8, size);
    p = pseudo3d_img_create_common(parent, lv_buf, w, h, slice_cnt);
    if (!p) {
        divoom_system_free(lv_buf);
        return NULL;
    }
    p->owned_buf = lv_buf;
    return (void *)p;
}

/* 设置伪3D的光照强度（0..255）
 * - 0: 不做明暗渐变（更“平”）
 * - 80~140: 有明显立体感但不脏
 * - >160: 立体感强但容易显得“黑”
 */
void pseudo3d_img_set_shading(void *handle, uint8_t shade_max)
{
    pseudo3d_img_t *p = (pseudo3d_img_t *)handle;
    if (!p) return;
    p->shade_max = shade_max;
}

void pseudo3d_img_delete(void *handle)
{
    pseudo3d_img_t *p = (pseudo3d_img_t *)handle;
    if (!p) return;
    if (p->cont) lv_obj_del(p->cont);
    free(p->slice);
    free(p->slice_base_w);
    if (p->owned_buf) divoom_system_free(p->owned_buf);
    free(p);
}

/* 将直观的四边长度映射到内部参数：
 * - near_zoom/far_zoom：由 bottom/top & right/left 两组约束共同决定（取更“保守”的缩放）
 * - shear_px：由近/远边高度差推导出一个默认倾斜（让上边更“靠后”）
 * - rot_tilt：默认关闭（旋转会放大锯齿/块感），如需更强立体可另传
 */
static void pseudo3d_edges_to_params(const pseudo3d_img_t *p,
                                     const pseudo3d_edges_t *e,
                                     int32_t *out_far_zoom,
                                     int32_t *out_near_zoom,
                                     lv_coord_t *out_shear_px)
{
    /* 防御：避免 0 */
    lv_coord_t topw = e->top_len > 1 ? e->top_len : 1;
    lv_coord_t botw = e->bottom_len > 1 ? e->bottom_len : 1;
    lv_coord_t lefth = e->left_len > 1 ? e->left_len : 1;
    lv_coord_t righth = e->right_len > 1 ? e->right_len : 1;

    /* 更直观的映射：
     * - top/bottom 直接决定“远端/近端的横向压缩比例”(far/near zoom)
     * - left/right 只用于推导倾斜(shear)，不再反向影响 zoom（否则用户填的 top/bottom 不生效）
     *
     * 说明：由于 LVGL 的 transform_zoom 是等比缩放，四边长无法严格同时满足，这是近似方案。 */
    int32_t near_zoom = (int32_t)botw * 256 / (int32_t)p->src_w;
    int32_t far_zoom  = (int32_t)topw * 256 / (int32_t)p->src_w;

    near_zoom = i32_clamp(near_zoom, 32, 512);
    far_zoom  = i32_clamp(far_zoom,  16, near_zoom);

    /* 倾斜：用左右“高度差”推一个 shear（只是视觉增强，不保证边长严格等于 left/right） */
    lv_coord_t shear = (righth - lefth) / 4;
    shear = (lv_coord_t)i32_clamp(shear, -80, 80);

    *out_far_zoom = far_zoom;
    *out_near_zoom = near_zoom;
    *out_shear_px = shear;
}

/* 仅用于等腰梯形模式：在目标高度 target_h 内居中放置每条切片。
 * 当 target_w 远小于自然宽度时（侧面窄条），保持高度不变，只压缩宽度：zoom 仅用于高度，宽度用裁剪+偏移实现。 */
static void pseudo3d_layout_center_h(pseudo3d_img_t *p,
                                     lv_coord_t face_x, lv_coord_t face_y,
                                     int32_t far_zoom, int32_t near_zoom,
                                     lv_coord_t target_w,
                                     lv_coord_t target_h,
                                     uint8_t shade_max)
{
    if (!p) return;
    if (target_w < 1) target_w = 1;
    if (target_h < 1) target_h = 1;
    p->shade_max = shade_max;

    lv_obj_add_flag(p->cont, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_set_pos(p->cont, face_x, face_y);

    /* 与 set_iso_trap3 迭代一致：用 smooth u 计算 total_w_px 与每条 zoom，保证保高且受 far/near 控制 */
    int32_t total_w_px = 0;
    int32_t *disp_w_arr = (int32_t *)malloc(sizeof(int32_t) * p->slice_cnt);
    if (!disp_w_arr) return;

    for (uint16_t i = 0; i < p->slice_cnt; i++) {
        float u = (float)i / (float)(p->slice_cnt - 1);
        float us = u * u * (3.0f - 2.0f * u);
        int32_t zoom = (int32_t)(far_zoom + (near_zoom - far_zoom) * us);
        if (zoom < 16) zoom = 16;
        int32_t base_w = (int32_t)p->slice_base_w[i];
        int32_t disp_w = (base_w * zoom + 255) / 256;
        if (disp_w < 1) disp_w = 1;
        disp_w_arr[i] = disp_w;
        total_w_px += disp_w;
    }
    if (total_w_px < 1) total_w_px = 1;

    int32_t zoom_scale_q16 = (int32_t)(((int64_t)target_w << 16) / (int64_t)total_w_px);
    zoom_scale_q16 = i32_clamp(zoom_scale_q16, (int32_t)(0.25f * 65536), (int32_t)(2.0f * 65536));
    int32_t zoom_max_h = (int32_t)((int64_t)target_h * 256 / (int64_t)p->src_h);
    if (zoom_max_h < 16) zoom_max_h = 16;

    lv_coord_t x_acc = 0;
    for (uint16_t i = 0; i < p->slice_cnt; i++) {
        float u = (float)i / (float)(p->slice_cnt - 1);
        float us = u * u * (3.0f - 2.0f * u);
        int32_t zoom = (int32_t)(far_zoom + (near_zoom - far_zoom) * us);
        if (zoom < 16) zoom = 16;

        int32_t zoom2 = (int32_t)(((int64_t)zoom * zoom_scale_q16 + 32768) >> 16);
        zoom2 = i32_clamp(zoom2, 16, 512);
        if (zoom2 > zoom_max_h) zoom2 = zoom_max_h;

        lv_obj_t *img = p->slice[i];
        lv_obj_set_style_transform_zoom(img, (int16_t)zoom2, 0);
        lv_obj_set_style_transform_angle(img, 0, 0);

        if (p->shade_max) {
            lv_obj_set_style_img_recolor(img, lv_color_black(), 0);
            uint8_t shade = (uint8_t)(((int32_t)p->shade_max * (int32_t)(1.0f - us)));
            lv_obj_set_style_img_recolor_opa(img, shade, 0);
        } else {
            lv_obj_set_style_img_recolor_opa(img, 0, 0);
        }

        lv_coord_t disp_h = (lv_coord_t)((int32_t)p->src_h * zoom2 / 256);
        lv_coord_t y_center_fix = (target_h - disp_h) / 2;
        if (y_center_fix < 0) y_center_fix = 0;

        int32_t base_w = (int32_t)p->slice_base_w[i];
        lv_coord_t full_disp_w = (lv_coord_t)((base_w * zoom2 + 255) / 256);
        if (full_disp_w < 1) full_disp_w = 1;

        lv_obj_set_pos(img, x_acc, y_center_fix);
        x_acc += full_disp_w;
    }

    if (x_acc < 1) x_acc = 1;
    lv_obj_set_size(p->cont, x_acc, target_h);
    free(disp_w_arr);
}

/* 核心布局：给定 far/near zoom 与 shear，布局切条
 * - target_w>0 时，强制把最终宽度映射到 target_w（让 bottom_len 更“所见即所得”）
 */
static void pseudo3d_layout(pseudo3d_img_t *p,
                            lv_coord_t face_x, lv_coord_t face_y,
                            int32_t far_zoom, int32_t near_zoom,
                            lv_coord_t shear_px,
                            float rot_tilt_deg,
                            lv_coord_t target_w)
{
    lv_obj_set_pos(p->cont, face_x, face_y);

    /* 两遍：
     * 1) 先按 far/near zoom 计算“自然宽度”
     * 2) 如果 target_w>0，则用 target_w 推一个 zoom_scale，对每条 zoom 做等比缩放
     *
     * 关键：不要对 x 做线性映射（会把条带拉开产生白线），应通过等比缩放 zoom 来匹配目标宽度 */
    int32_t total_w_px = 0;
    int32_t *disp_w_arr = (int32_t *)malloc(sizeof(int32_t) * p->slice_cnt);
    if (!disp_w_arr) return;

    for (uint16_t i = 0; i < p->slice_cnt; i++) {
        float u = (float)i / (float)(p->slice_cnt - 1);
        float us = u * u * (3.0f - 2.0f * u);
        int32_t zoom = (int32_t)(far_zoom + (near_zoom - far_zoom) * us);
        if (zoom < 16) zoom = 16;

        int32_t base_w = (int32_t)p->slice_base_w[i];
        int32_t disp_w = (base_w * zoom + 255) / 256;
        if (disp_w < 1) disp_w = 1;
        disp_w_arr[i] = disp_w;
        total_w_px += disp_w;
    }

    if (total_w_px < 1) total_w_px = 1;

    /* zoom_scale_q16：用于把自然宽度缩放到目标宽度 */
    int32_t zoom_scale_q16 = (1 << 16);
    if (target_w > 0) {
        zoom_scale_q16 = (int32_t)(((int64_t)target_w << 16) / (int64_t)total_w_px);
        /* 限制缩放范围，避免异常值 */
        zoom_scale_q16 = i32_clamp(zoom_scale_q16, (int32_t)(0.25f * 65536), (int32_t)(2.0f * 65536));
    }

    lv_coord_t x_acc = 0;
    for (uint16_t i = 0; i < p->slice_cnt; i++) {
        float u = (float)i / (float)(p->slice_cnt - 1);
        float us = u * u * (3.0f - 2.0f * u);
        int32_t zoom = (int32_t)(far_zoom + (near_zoom - far_zoom) * us);
        if (zoom < 16) zoom = 16;

        lv_coord_t y_off = (lv_coord_t)((1.0f - u) * (float)shear_px);
        int16_t ang = (int16_t)(rot_tilt_deg * 10.0f * (1.0f - u));

        /* 按目标宽度等比缩放 zoom（避免拉开条带产生白线） */
        int32_t zoom2 = (int32_t)(((int64_t)zoom * zoom_scale_q16 + 32768) >> 16);
        zoom2 = i32_clamp(zoom2, 16, 512);

        lv_obj_t *img = p->slice[i];
        lv_obj_set_style_transform_zoom(img, (int16_t)zoom2, 0);
        lv_obj_set_style_transform_angle(img, ang, 0);

        if (p->shade_max) {
            lv_obj_set_style_img_recolor(img, lv_color_black(), 0);
            uint8_t shade = (uint8_t)(((int32_t)p->shade_max * (int32_t)(1.0f - u)));
            lv_obj_set_style_img_recolor_opa(img, shade, 0);
        } else {
            lv_obj_set_style_img_recolor_opa(img, 0, 0);
        }

        lv_coord_t disp_h = (lv_coord_t)((int32_t)p->src_h * zoom2 / 256);
        lv_coord_t y_center_fix = (p->src_h - disp_h) / 2;
        lv_obj_set_pos(img, x_acc, y_off + y_center_fix);

        /* 重新按 zoom2 计算本条的显示宽度用于累计（ceil，避免缝） */
        int32_t base_w = (int32_t)p->slice_base_w[i];
        lv_coord_t disp_w2 = (lv_coord_t)((base_w * zoom2 + 255) / 256);
        if (disp_w2 < 1) disp_w2 = 1;
        x_acc += disp_w2;
    }

    if (x_acc < 1) x_acc = 1;
    lv_obj_set_size(p->cont, x_acc, p->src_h + LV_ABS(shear_px));
    free(disp_w_arr);
}

void pseudo3d_img_set(void *handle,
                      lv_coord_t face_x, lv_coord_t face_y,
                      float yaw_deg,
                      lv_coord_t shear_px,
                      float rot_tilt_deg)
{
    pseudo3d_img_t *p = (pseudo3d_img_t *)handle;
    if (!p) return;

    int32_t near_zoom = 256;
    int32_t far_zoom  = 256 - (int32_t)(yaw_deg * 1.4f);
    far_zoom = i32_clamp(far_zoom, 96, near_zoom);
    pseudo3d_layout(p, face_x, face_y, far_zoom, near_zoom, shear_px, rot_tilt_deg, 0);
}

/* 新接口：用四边长度“直观设置”一张图的立方体侧面效果（不做旋转，靠透视+光照增强）
 * - x/y：放置位置（容器左上角）
 * - edges：目标四边长度（上/下/左/右）
 * - shade_max：阴影强度 0..255（建议 100~140）
 *
 * 注意：
 * - 这是一个“更直观的包装层”，内部仍用切条的 2.5D 近似
 * - top/bottom/left/right 不需要严格满足几何一致性，内部会做合理映射
 */
void pseudo3d_img_set_edges(void *handle,
                            lv_coord_t x, lv_coord_t y,
                            const pseudo3d_edges_t *edges,
                            uint8_t shade_max)
{
    pseudo3d_img_t *p = (pseudo3d_img_t *)handle;
    if (!p || !edges) return;

    p->shade_max = shade_max;

    int32_t far_zoom, near_zoom;
    lv_coord_t shear_px;
    pseudo3d_edges_to_params(p, edges, &far_zoom, &near_zoom, &shear_px);
    /* 直接用 bottom_len 作为目标显示宽度，保证“参数更直观” */
    lv_coord_t target_w = edges->bottom_len;
    pseudo3d_layout(p, x, y, far_zoom, near_zoom, shear_px, 0.0f, target_w);
}

/* 新接口：等腰梯形（三参数）设置/动画
 * - far_h/near_h：左右两条平行竖边高度
 * - span_w：两竖边之间距离（最终宽度）
 *
 * 注：由于 LVGL 的 zoom 是等比缩放，严格同时满足(宽度=span_w 且 左右高度精确)不可行。
 * 本接口优先保证“最终宽度=span_w”，并尽量逼近左右高度（两次迭代修正）。
 */
void pseudo3d_img_set_iso_trap3(void *handle,
                                lv_coord_t x, lv_coord_t y,
                                lv_coord_t far_h, lv_coord_t near_h, lv_coord_t span_w,
                                uint8_t shade_max)
{
    pseudo3d_img_t *p = (pseudo3d_img_t *)handle;
    if (!p || !p->slice_base_w || p->slice_cnt < 2) return;
    if (p->src_h < 1) return;

    if (far_h < 1) far_h = 1;
    if (near_h < 1) near_h = 1;
    if (span_w < 1) span_w = 1;

    lv_coord_t target_h = far_h > near_h ? far_h : near_h;

    /* 初始 zoom：按高度拟合 */
    int32_t far_zoom = (int32_t)far_h * 256 / (int32_t)p->src_h;
    int32_t near_zoom = (int32_t)near_h * 256 / (int32_t)p->src_h;
    far_zoom = i32_clamp(far_zoom, 16, 512);
    near_zoom = i32_clamp(near_zoom, 16, 512);

    /* 两次迭代：考虑 width=span_w 时的全局 zoom_scale 会影响高度 */
    for (int it = 0; it < 2; it++) {
        /* 用当前 far/near zoom 估算自然宽度 */
        int32_t total_w_px = 0;
        for (uint16_t i = 0; i < p->slice_cnt; i++) {
            float u = (float)i / (float)(p->slice_cnt - 1);
            float us = u * u * (3.0f - 2.0f * u);
            int32_t zoom = (int32_t)(far_zoom + (near_zoom - far_zoom) * us);
            if (zoom < 16) zoom = 16;
            int32_t base_w = (int32_t)p->slice_base_w[i];
            int32_t dw = (base_w * zoom + 255) / 256;
            if (dw < 1) dw = 1;
            total_w_px += dw;
        }
        if (total_w_px < 1) total_w_px = 1;
        int32_t zoom_scale_q16 = (int32_t)(((int64_t)span_w << 16) / (int64_t)total_w_px);
        zoom_scale_q16 = i32_clamp(zoom_scale_q16, (int32_t)(0.25f * 65536), (int32_t)(2.0f * 65536));

        /* 反推：为了抵消 zoom_scale，让最终高度更接近 far_h/near_h */
        int32_t inv_q16 = (int32_t)(((int64_t)65536 << 16) / (int64_t)zoom_scale_q16);
        far_zoom = (int32_t)(((int64_t)far_zoom * inv_q16 + 32768) >> 16);
        near_zoom = (int32_t)(((int64_t)near_zoom * inv_q16 + 32768) >> 16);
        far_zoom = i32_clamp(far_zoom, 16, 512);
        near_zoom = i32_clamp(near_zoom, 16, 512);
    }

    pseudo3d_layout_center_h(p, x, y, far_zoom, near_zoom, span_w, target_h, shade_max);
}

typedef struct {
    pseudo3d_img_t *p;
    lv_coord_t x, y;
    pseudo3d_iso_trap3_t to;
    uint8_t shade_max;
} pseudo3d_iso_trap3_anim_ctx_t;

static void pseudo3d_iso_trap3_anim_exec(void *var, int32_t v)
{
    pseudo3d_iso_trap3_anim_ctx_t *c = (pseudo3d_iso_trap3_anim_ctx_t *)var; /* 0..1000 */
    pseudo3d_img_t *p = c->p;
    if (!p) return;

    /* from：矩形（左右高度=src_h，宽度=src_w） */
    lv_coord_t far0 = p->src_h;
    lv_coord_t near0 = p->src_h;
    lv_coord_t span0 = p->src_w;

    int32_t far_delta = ((int32_t)(c->to.far_h - far0) * v) / 1000;
    int32_t near_delta = ((int32_t)(c->to.near_h - near0) * v) / 1000;
    int32_t span_delta = ((int32_t)(c->to.span_w - span0) * v) / 1000;
    lv_coord_t far_h = (lv_coord_t)((int32_t)far0 + far_delta);
    lv_coord_t near_h = (lv_coord_t)((int32_t)near0 + near_delta);
    lv_coord_t span_w = (lv_coord_t)((int32_t)span0 + span_delta);

    pseudo3d_img_set_iso_trap3(p, c->x, c->y, far_h, near_h, span_w, c->shade_max);
}

/* 当前正在运行的 iso_trap3 动画 context，用于在删除 handle 前停止动画，避免 use-after-free */
static pseudo3d_iso_trap3_anim_ctx_t *s_iso_trap3_anim_ctx = NULL;

static void pseudo3d_iso_trap3_anim_ready(lv_anim_t *a)
{
    pseudo3d_iso_trap3_anim_ctx_t *c = (pseudo3d_iso_trap3_anim_ctx_t *)a->user_data;
    if (s_iso_trap3_anim_ctx == c) s_iso_trap3_anim_ctx = NULL;
    free(c);
}

/** 停止正在作用于给定 handle 的 iso_trap3 动画（新一轮旋转前必须先调，再 delete handle） */
void pseudo3d_img_anim_iso_trap3_stop(void *handle)
{
    if (!handle) return;
    if (s_iso_trap3_anim_ctx && s_iso_trap3_anim_ctx->p == (pseudo3d_img_t *)handle) {
        lv_anim_del(s_iso_trap3_anim_ctx, NULL);
        free(s_iso_trap3_anim_ctx);
        s_iso_trap3_anim_ctx = NULL;
    }
}

void pseudo3d_img_anim_iso_trap3(void *handle,
                                 lv_coord_t x, lv_coord_t y,
                                 lv_coord_t far_h, lv_coord_t near_h, lv_coord_t span_w,
                                 uint8_t shade_max,
                                 uint32_t dur_ms, uint32_t delay_ms)
{
    pseudo3d_img_t *p = (pseudo3d_img_t *)handle;
    if (!p) return;

    pseudo3d_iso_trap3_anim_ctx_t *c = (pseudo3d_iso_trap3_anim_ctx_t *)malloc(sizeof(*c));
    if (!c) return;
    /* 若上一轮动画仍在跑且针对同一 handle，先停掉（理论上不会同 handle 连续两次 start） */
    if (s_iso_trap3_anim_ctx && s_iso_trap3_anim_ctx->p == p) {
        lv_anim_del(s_iso_trap3_anim_ctx, NULL);
        free(s_iso_trap3_anim_ctx);
        s_iso_trap3_anim_ctx = NULL;
    }
    s_iso_trap3_anim_ctx = c;
    c->p = p;
    c->x = x;
    c->y = y;
    c->to.far_h = far_h;
    c->to.near_h = near_h;
    c->to.span_w = span_w;
    c->shade_max = shade_max;

    lv_anim_t an;
    lv_anim_init(&an);
    lv_anim_set_var(&an, c);
    lv_anim_set_exec_cb(&an, pseudo3d_iso_trap3_anim_exec);
    lv_anim_set_values(&an, 0, 1000);
    lv_anim_set_time(&an, dur_ms);
    lv_anim_set_delay(&an, delay_ms);
    lv_anim_set_path_cb(&an, lv_anim_path_ease_out);
    lv_anim_set_ready_cb(&an, pseudo3d_iso_trap3_anim_ready);
    an.user_data = c;
    lv_anim_start(&an);
}

typedef struct {
    pseudo3d_img_t *p;
    lv_coord_t x, y;
    pseudo3d_edges_t from;
    pseudo3d_edges_t to;
    uint8_t shade_max;
} pseudo3d_edges_anim_ctx_t;

static void pseudo3d_edges_anim_exec(void *var, int32_t v)
{
    pseudo3d_edges_anim_ctx_t *c = (pseudo3d_edges_anim_ctx_t *)var; /* 0..1000 */
    pseudo3d_edges_t cur;
    int32_t top_delta = ((int32_t)(c->to.top_len - c->from.top_len) * v) / 1000;
    int32_t bottom_delta = ((int32_t)(c->to.bottom_len - c->from.bottom_len) * v) / 1000;
    int32_t left_delta = ((int32_t)(c->to.left_len - c->from.left_len) * v) / 1000;
    int32_t right_delta = ((int32_t)(c->to.right_len - c->from.right_len) * v) / 1000;
    cur.top_len    = (lv_coord_t)((int32_t)c->from.top_len + top_delta);
    cur.bottom_len = (lv_coord_t)((int32_t)c->from.bottom_len + bottom_delta);
    cur.left_len   = (lv_coord_t)((int32_t)c->from.left_len + left_delta);
    cur.right_len  = (lv_coord_t)((int32_t)c->from.right_len + right_delta);
    pseudo3d_img_set_edges(c->p, c->x, c->y, &cur, c->shade_max);
}

static void pseudo3d_edges_anim_ready(lv_anim_t *a)
{
    pseudo3d_edges_anim_ctx_t *c = (pseudo3d_edges_anim_ctx_t *)a->user_data;
    free(c);
}

/* 新接口：从“原始平面”动画到目标四边长度（更直观）
 * - 起始边长用“原图尺寸”生成一个矩形：
 *   top=bottom=src_w, left=right=src_h
 */
void pseudo3d_img_anim_edges(void *handle,
                             lv_coord_t x, lv_coord_t y,
                             const pseudo3d_edges_t *to,
                             uint8_t shade_max,
                             uint32_t dur_ms, uint32_t delay_ms)
{
    pseudo3d_img_t *p = (pseudo3d_img_t *)handle;
    if (!p || !to) return;

    pseudo3d_edges_anim_ctx_t *c = (pseudo3d_edges_anim_ctx_t *)malloc(sizeof(*c));
    if (!c) return;
    c->p = p;
    c->x = x;
    c->y = y;
    c->shade_max = shade_max;
    c->from.top_len = p->src_w;
    c->from.bottom_len = p->src_w;
    c->from.left_len = p->src_h;
    c->from.right_len = p->src_h;
    c->to = *to;

    lv_anim_t an;
    lv_anim_init(&an);
    lv_anim_set_var(&an, c);
    lv_anim_set_exec_cb(&an, pseudo3d_edges_anim_exec);
    lv_anim_set_values(&an, 0, 1000);
    lv_anim_set_time(&an, dur_ms);
    lv_anim_set_delay(&an, delay_ms);
    lv_anim_set_path_cb(&an, lv_anim_path_ease_out);
    lv_anim_set_ready_cb(&an, pseudo3d_edges_anim_ready);
    an.user_data = c;
    lv_anim_start(&an);
}

typedef struct {
    pseudo3d_img_t *p;
    lv_coord_t x, y;
    lv_coord_t shear;
    float rot_tilt;
    float yaw0, yaw1;
} pseudo3d_anim_ctx_t;

static void pseudo3d_anim_exec(void *var, int32_t v)
{
    pseudo3d_anim_ctx_t *c = (pseudo3d_anim_ctx_t *)var; /* v:0..1000 */
    float yaw = c->yaw0 + (c->yaw1 - c->yaw0) * (float)v / 1000.0f;
    pseudo3d_img_set(c->p, c->x, c->y, yaw, c->shear, c->rot_tilt);
}

static void pseudo3d_anim_ready(lv_anim_t *a)
{
    pseudo3d_anim_ctx_t *c = (pseudo3d_anim_ctx_t *)a->user_data;
    free(c);
}

void pseudo3d_img_anim_yaw(void *handle,
                           lv_coord_t x, lv_coord_t y,
                           float yaw0_deg, float yaw1_deg,
                           lv_coord_t shear_px,
                           float rot_tilt_deg,
                           uint32_t dur_ms, uint32_t delay_ms)
{
    pseudo3d_img_t *p = (pseudo3d_img_t *)handle;
    if (!p) return;

    pseudo3d_anim_ctx_t *c = (pseudo3d_anim_ctx_t *)malloc(sizeof(*c));
    if (!c) return;
    c->p = p;
    c->x = x;
    c->y = y;
    c->shear = shear_px;
    c->rot_tilt = rot_tilt_deg;
    c->yaw0 = yaw0_deg;
    c->yaw1 = yaw1_deg;

    lv_anim_t an;
    lv_anim_init(&an);
    lv_anim_set_var(&an, c);
    lv_anim_set_exec_cb(&an, pseudo3d_anim_exec);
    lv_anim_set_values(&an, 0, 1000);
    lv_anim_set_time(&an, dur_ms);
    lv_anim_set_delay(&an, delay_ms);
    lv_anim_set_path_cb(&an, lv_anim_path_ease_out);
    lv_anim_set_ready_cb(&an, pseudo3d_anim_ready);
    an.user_data = c;
    lv_anim_start(&an);
}

/* ========== Label 字体阴影 / 3D 效果 ==========
 * LVGL 8 的 LABEL 没有“文字阴影”样式：lv_obj_set_style_shadow_* 是给**整个控件矩形**加阴影，
 * 不是沿字形轮廓的字体阴影，无法直接做出“字体 3D”效果。
 *
 * 实现方式：
 * 1) 多层 Label 叠放：底层 1～2 个 label 用 translate_x/y 偏移 + 深色/半透明，最上层主文字，
 *    形成描边或 3D 立体感（推荐，无需改 LVGL 源码）。
 * 2) 给 label 设 shadow 样式：只得到控件矩形的投影，适合“整块文字框”的阴影，不是字形阴影。
 *
 * 下面提供“多层 label”实现字体阴影/3D 的辅助接口。
 */

/** 多段文本：每段可指定字串与字体，用于多字号/多字体混排 */
typedef struct {
    const char *text;       /* 字串，可为 NULL 表示空 */
    const lv_font_t *font;  /* 字体，可为 NULL 用默认 */
} lv_label_segment_t;

/* UTF-8 字符数（按码点计，用于打字机效果） */
static uint32_t typing_utf8_char_count(const char *s)
{
    if (!s) return 0;
    uint32_t n = 0;
    while (*s) {
        if ((*s & 0xC0) != 0x80) n++;
        s++;
    }
    return n;
}

/* 将 s 的前 max_chars 个 UTF-8 码点复制到 out，并加 '\0'；out_size 为 out 字节数，不越界 */
static void typing_utf8_prefix(char *out, size_t out_size, const char *s, uint32_t max_chars)
{
    if (!out || out_size == 0) { *out = '\0'; return; }
    *out = '\0';
    if (!s) return;
    uint32_t count = 0;
    const char *p = s;
    size_t max_bytes = out_size - 1;  /* 预留 \0 */
    while (*p && count < max_chars && (size_t)(p - s) < max_bytes) {
        if (((unsigned char)*p & 0xC0) != 0x80) count++;
        p++;
    }
    /* 数够 max_chars 时 p 可能停在多字节字符中间，必须走到该字符末尾，避免每段最后一字（大/唱/吧）被截断 */
    while (*p && ((unsigned char)*p & 0xC0) == 0x80)
        p++;
    size_t len = (size_t)(p - s);
    if (len > max_bytes) {
        len = max_bytes;
        /* 不裁在 UTF-8 多字节字符中间，避免该位置“留空不显示” */
        while (len > 0 && ((unsigned char)s[len - 1] & 0xC0) == 0x80)
            len--;
    }
    memcpy(out, s, len);
    out[len] = '\0';
}

/* 嵌入式友好：每段缓冲缩小，避免 OOM；单段建议不超过约 96 字符 */
#define TYPING_BUF_SIZE 96
/* 多段 shadow 每段宽度余量(px)，避免测量略小导致 LONG_CLIP 裁切 */
#define SHADOW_SEG_WIDTH_MARGIN 16

/** 带阴影/3D 的 label 句柄：支持多段（多字号）；支持打字机效果；支持最大宽度与自动换行 */
typedef struct {
    lv_obj_t *cont;
    lv_obj_t *main;
    uint32_t segment_count;
    lv_obj_t **shadow_labels;
    lv_obj_t **main_labels;
    lv_coord_t max_width;   /* >0 时限制宽度并自动换行（LVGL 按断字字符换行，英文单词不拆开） */
    /* 打字机效果：总时长(ms)、总字符数、每段字符数、定时器、开始 tick；段文本副本(避免调用方栈失效) */
    uint32_t typing_duration_ms;
    uint32_t typing_total_chars;
    uint32_t *typing_segment_chars;
    lv_timer_t *typing_timer;
    uint32_t typing_start_tick;
    const lv_label_segment_t *typing_segments; /* 指向下面两个副本，非用户指针 */
    char *typing_buf_own;                       /* 段文本副本 buffer */
    lv_label_segment_t *typing_segments_own;    /* 段副本(.text 指向上面的 buf) */
} lv_label_shadow_handle_t;

/* 打字机定时器回调：按已过时间逐字更新各段显示 */
static void shadow_typing_timer_cb(lv_timer_t *tmr)
{
    lv_label_shadow_handle_t *h = (lv_label_shadow_handle_t *)lv_timer_get_user_data(tmr);
    if (!h || !h->cont || !h->typing_segment_chars || !h->typing_segments) return;
    uint32_t elapsed = lv_tick_elaps(h->typing_start_tick);
    uint32_t duration = h->typing_duration_ms;
    uint32_t total = h->typing_total_chars;
    if (total == 0) { lv_timer_del(tmr); h->typing_timer = NULL; return; }
    uint32_t current_chars = (elapsed >= duration) ? total : (uint32_t)((uint64_t)elapsed * (uint64_t)total / (uint64_t)duration);
    if (current_chars > total) current_chars = total;

    char buf[TYPING_BUF_SIZE];
    uint32_t prefix = 0;
    for (uint32_t i = 0; i < h->segment_count; i++) {
        uint32_t seg_chars = h->typing_segment_chars[i];
        uint32_t show = 0;
        if (current_chars > prefix)
            show = (current_chars - prefix) < seg_chars ? (current_chars - prefix) : seg_chars;
        const char *src = h->typing_segments[i].text ? h->typing_segments[i].text : "";
        typing_utf8_prefix(buf, sizeof(buf), src, show);
        if (h->shadow_labels && h->shadow_labels[i]) lv_label_set_text(h->shadow_labels[i], buf);
        if (h->main_labels && h->main_labels[i]) lv_label_set_text(h->main_labels[i], buf);
        prefix += seg_chars;
    }
    if (current_chars >= total) {
        lv_timer_del(tmr);
        h->typing_timer = NULL;
    }
}

/* 判断 UTF-8 串是否以标点开头（含 ASCII 与全角 ，。！？、；：），用于 outline8 以标点开头的段强制换行 */
static int segment_starts_with_punct(const char *s)
{
    if (!s || !*s) return 0;
    unsigned char c = (unsigned char)*s;
    if ((c & 0x80) == 0)
        return (c == ',' || c == '.' || c == '!' || c == '?' || c == ';' || c == ':' || c == ' ');
    if (c == 0xEF && s[1] && s[2] && (unsigned char)s[1] == 0xBC && (unsigned char)s[2] >= 0x80) return 1;
    if (c == 0xE3 && s[1] && s[2] && (unsigned char)s[1] == 0x80) {
        unsigned char t = (unsigned char)s[2];
        return (t == 0x82 || (t >= 0x81 && t <= 0x84));
    }
    return 0;
}

/* 测量一段文本在给定字体下的宽高（用临时 label 测；parent 用于创建临时对象） */
static void segment_measure(lv_obj_t *parent, const lv_label_segment_t *seg,
                            lv_coord_t *out_w, lv_coord_t *out_h)
{
    lv_obj_t *tmp = lv_label_create(parent ? parent : lv_scr_act());
    lv_label_set_text(tmp, seg->text ? seg->text : "");
    if (seg->font) lv_obj_set_style_text_font(tmp, seg->font, 0);
    lv_obj_add_flag(tmp, LV_OBJ_FLAG_HIDDEN);
    lv_obj_update_layout(tmp);
    *out_w = lv_obj_get_width(tmp);
    *out_h = lv_obj_get_height(tmp);
    lv_obj_del(tmp);
    if (*out_w < 0) *out_w = 0;
    if (*out_h < 0) *out_h = 0;
}

/**
 * 创建一个带阴影色的 label，支持多段字串/多字体（多字号）。
 * @param max_width 最大宽度(px)；>0 时超宽自动换行（LVGL 按断字字符换行，英文等单词不拆开）；0 不限制
 * @param typing_duration_ms 打字机效果总时长(ms)；0=直接显示全文，非0=逐字显示
 * @return 句柄，失败返回 NULL。
 */
void *lv_label_create_with_shadow(lv_obj_t *parent,
                                 const lv_label_segment_t *segments,
                                 uint32_t segment_count,
                                 lv_coord_t max_width,
                                 lv_coord_t shadow_offset_x,
                                 lv_coord_t shadow_offset_y,
                                 lv_color_t shadow_color,
                                 lv_opa_t shadow_opa,
                                 lv_color_t text_color,
                                 uint32_t typing_duration_ms)
{
    if (!parent) return NULL;
    if (!segments || segment_count == 0) return NULL;

    lv_label_shadow_handle_t *h = (lv_label_shadow_handle_t *)malloc(sizeof(lv_label_shadow_handle_t));
    if (!h) return NULL;
    memset(h, 0, sizeof(*h));
    h->segment_count = segment_count;
    h->typing_duration_ms = typing_duration_ms;
    h->max_width = max_width;
    h->shadow_labels = (lv_obj_t **)calloc(segment_count, sizeof(lv_obj_t *));
    h->main_labels = (lv_obj_t **)calloc(segment_count, sizeof(lv_obj_t *));
    if (!h->shadow_labels || !h->main_labels) {
        free(h->shadow_labels);
        free(h->main_labels);
        free(h);
        return NULL;
    }

    h->cont = lv_obj_create(parent);
    lv_obj_set_style_pad_all(h->cont, 0, 0);
    lv_obj_set_style_bg_opa(h->cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(h->cont, 0, 0);
    lv_obj_clear_flag(h->cont, LV_OBJ_FLAG_SCROLLABLE);

    /* 第一遍：测量各段宽高，得到整行最大高度 total_h */
    lv_coord_t *seg_ws = (lv_coord_t *)malloc(segment_count * sizeof(lv_coord_t));
    lv_coord_t *seg_hs = (lv_coord_t *)malloc(segment_count * sizeof(lv_coord_t));
    if (!seg_ws || !seg_hs) {
        free(seg_ws);
        free(seg_hs);
        lv_obj_del(h->cont);
        free(h->shadow_labels);
        free(h->main_labels);
        free(h);
        return NULL;
    }
    lv_coord_t total_h = 0;
    for (uint32_t i = 0; i < segment_count; i++) {
        segment_measure(parent, &segments[i], &seg_ws[i], &seg_hs[i]);
        if (seg_ws[i] < 1) seg_ws[i] = 1;
        if (seg_hs[i] < 1) seg_hs[i] = 20;
        if (seg_hs[i] > total_h) total_h = seg_hs[i];
        divoom_print("[shadow] seg#%u w=%d h=%d text=%.32s\n", (unsigned)i, (int)seg_ws[i], (int)seg_hs[i],
            segments[i].text ? segments[i].text : "");
    }

    int use_typing = (typing_duration_ms > 0);
    if (use_typing) {
        h->typing_segment_chars = (uint32_t *)malloc(segment_count * sizeof(uint32_t));
        h->typing_buf_own = (char *)malloc((size_t)segment_count * TYPING_BUF_SIZE);
        h->typing_segments_own = (lv_label_segment_t *)malloc(segment_count * sizeof(lv_label_segment_t));
        if (!h->typing_segment_chars || !h->typing_buf_own || !h->typing_segments_own) {
            free(h->typing_segment_chars);
            free(h->typing_buf_own);
            free(h->typing_segments_own);
            use_typing = 0;
        } else {
            h->typing_total_chars = 0;
            for (uint32_t i = 0; i < segment_count; i++) {
                const char *t = segments[i].text ? segments[i].text : "";
                size_t len = strlen(t);
                if (len >= TYPING_BUF_SIZE) len = TYPING_BUF_SIZE - 1;
                char *dst = h->typing_buf_own + i * TYPING_BUF_SIZE;
                memcpy(dst, t, len);
                dst[len] = '\0';
                h->typing_segments_own[i].font = segments[i].font;
                h->typing_segments_own[i].text = dst;
                h->typing_segment_chars[i] = typing_utf8_char_count(t);
                h->typing_total_chars += h->typing_segment_chars[i];
            }
            h->typing_segments = h->typing_segments_own;
        }
    }

    /* max_width>0 时多段水平排列，超宽才换行；同一行内各段垂直居中，上边下边留空 */
    lv_coord_t cur_x = 0;
    lv_coord_t cur_y = 0;
    lv_coord_t line_h = 0;
    uint32_t line_start = 0;
    for (uint32_t i = 0; i < segment_count; i++) {
        /* 换行只按测量宽度判断，不把余量算进去，避免提前换行导致行数增多、底部被裁、显示变少 */
        if (max_width > 0 && cur_x + seg_ws[i] > max_width && cur_x > 0) {
            divoom_print("[shadow] wrap: seg#%u cur_x=%d seg_w=%d max_width=%d (whole segment to next line)\n",
                (unsigned)i, (int)cur_x, (int)seg_ws[i], (int)max_width);
            /* 换行前：把上一行已放置的 segment 垂直居中（上边下边留空） */
            for (uint32_t j = line_start; j < i; j++) {
                lv_coord_t sy = cur_y + (line_h - seg_hs[j]) / 2;
                lv_obj_set_y(h->shadow_labels[j], sy);
                lv_obj_set_y(h->main_labels[j], sy);
            }
            cur_x = 0;
            cur_y += line_h;
            line_start = i;
            line_h = 0;
        }
        lv_coord_t seg_y = (max_width > 0) ? cur_y : (total_h - seg_hs[i]) / 2;
        const char *init_text = (use_typing && h->typing_total_chars > 0) ? "" : (segments[i].text ? segments[i].text : "");

        /* 固定宽度：上游 word_cube 已在建段时按 max_width 在词边界拆分，此处每段宽度均 <= max_width，用 CLIP 保持原排版 */
        lv_coord_t seg_w_display = (max_width > 0) ? (seg_ws[i] + (lv_coord_t)SHADOW_SEG_WIDTH_MARGIN) : 0;

        lv_obj_t *shadow = lv_label_create(h->cont);
        lv_label_set_text(shadow, init_text);
        lv_obj_set_style_text_color(shadow, shadow_color, 0);
        lv_obj_set_style_text_opa(shadow, shadow_opa, 0);
        if (segments[i].font) lv_obj_set_style_text_font(shadow, segments[i].font, 0);
        lv_obj_set_style_translate_x(shadow, shadow_offset_x, 0);
        lv_obj_set_style_translate_y(shadow, shadow_offset_y, 0);
        if (max_width > 0) {
            lv_obj_set_width(shadow, seg_w_display);
            lv_label_set_long_mode(shadow, LV_LABEL_LONG_CLIP);
        }
        lv_obj_set_pos(shadow, cur_x, seg_y);
        h->shadow_labels[i] = shadow;

        lv_obj_t *main_lab = lv_label_create(h->cont);
        lv_label_set_text(main_lab, init_text);
        lv_obj_set_style_text_color(main_lab, text_color, 0);
        if (segments[i].font) lv_obj_set_style_text_font(main_lab, segments[i].font, 0);
        if (max_width > 0) {
            lv_obj_set_width(main_lab, seg_w_display);
            lv_label_set_long_mode(main_lab, LV_LABEL_LONG_CLIP);
        }
        lv_obj_set_pos(main_lab, cur_x, seg_y);
        h->main_labels[i] = main_lab;

        if (segment_count == 1) h->main = main_lab;

        divoom_print("[shadow] place seg#%u x=%d y=%d w_display=%d w_layout=%d len=%u text=%.24s\n",
            (unsigned)i, (int)cur_x, (int)seg_y, (int)seg_w_display, (int)seg_ws[i],
            (unsigned)(init_text ? strlen(init_text) : 0), init_text ? init_text : "");
        cur_x += seg_ws[i];
        if (seg_hs[i] > line_h) line_h = seg_hs[i];
    }
    divoom_print("[shadow] layout done: seg_count=%u cur_y+line_h=%d max_width=%d (total height of shadow block)\n",
        (unsigned)segment_count, (int)(cur_y + line_h), (int)max_width);
    /* 最后一行：垂直居中（用各 label 实际高度，兼容 WRAP 多行段） */
    if (max_width > 0 && line_h > 0) {
        for (uint32_t j = line_start; j < segment_count; j++) {
            lv_coord_t lh = lv_obj_get_height(h->shadow_labels[j]);
            if (lh < 1) lh = seg_hs[j];
            lv_coord_t sy = cur_y + (line_h - lh) / 2;
            lv_obj_set_y(h->shadow_labels[j], sy);
            lv_obj_set_y(h->main_labels[j], sy);
        }
    }
    free(seg_ws);
    free(seg_hs);

    if (use_typing && h->typing_total_chars > 0) {
        h->typing_start_tick = lv_tick_get();
        h->typing_timer = lv_timer_create(shadow_typing_timer_cb, 40, h);
    }

    lv_obj_update_layout(h->cont);
    lv_coord_t cw, ch;
    if (max_width > 0) {
        cw = max_width + (lv_coord_t)(shadow_offset_x > 0 ? shadow_offset_x : -shadow_offset_x) + 4;
        ch = cur_y + line_h + (lv_coord_t)(shadow_offset_y > 0 ? shadow_offset_y : -shadow_offset_y) + 4;
    } else {
        cw = cur_x + (lv_coord_t)(shadow_offset_x > 0 ? shadow_offset_x : -shadow_offset_x) + 4;
        if (total_h < 1) total_h = 60;
        ch = total_h + (lv_coord_t)(shadow_offset_y > 0 ? shadow_offset_y : -shadow_offset_y) + 4;
    }
    lv_obj_set_size(h->cont, cw, ch);
    lv_obj_add_flag(h->cont, LV_OBJ_FLAG_OVERFLOW_VISIBLE);

    return h;
}

/** 获取带阴影 label 实际使用的高度（换行后总高度）；创建后或 set_segments 后调用前建议先 lv_obj_update_layout(cont) */
lv_coord_t lv_label_with_shadow_get_height(void *h_OBJ)
{
    lv_label_shadow_handle_t *sh = (lv_label_shadow_handle_t *)h_OBJ;
    if (!sh || !sh->cont) return 0;
    lv_obj_update_layout(sh->cont);
    return lv_obj_get_height(sh->cont);
}

/** 设置带阴影的 label 的文本（按段更新；阴影与主文字同步） */
void lv_label_with_shadow_set_text(lv_label_shadow_handle_t *h, const char *text)
{
    if (!h || !h->cont) return;
    /* 单段：更新第一个主 label 与第一个阴影 label */
    if (h->segment_count > 0 && h->main_labels && h->shadow_labels) {
        if (h->shadow_labels[0]) lv_label_set_text(h->shadow_labels[0], text ? text : "");
        if (h->main_labels[0]) lv_label_set_text(h->main_labels[0], text ? text : "");
    }
}

/** 按段设置带阴影的 label（多段字串/字体） */
void lv_label_with_shadow_set_segments(lv_label_shadow_handle_t *h,
                                       const lv_label_segment_t *segments,
                                       uint32_t segment_count)
{
    if (!h || !h->cont || !segments || segment_count == 0) return;
    uint32_t n = segment_count <= h->segment_count ? segment_count : h->segment_count;
    for (uint32_t i = 0; i < n; i++) {
        if (h->shadow_labels[i])
            lv_label_set_text(h->shadow_labels[i], segments[i].text ? segments[i].text : "");
        if (h->main_labels[i]) {
            lv_label_set_text(h->main_labels[i], segments[i].text ? segments[i].text : "");
            if (segments[i].font) lv_obj_set_style_text_font(h->main_labels[i], segments[i].font, 0);
        }
    }
}

/** 释放带阴影的 label（会删除 cont 及所有子对象） */
void lv_label_with_shadow_delete(void *h_OBJ)
{
    lv_label_shadow_handle_t *h = (lv_label_shadow_handle_t *)h_OBJ;
    if (!h) return;
    if (h->typing_timer) { lv_timer_del(h->typing_timer); h->typing_timer = NULL; }
    if (h->cont) lv_obj_del(h->cont);
    free(h->shadow_labels);
    free(h->main_labels);
    free(h->typing_segment_chars);
    free(h->typing_buf_own);
    free(h->typing_segments_own);
    free(h);
}

/* ========== 8 方向描边（只传一个偏移量），两种模式 ========== */
/** 模式：0=中心透明（透视频），1=描边+框内不透明（可设描边色与填充色） */
#define LV_LABEL_OUTLINE8_MODE_TRANSPARENT_CENTER  0
#define LV_LABEL_OUTLINE8_MODE_OPAQUE_FILL         1
/** 1=canvas 单对象 8 向实心包边（outline8_canvas_draw_segments_opaque_fill）；0=9 层 label 偏移（外圈易不均匀） */
#ifndef OUTLINE8_OPAQUE_USE_CANVAS
#define OUTLINE8_OPAQUE_USE_CANVAS  1
#endif

typedef struct {
    lv_obj_t *cont;
    lv_obj_t *main;
    lv_obj_t *canvas;
    void *buf;
    lv_coord_t w, h;
    lv_color_t outline_color;
    lv_opa_t outline_opa;
    lv_coord_t offset_px;
    const lv_font_t *font;
    uint8_t mode;
    lv_color_t fill_color;
    uint32_t segment_count;
    const lv_label_segment_t *segments; /* 始终指向下面 segments_own，不持用户指针 */
    lv_obj_t **outline_labels;
    lv_obj_t **main_labels;
    /* 段文本副本：create 时复制，避免调用方栈失效导致只显示残字 */
    char *segments_own_buf;
    lv_label_segment_t *segments_own;
    /* 打字机效果 */
    uint32_t typing_duration_ms;
    uint32_t typing_total_chars;
    uint32_t *typing_segment_chars;
    lv_timer_t *typing_timer;
    uint32_t typing_start_tick;
    const lv_label_segment_t *typing_origin_segments; /* 与 segments_own 相同 */
    char *typing_buf;                  /* 每段 TYPING_BUF_SIZE 字节 */
    lv_label_segment_t *typing_segments_display; /* 打字时用于绘制的段(指向 typing_buf) */
    lv_obj_t *cursor_img;              /* 可选：打字机时作为“笔”位置，定时器会更新其位置到下一个要显示的字处 */
    lv_coord_t max_width;              /* >0 时限制宽度并自动换行（LVGL 按断字字符换行，英文单词不拆开） */
} lv_label_outline8_handle_t;
static void outline8_canvas_draw_segments(lv_label_outline8_handle_t *h);
static void outline8_canvas_draw_segments_opaque_fill(lv_label_outline8_handle_t *h);

static void outline8_canvas_redraw(lv_label_outline8_handle_t *h)
{
    if (!h || !h->canvas)
    {
        return;
    }
    if (h->mode == LV_LABEL_OUTLINE8_MODE_OPAQUE_FILL)
    {
        outline8_canvas_draw_segments_opaque_fill(h);
    }
    else
    {
        outline8_canvas_draw_segments(h);
    }
}

/* 8 方向偏移：(dx,dy) = 右/左/下/上/右下/左下/右上/左上 */
static const int8_t outline8_dx[] = { 1, -1, 0, 0, 1, -1, 1, -1 };
static const int8_t outline8_dy[] = { 0, 0, 1, -1, 1, 1, -1, -1 };

/* 打字机定时器回调：outline8 逐字更新（label 或 canvas）；若有 cursor_img 则更新其位置到“下一个要显示的字”处 */
static void outline8_typing_timer_cb(lv_timer_t *tmr)
{
    lv_label_outline8_handle_t *h = (lv_label_outline8_handle_t *)lv_timer_get_user_data(tmr);
    if (!h || !h->cont || !h->typing_segment_chars || !h->typing_origin_segments) return;
    uint32_t elapsed = lv_tick_elaps(h->typing_start_tick);
    uint32_t duration = h->typing_duration_ms;
    uint32_t total = h->typing_total_chars;
    if (total == 0) { lv_timer_del(tmr); h->typing_timer = NULL; return; }
    uint32_t current_chars = (elapsed >= duration) ? total : (uint32_t)((uint64_t)elapsed * (uint64_t)total / (uint64_t)duration);
    if (current_chars > total) current_chars = total;

    uint32_t prefix = 0;
    uint32_t last_seg_with_show = 0;
    for (uint32_t i = 0; i < h->segment_count; i++) {
        uint32_t seg_chars = h->typing_segment_chars[i];
        uint32_t show = 0;
        if (current_chars > prefix)
            show = (current_chars - prefix) < seg_chars ? (current_chars - prefix) : seg_chars;
        if (show > 0) last_seg_with_show = i;
        const char *src = h->typing_origin_segments[i].text ? h->typing_origin_segments[i].text : "";
        if (h->typing_buf && h->typing_segments_display) {
            typing_utf8_prefix(h->typing_buf + i * TYPING_BUF_SIZE, TYPING_BUF_SIZE, src, show);
            if (h->main_labels && h->main_labels[i]) {
                lv_label_set_text(h->main_labels[i], h->typing_buf + i * TYPING_BUF_SIZE);
                lv_obj_invalidate(h->main_labels[i]);
            }
            if (h->outline_labels) {
                for (int d = 0; d < 8; d++) {
                    lv_obj_t *o = h->outline_labels[d * (int)h->segment_count + i];
                    if (o) {
                        lv_label_set_text(o, h->typing_buf + i * TYPING_BUF_SIZE);
                        lv_obj_invalidate(o);
                    }
                }
            }
        }
        prefix += seg_chars;
    }

    /* 若有 cursor_img 且为 OPAQUE_FILL（有 main_labels），更新“笔”位置到下一个要显示的字处 */
    if (h->cursor_img && h->main_labels && h->typing_buf) {
        uint32_t seg_idx = last_seg_with_show;
        const char *prefix_str = h->typing_buf + seg_idx * TYPING_BUF_SIZE;
        size_t prefix_len = strlen(prefix_str);
        const lv_font_t *font = h->segments[seg_idx].font ? h->segments[seg_idx].font : h->font;
        lv_coord_t prefix_w = (font && prefix_len > 0)
            ? divoom_lv_txt_get_width(prefix_str, (uint32_t)prefix_len, font, 0, LV_TEXT_FLAG_NONE)
            : 0;
        lv_obj_t *lab = h->main_labels[seg_idx];
        if (lab) {
            lv_coord_t cont_x = lv_obj_get_x(h->cont);
            lv_coord_t cont_y = lv_obj_get_y(h->cont);
            lv_coord_t lab_x = lv_obj_get_x(lab);
            lv_coord_t lab_y = lv_obj_get_y(lab);
            lv_coord_t lab_h = lv_obj_get_height(lab);
            lv_coord_t pen_x = cont_x + lab_x + prefix_w;
            lv_coord_t pen_y = cont_y + lab_y + lab_h / 2;
            lv_obj_set_pos(h->cursor_img, pen_x, pen_y);
        }
    }

#if LV_USE_CANVAS
    if (h->canvas && h->typing_segments_display) {
        h->segments = h->typing_segments_display;
        outline8_canvas_redraw(h);
        h->segments = h->typing_origin_segments;
    }
#endif
    if (current_chars >= total) {
        lv_timer_del(tmr);
        h->typing_timer = NULL;
    }
}

#if LV_USE_CANVAS
#define OUTLINE8_CANVAS_MAX_SEG 32
/* 多段时在 canvas 上绘制空心描边：8 向描边 + mask 挖空中心；max_width>0 时换行布局完整显示 */
static void outline8_canvas_draw_segments(lv_label_outline8_handle_t *h)
{
    if (!h || !h->canvas || !h->buf || !h->segments || h->segment_count == 0) return;
    lv_img_dsc_t *main_dsc = lv_canvas_get_img(h->canvas);
    if (!main_dsc || !main_dsc->data) return;
    lv_coord_t w = h->w, off = h->offset_px;
    /* 与 creation 一致：可绘制区域 [off, off+max_width]，故换行阈值为 max_width+off */
    lv_coord_t seg_w_max = (h->max_width > 0) ? (h->max_width + off) : 0;
    size_t buf_size = divoom_pixel_format_size(w, h->h, LV_COLOR_FORMAT_RGB565A8);
    memset(h->buf, 0, buf_size);

    lv_obj_t *parent = lv_obj_get_parent(h->canvas);
    lv_coord_t seg_ws[OUTLINE8_CANVAS_MAX_SEG], seg_hs[OUTLINE8_CANVAS_MAX_SEG];
    uint32_t n = h->segment_count <= OUTLINE8_CANVAS_MAX_SEG ? h->segment_count : OUTLINE8_CANVAS_MAX_SEG;
    for (uint32_t s = 0; s < n; s++) {
        segment_measure(parent, &h->segments[s], &seg_ws[s], &seg_hs[s]);
        if (seg_ws[s] < 1) seg_ws[s] = 1;
        if (seg_hs[s] < 1) seg_hs[s] = 20;
    }

    lv_coord_t seg_x[OUTLINE8_CANVAS_MAX_SEG], seg_y[OUTLINE8_CANVAS_MAX_SEG];
    lv_coord_t cur_x = off, cur_y = off, line_h = 0;
    uint32_t line_start = 0;
    for (uint32_t s = 0; s < n; s++) {
        if (seg_w_max > 0 && cur_x + seg_ws[s] > seg_w_max && cur_x > off) {
            for (uint32_t j = line_start; j < s; j++)
                seg_y[j] = cur_y + (line_h - seg_hs[j]) / 2;
            cur_y += line_h;
            cur_x = off;
            line_start = s;
            line_h = 0;
        }
        seg_x[s] = cur_x;
        seg_y[s] = cur_y;
        cur_x += seg_ws[s];
        if (seg_hs[s] > line_h) line_h = seg_hs[s];
    }
    for (uint32_t j = line_start; j < n; j++)
        seg_y[j] = cur_y + (line_h - seg_hs[j]) / 2;

    lv_draw_label_dsc_t label_dsc;
    lv_draw_label_dsc_init(&label_dsc);
    label_dsc.color = h->outline_color;
    label_dsc.opa = h->outline_opa;
    label_dsc.align = LV_TEXT_ALIGN_LEFT;

    for (uint32_t s = 0; s < n; s++) {
        const char *str = h->segments[s].text ? h->segments[s].text : "";
        label_dsc.font = h->segments[s].font ? h->segments[s].font : lv_font_default();
        lv_coord_t max_w = w - seg_x[s] - off;
        if (max_w < 1) break;
        for (int i = 0; i < 8; i++) {
            lv_coord_t px = seg_x[s] + (lv_coord_t)(outline8_dx[i] * off);
            lv_coord_t py = seg_y[s] + (lv_coord_t)(outline8_dy[i] * off);
            lv_canvas_draw_text(h->canvas, px, py, max_w, &label_dsc, str);
        }
    }

    void *mask_buf = malloc(buf_size);
    if (!mask_buf) return;
    memset(mask_buf, 0, buf_size);
    lv_obj_t *mask_canvas = lv_canvas_create(parent);
    lv_canvas_set_buffer(mask_canvas, mask_buf, w, h->h, LV_COLOR_FORMAT_RGB565A8);
    lv_obj_add_flag(mask_canvas, LV_OBJ_FLAG_HIDDEN);
    label_dsc.color = lv_color_white();
    label_dsc.opa = LV_OPA_COVER;
    for (uint32_t s = 0; s < n; s++) {
        const char *str = h->segments[s].text ? h->segments[s].text : "";
        label_dsc.font = h->segments[s].font ? h->segments[s].font : lv_font_default();
        lv_coord_t max_w = w - seg_x[s] - off;
        if (max_w < 1) break;
        lv_canvas_draw_text(mask_canvas, seg_x[s], seg_y[s], max_w, &label_dsc, str);
    }
    lv_img_dsc_t *mask_dsc = lv_canvas_get_img(mask_canvas);
    for (lv_coord_t y = 0; y < h->h; y++) {
        for (lv_coord_t x = 0; x < w; x++) {
            if (lv_img_buf_get_px_alpha(mask_dsc, x, y) > 0)
                lv_img_buf_set_px_alpha(main_dsc, x, y, 0);
        }
    }
    lv_obj_del(mask_canvas);
    free(mask_buf);
    lv_obj_invalidate(h->canvas);
}

/* 模式1 canvas：8 向描边 + 中心填充，单 canvas 单对象（避免 9*segment 个 label transform 拖影） */
static void outline8_canvas_draw_segments_opaque_fill(lv_label_outline8_handle_t *h)
{
    if (!h || !h->canvas || !h->buf || !h->segments || h->segment_count == 0)
    {
        return;
    }
    lv_img_dsc_t *main_dsc = lv_canvas_get_img(h->canvas);
    if (!main_dsc || !main_dsc->data)
    {
        return;
    }
    lv_coord_t w = h->w;
    lv_coord_t off = h->offset_px;
    lv_coord_t seg_w_max = (h->max_width > 0) ? (h->max_width + off) : 0;
    size_t buf_size = divoom_pixel_format_size(w, h->h, LV_COLOR_FORMAT_RGB565A8);

    lv_obj_t *parent = lv_obj_get_parent(h->canvas);
    lv_coord_t seg_ws[OUTLINE8_CANVAS_MAX_SEG];
    lv_coord_t seg_hs[OUTLINE8_CANVAS_MAX_SEG];
    uint32_t n = h->segment_count <= OUTLINE8_CANVAS_MAX_SEG ? h->segment_count : OUTLINE8_CANVAS_MAX_SEG;
    uint32_t s;
    uint32_t j;
    int i;

    memset(h->buf, 0, buf_size);
    for (s = 0; s < n; s++)
    {
        segment_measure(parent, &h->segments[s], &seg_ws[s], &seg_hs[s]);
        if (seg_ws[s] < 1)
        {
            seg_ws[s] = 1;
        }
        if (seg_hs[s] < 1)
        {
            seg_hs[s] = 20;
        }
    }

    lv_coord_t seg_x[OUTLINE8_CANVAS_MAX_SEG];
    lv_coord_t seg_y[OUTLINE8_CANVAS_MAX_SEG];
    lv_coord_t cur_x = off;
    lv_coord_t cur_y = off;
    lv_coord_t line_h = 0;
    uint32_t line_start = 0;

    for (s = 0; s < n; s++)
    {
        if (seg_w_max > 0 && cur_x + seg_ws[s] > seg_w_max && cur_x > off)
        {
            for (j = line_start; j < s; j++)
            {
                seg_y[j] = cur_y + (line_h - seg_hs[j]) / 2;
            }
            cur_y += line_h;
            cur_x = off;
            line_start = s;
            line_h = 0;
        }
        seg_x[s] = cur_x;
        seg_y[s] = cur_y;
        cur_x += seg_ws[s];
        if (seg_hs[s] > line_h)
        {
            line_h = seg_hs[s];
        }
    }
    for (j = line_start; j < n; j++)
    {
        seg_y[j] = cur_y + (line_h - seg_hs[j]) / 2;
    }

    lv_draw_label_dsc_t label_dsc;
    lv_draw_label_dsc_init(&label_dsc);
    label_dsc.align = LV_TEXT_ALIGN_LEFT;

    label_dsc.color = h->outline_color;
    label_dsc.opa = h->outline_opa;
    for (s = 0; s < n; s++)
    {
        const char *str = h->segments[s].text ? h->segments[s].text : "";
        lv_coord_t max_w = w - seg_x[s] - off;

        label_dsc.font = h->segments[s].font ? h->segments[s].font : lv_font_default();
        if (max_w < 1)
        {
            break;
        }
        for (i = 0; i < 8; i++)
        {
            lv_coord_t px = seg_x[s] + (lv_coord_t)(outline8_dx[i] * off);
            lv_coord_t py = seg_y[s] + (lv_coord_t)(outline8_dy[i] * off);

            lv_canvas_draw_text(h->canvas, px, py, max_w, &label_dsc, str);
        }
    }

    label_dsc.color = h->fill_color;
    label_dsc.opa = LV_OPA_COVER;
    for (s = 0; s < n; s++)
    {
        const char *str = h->segments[s].text ? h->segments[s].text : "";
        lv_coord_t max_w = w - seg_x[s] - off;

        label_dsc.font = h->segments[s].font ? h->segments[s].font : lv_font_default();
        if (max_w < 1)
        {
            break;
        }
        lv_canvas_draw_text(h->canvas, seg_x[s], seg_y[s], max_w, &label_dsc, str);
    }
    lv_obj_invalidate(h->canvas);
}
#endif

/**
 * 创建 8 方向描边，支持多段字串/多字体；两种模式。
 * @param max_width 最大宽度(px)；>0 时超宽自动换行（LVGL 按断字字符换行，英文等单词不拆开）；0 不限制
 * @param typing_duration_ms 打字机效果总时长(ms)；0=直接显示全文，非0=逐字显示
 * @param cursor_img 可选 LVGL 对象（如 lv_img）：非空时在打字机效果中会持续更新其位置到“下一个要显示的字”处，类似笔尖
 */
void *lv_label_create_with_outline8(lv_obj_t *parent,
                                    const lv_label_segment_t *segments,
                                    uint32_t segment_count,
                                    lv_coord_t max_width,
                                    lv_coord_t offset_px,
                                    lv_color_t outline_color,
                                    lv_opa_t outline_opa,
                                    uint8_t mode,
                                    lv_color_t fill_color,
                                    uint32_t typing_duration_ms,
                                    lv_obj_t *cursor_img)
{
    if (!parent || !segments || segment_count == 0) return NULL;

    lv_label_outline8_handle_t *h = (lv_label_outline8_handle_t *)malloc(sizeof(lv_label_outline8_handle_t));
    lv_coord_t outline_off_req;
    if (!h) return NULL;
    memset(h, 0, sizeof(*h));
    outline_off_req = offset_px;
    if (offset_px < 1)
    {
        offset_px = 1;
    }
    h->outline_color = outline_color;
    h->outline_opa = outline_opa;
    h->offset_px = offset_px;
    h->segment_count = segment_count;
    h->typing_duration_ms = typing_duration_ms;
    h->mode = (mode != 0) ? LV_LABEL_OUTLINE8_MODE_OPAQUE_FILL : LV_LABEL_OUTLINE8_MODE_TRANSPARENT_CENTER;
    h->fill_color = fill_color;

    int use_typing = (typing_duration_ms > 0);
#if LV_USE_CANVAS
    int need_segments_copy = use_typing || (h->mode == LV_LABEL_OUTLINE8_MODE_TRANSPARENT_CENTER);
#else
    int need_segments_copy = use_typing;
#endif
    /* 打字机时必须先复制段：前一个 create 的定时器可能已覆盖调用方栈上的 segments，导致只统计到前几字 */
    if (need_segments_copy) {
        h->segments_own_buf = (char *)malloc((size_t)segment_count * TYPING_BUF_SIZE);
        h->segments_own = (lv_label_segment_t *)malloc(segment_count * sizeof(lv_label_segment_t));
        if (!h->segments_own_buf || !h->segments_own) {
            free(h->segments_own_buf);
            free(h->segments_own);
            free(h);
            return NULL;
        }
        for (uint32_t i = 0; i < segment_count; i++) {
            const char *t = segments[i].text ? segments[i].text : "";
            size_t len = strlen(t);
            if (len >= TYPING_BUF_SIZE) {
                len = TYPING_BUF_SIZE - 1;
                /* 不裁在 UTF-8 多字节字符中间，避免该段末尾“空位不显示” */
                while (len > 0 && ((unsigned char)t[len - 1] & 0xC0) == 0x80)
                    len--;
            }
            char *dst = h->segments_own_buf + i * TYPING_BUF_SIZE;
            memcpy(dst, t, len);
            dst[len] = '\0';
            h->segments_own[i].font = segments[i].font;
            h->segments_own[i].text = dst;
        }
        h->segments = h->segments_own;
        h->typing_origin_segments = h->segments_own;
    } else {
        h->segments = segments;
        h->typing_origin_segments = segments;
    }
    if (segment_count == 1) h->font = (segments[0].font != NULL) ? segments[0].font : (h->segments_own ? h->segments_own[0].font : NULL);
    h->cursor_img = cursor_img;
    h->max_width = max_width;

    if (use_typing) {
        h->typing_segment_chars = (uint32_t *)malloc(segment_count * sizeof(uint32_t));
        h->typing_buf = (char *)malloc((size_t)segment_count * TYPING_BUF_SIZE);
        h->typing_segments_display = (lv_label_segment_t *)malloc(segment_count * sizeof(lv_label_segment_t));
        if (!h->typing_segment_chars || !h->typing_buf || !h->typing_segments_display) {
            free(h->typing_segment_chars);
            free(h->typing_buf);
            free(h->typing_segments_display);
            use_typing = 0;
        } else {
            /* 字长只从已复制的 segments_own 统计，与定时器读取的源一致，避免栈被前一个定时器覆盖后只得到前几字 */
            h->typing_total_chars = 0;
            for (uint32_t i = 0; i < segment_count; i++) {
                const char *t = h->segments[i].text ? h->segments[i].text : "";
                h->typing_segment_chars[i] = typing_utf8_char_count(t);
                h->typing_total_chars += h->typing_segment_chars[i];
                h->typing_segments_display[i].font = h->segments[i].font;
                h->typing_segments_display[i].text = h->typing_buf + i * TYPING_BUF_SIZE;
            }
            for (uint32_t i = 0; i < segment_count; i++)
                h->typing_buf[i * TYPING_BUF_SIZE] = '\0';
        }
    }

    h->cont = lv_obj_create(parent);
    lv_obj_set_style_pad_all(h->cont, 0, 0);
    lv_obj_set_style_bg_opa(h->cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(h->cont, 0, 0);
    lv_obj_clear_flag(h->cont, LV_OBJ_FLAG_SCROLLABLE);

    if (h->mode == LV_LABEL_OUTLINE8_MODE_OPAQUE_FILL) {
#if LV_USE_CANVAS
        uint8_t opaque_canvas_ok = 0;

        /* canvas OPAQUE：单 canvas 绘制 8 向描边+填充，外圈均匀；旧 9 label 层 translate 易不均匀 */
#if OUTLINE8_OPAQUE_USE_CANVAS
        if (outline_off_req >= 1)
#else
        if (0)
#endif
        {
            lv_coord_t *seg_ws_c = (lv_coord_t *)malloc(segment_count * sizeof(lv_coord_t));
            lv_coord_t *seg_hs_c = (lv_coord_t *)malloc(segment_count * sizeof(lv_coord_t));

            if (seg_ws_c && seg_hs_c)
            {
                uint32_t s;
                lv_coord_t total_w = 0;
                lv_coord_t max_h = 0;
                lv_coord_t layout_h = 0;

                for (s = 0; s < segment_count; s++)
                {
                    segment_measure(parent, &h->segments[s], &seg_ws_c[s], &seg_hs_c[s]);
                    if (seg_ws_c[s] < 1)
                    {
                        seg_ws_c[s] = 1;
                    }
                    if (seg_hs_c[s] < 1)
                    {
                        seg_hs_c[s] = 20;
                    }
                }
                if (max_width > 0)
                {
                    lv_coord_t cur_x = 0;
                    lv_coord_t cur_y = 0;
                    lv_coord_t line_h = 0;

                    for (s = 0; s < segment_count; s++)
                    {
                        if (cur_x + seg_ws_c[s] > max_width && cur_x > 0)
                        {
                            cur_y += line_h;
                            cur_x = 0;
                            line_h = 0;
                        }
                        if (seg_hs_c[s] > line_h)
                        {
                            line_h = seg_hs_c[s];
                        }
                        cur_x += seg_ws_c[s];
                    }
                    layout_h = cur_y + line_h;
                    if (layout_h < 1)
                    {
                        layout_h = 40;
                    }
                    h->w = max_width + 2 * offset_px;
                    h->h = layout_h + 2 * offset_px;
                }
                else
                {
                    for (s = 0; s < segment_count; s++)
                    {
                        total_w += seg_ws_c[s];
                        if (seg_hs_c[s] > max_h)
                        {
                            max_h = seg_hs_c[s];
                        }
                    }
                    if (total_w < 1)
                    {
                        total_w = 200;
                    }
                    if (max_h < 1)
                    {
                        max_h = 40;
                    }
                    h->w = total_w + 2 * offset_px;
                    h->h = max_h + 2 * offset_px;
                }
                {
                    size_t buf_size = divoom_pixel_format_size(h->w, h->h, LV_COLOR_FORMAT_RGB565A8);

                    h->buf = malloc(buf_size);
                    if (h->buf)
                    {
                        h->canvas = lv_canvas_create(h->cont);
                        lv_canvas_set_buffer(h->canvas, h->buf, h->w, h->h, LV_COLOR_FORMAT_RGB565A8);
                        lv_obj_set_size(h->canvas, h->w, h->h);
                        lv_obj_set_pos(h->canvas, 0, 0);
                        if (use_typing && h->typing_total_chars > 0 && h->typing_segments_display)
                        {
                            h->segments = h->typing_segments_display;
                            outline8_canvas_draw_segments_opaque_fill(h);
                            h->segments = h->segments_own;
                        }
                        else
                        {
                            outline8_canvas_draw_segments_opaque_fill(h);
                        }
                        lv_obj_set_size(h->cont, h->w, h->h);
                        h->outline_labels = NULL;
                        h->main_labels = NULL;
                        h->main = NULL;
                        opaque_canvas_ok = 1;
                    }
                }
            }
            free(seg_ws_c);
            free(seg_hs_c);
        }
        if (!opaque_canvas_ok)
#endif
        outline8_wrapped_labels:
        {
        /* 8 向 label 换行布局：OPAQUE 填充 / TRANSPARENT 空心（中心字透明） */
        uint32_t num_outline = 8 * segment_count;
        h->outline_labels = (lv_obj_t **)calloc(num_outline, sizeof(lv_obj_t *));
        h->main_labels = (lv_obj_t **)calloc(segment_count, sizeof(lv_obj_t *));
        if (!h->outline_labels || !h->main_labels) {
            free(h->outline_labels);
            free(h->main_labels);
            lv_obj_del(h->cont);
            free(h);
            return NULL;
        }
        /* 先测量各段，得到 total_h，再创建 label 并垂直居中 */
        lv_coord_t *seg_ws = (lv_coord_t *)malloc(segment_count * sizeof(lv_coord_t));
        lv_coord_t *seg_hs = (lv_coord_t *)malloc(segment_count * sizeof(lv_coord_t));
        if (!seg_ws || !seg_hs) {
            free(seg_ws);
            free(seg_hs);
            free(h->outline_labels);
            free(h->main_labels);
            lv_obj_del(h->cont);
            free(h);
            return NULL;
        }
        lv_coord_t cur_x = 0, cur_y = 0, total_h = 0, line_h = 0;
        for (uint32_t s = 0; s < segment_count; s++) {
            segment_measure(parent, &h->segments[s], &seg_ws[s], &seg_hs[s]);
            if (seg_ws[s] < 1) seg_ws[s] = 1;
            if (seg_hs[s] < 1) seg_hs[s] = 20;
            if (seg_hs[s] > total_h) total_h = seg_hs[s];
        }
        lv_coord_t seg_w_max = (h->max_width > 0) ? h->max_width : 0;
        uint32_t line_start = 0;
        const lv_font_t *prev_font = NULL;
        for (uint32_t s = 0; s < segment_count; s++) {
            /* 换行判断：仅在一行放不下时换行；若本段以「非空格」标点开头（如中文「，一行…」）且当前行已有内容则也换行，避免英文仅因前导空格就换行 */
            lv_coord_t seg_w_for_wrap = seg_ws[s] + (lv_coord_t)SHADOW_SEG_WIDTH_MARGIN;
            int would_wrap = (seg_w_max > 0 && cur_x + seg_w_for_wrap > seg_w_max && cur_x > 0);
            const char *seg_txt = h->segments[s].text ? h->segments[s].text : "";
            if (cur_x > 0 && segment_starts_with_punct(seg_txt) && *seg_txt != ' ')
                would_wrap = 1;
            int font_changed = (s > 0 && prev_font != h->segments[s].font);
#if 0 /* 调试换行，量产关闭避免阻塞主线程 */
            divoom_print("[outline8] seg %u cur_x=%d seg_w=%d wrap_need=%d max=%d would_wrap=%d font_changed=%d txt=%.16s\n",
                (unsigned)s, (int)cur_x, (int)seg_ws[s], (int)seg_w_for_wrap, (int)seg_w_max,
                would_wrap, font_changed, h->segments[s].text ? h->segments[s].text : "");
#endif
            if (would_wrap) {
#if 0
                divoom_print("[outline8] wrap at seg %u cur_x=%d seg_w=%d max=%d line_start=%u font_changed=%d\n",
                    (unsigned)s, (int)cur_x, (int)seg_ws[s], (int)seg_w_max, (unsigned)line_start, font_changed);
#endif
                /* 换行前：上一行各 segment 垂直居中（上边下边留空） */
                for (uint32_t j = line_start; j < s; j++) {
                    lv_coord_t sy = cur_y + (line_h - seg_hs[j]) / 2;
                    for (int i = 0; i < 8; i++)
                        lv_obj_set_y(h->outline_labels[i * segment_count + j], sy);
                    lv_obj_set_y(h->main_labels[j], sy);
                }
                cur_x = 0;
                cur_y += line_h;
                line_start = s;
                line_h = 0;
            }
            const char *seg_init = (use_typing && h->typing_total_chars > 0) ? "" : (h->segments[s].text ? h->segments[s].text : "");
            /* 每段宽度：与 slash 一致用测量值+余量（SLASH_WIDTH_EXTRA 思路），不设 LONG_CLIP */
            lv_coord_t seg_w_display = seg_ws[s] + (lv_coord_t)SHADOW_SEG_WIDTH_MARGIN + 2 * offset_px;
            if (seg_w_display < seg_ws[s]) seg_w_display = seg_ws[s];
            lv_coord_t seg_y = (seg_w_max > 0) ? cur_y : (total_h - seg_hs[s]) / 2;
            for (int i = 0; i < 8; i++) {
                lv_obj_t *out = lv_label_create(h->cont);
                lv_label_set_text(out, seg_init);
                lv_obj_set_width(out, seg_w_display);
                if (seg_w_max > 0)
                {
                    /* 空心描边：outline 层禁止 WRAP+translate，否则长句会竖条花屏 */
                    if (h->mode == LV_LABEL_OUTLINE8_MODE_TRANSPARENT_CENTER)
                    {
                        lv_label_set_long_mode(out, LV_LABEL_LONG_CLIP);
                    }
                    else
                    {
                        lv_label_set_long_mode(out, LV_LABEL_LONG_WRAP);
                    }
                }
                lv_obj_set_style_text_color(out, outline_color, 0);
                lv_obj_set_style_text_opa(out, outline_opa, 0);
                if (h->segments[s].font) lv_obj_set_style_text_font(out, h->segments[s].font, 0);
                lv_obj_set_style_translate_x(out, (lv_coord_t)(outline8_dx[i] * offset_px), 0);
                lv_obj_set_style_translate_y(out, (lv_coord_t)(outline8_dy[i] * offset_px), 0);
                lv_obj_set_pos(out, cur_x, seg_y);
                h->outline_labels[i * segment_count + s] = out;
            }
            lv_obj_t *main_lab = lv_label_create(h->cont);
            lv_label_set_text(main_lab, seg_init);
            lv_obj_set_width(main_lab, seg_w_display);
            if (seg_w_max > 0) lv_label_set_long_mode(main_lab, LV_LABEL_LONG_WRAP);
            lv_obj_set_style_text_color(main_lab, fill_color, 0);
            if (h->mode == LV_LABEL_OUTLINE8_MODE_TRANSPARENT_CENTER)
            {
                lv_obj_set_style_text_opa(main_lab, LV_OPA_TRANSP, 0);
            }
            else
            {
                lv_obj_set_style_text_opa(main_lab, LV_OPA_COVER, 0);
            }
            if (h->segments[s].font) lv_obj_set_style_text_font(main_lab, h->segments[s].font, 0);
            lv_obj_set_pos(main_lab, cur_x, seg_y);
            h->main_labels[s] = main_lab;
            if (segment_count == 1) h->main = main_lab;
            cur_x += seg_ws[s];
            if (seg_hs[s] > line_h) line_h = seg_hs[s];
            prev_font = h->segments[s].font;
        }
        /* 最后一行：垂直居中 */
        if (seg_w_max > 0 && line_h > 0) {
            for (uint32_t j = line_start; j < segment_count; j++) {
                lv_coord_t sy = cur_y + (line_h - seg_hs[j]) / 2;
                for (int i = 0; i < 8; i++)
                    lv_obj_set_y(h->outline_labels[i * segment_count + j], sy);
                lv_obj_set_y(h->main_labels[j], sy);
            }
        }
        free(seg_ws);
        free(seg_hs);
        /* 容器宽取 max(seg_w_max, cur_x)，因不裁切时实际总宽可能略大于 seg_w_max */
        lv_coord_t total_w = (seg_w_max > 0 && cur_x > seg_w_max) ? cur_x : (seg_w_max > 0 ? seg_w_max : cur_x);
        lv_coord_t cw = total_w + 2 * offset_px;
        lv_coord_t ch = (seg_w_max > 0) ? (cur_y + line_h + 2 * offset_px) : ((total_h > 0 ? total_h : 40) + 2 * offset_px);
        lv_obj_set_size(h->cont, cw, ch);
        h->canvas = NULL;
        h->buf = NULL;
#if LV_USE_CANVAS
        }
#endif
    } else {
        goto outline8_wrapped_labels;
    }
    if (use_typing && h->typing_total_chars > 0) {
        h->typing_start_tick = lv_tick_get();
        h->typing_timer = lv_timer_create(outline8_typing_timer_cb, 40, h);
    }
    lv_obj_add_flag(h->cont, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    return h;
}

/** 设置 8 方向描边 label 的文本（仅更新第一段，多段时请用 set_segments） */
void lv_label_with_outline8_set_text(void *h_OBJ, const char *text)
{
    lv_label_outline8_handle_t *h = (lv_label_outline8_handle_t *)h_OBJ;
    if (!h || !h->cont) return;
    const char *str = text ? text : "";
#if LV_USE_CANVAS
    if (h->canvas && h->segments && h->segment_count > 0) {
        /* 仅第一段更新时无法单独重绘，需调用方用 set_segments 更新整段 */
        (void)str;
        return;
    }
#endif
    if (h->main_labels && h->segment_count > 0 && h->main_labels[0])
        lv_label_set_text(h->main_labels[0], str);
    if (h->outline_labels && h->segment_count > 0) {
        for (int i = 0; i < 8; i++) {
            if (h->outline_labels[i * (int)h->segment_count])
                lv_label_set_text(h->outline_labels[i * (int)h->segment_count], str);
        }
    }
    if (h->main) lv_label_set_text(h->main, str);
}

/** 按段设置 8 方向描边 label（多段字串/字体）；canvas 模式会重绘 */
void lv_label_with_outline8_set_segments(void *h_OBJ,
                                          const lv_label_segment_t *segments,
                                          uint32_t segment_count)
{
    lv_label_outline8_handle_t *h = (lv_label_outline8_handle_t *)h_OBJ;
    if (!h || !h->cont || !segments || segment_count == 0) return;
    uint32_t n = segment_count <= h->segment_count ? segment_count : h->segment_count;
    if (h->segments_own_buf && h->segments_own) {
        for (uint32_t s = 0; s < n; s++) {
            const char *t = segments[s].text ? segments[s].text : "";
            size_t len = 0;
            while (len < TYPING_BUF_SIZE - 1 && t[len]) len++;
            char *dst = h->segments_own_buf + s * TYPING_BUF_SIZE;
            memcpy(dst, t, len);
            dst[len] = '\0';
            h->segments_own[s].font = segments[s].font;
            h->segments_own[s].text = dst;
        }
    }
#if LV_USE_CANVAS
    if (h->canvas) {
        outline8_canvas_redraw(h);
        return;
    }
#endif
    for (uint32_t s = 0; s < n; s++) {
        const char *str = segments[s].text ? segments[s].text : "";
        const lv_font_t *font = segments[s].font;
        if (h->main_labels && h->main_labels[s]) {
            lv_label_set_text(h->main_labels[s], str);
            if (font) lv_obj_set_style_text_font(h->main_labels[s], font, 0);
        }
        if (h->outline_labels) {
            for (int i = 0; i < 8; i++) {
                lv_obj_t *o = h->outline_labels[i * (int)h->segment_count + s];
                if (o) {
                    lv_label_set_text(o, str);
                    if (font) lv_obj_set_style_text_font(o, font, 0);
                }
            }
        }
    }
}

/** 获取 8 方向描边 label 实际使用的高度（换行后总高度）；创建后或 set_segments 后调用前建议先 lv_obj_update_layout(cont) */
lv_coord_t lv_label_with_outline8_get_height(void *h_OBJ)
{
    lv_label_outline8_handle_t *oh = (lv_label_outline8_handle_t *)h_OBJ;
    if (!oh || !oh->cont) return 0;
    lv_obj_update_layout(oh->cont);
    return lv_obj_get_height(oh->cont);
}

/** 释放 8 方向描边 label */
void lv_label_with_outline8_delete(void *h_OBJ)
{
    lv_label_outline8_handle_t *h = (lv_label_outline8_handle_t *)h_OBJ;
    if (!h) return;
    if (h->typing_timer) { lv_timer_del(h->typing_timer); h->typing_timer = NULL; }
    if (h->cont) lv_obj_del(h->cont);
    if (h->buf) free(h->buf);
    free(h->outline_labels);
    free(h->main_labels);
    free(h->segments_own_buf);
    free(h->segments_own);
    free(h->typing_segment_chars);
    free(h->typing_buf);
    free(h->typing_segments_display);
    free(h);
}

/* ========== 描边 + 柔光（canvas 圆周密集采样：连续包边，非 8 向 label 描边） ========== */
#define STROKE_GLOW_CANVAS_MAX_SEG  32
#define STROKE_GLOW_ANGLE_STEP      12   /* 每圈 30 点，外圈无断点 */
#define STROKE_GLOW_MAX_CANVAS_PIXELS  220000u

typedef struct {
    lv_obj_t *cont;
    lv_obj_t *main;
    lv_obj_t *canvas;
    void *buf;
    lv_coord_t w;
    lv_coord_t h;
    lv_coord_t pad;
    lv_coord_t max_width;
    lv_coord_t stroke_px;
    lv_color_t stroke_color;
    lv_opa_t stroke_opa;
    lv_color_t fill_color;
    lv_opa_t fill_opa;
    lv_coord_t glow_max_px;
    lv_color_t glow_color;
    lv_opa_t glow_opa;
    uint32_t segment_count;
    const lv_label_segment_t *segments;
    char *segments_own_buf;
    lv_label_segment_t *segments_own;
    uint32_t typing_duration_ms;
    uint32_t typing_total_chars;
    uint32_t *typing_segment_chars;
    lv_timer_t *typing_timer;
    uint32_t typing_start_tick;
    char *typing_buf;
    lv_label_segment_t *typing_segments_display;
} lv_label_stroke_glow_handle_t;

#if LV_USE_CANVAS
static void stroke_glow_canvas_draw_ring_text(lv_obj_t *canvas,
    lv_coord_t base_x, lv_coord_t base_y, lv_coord_t text_max_w,
    const char *str, const lv_font_t *font,
    lv_color_t color, lv_opa_t opa, lv_coord_t radius_px, int16_t angle_step_deg)
{
    lv_draw_label_dsc_t label_dsc;
    int16_t deg;

    if (!canvas || !str || radius_px < 1)
    {
        return;
    }
    if (angle_step_deg < 1)
    {
        angle_step_deg = 1;
    }
    lv_draw_label_dsc_init(&label_dsc);
    label_dsc.align = LV_TEXT_ALIGN_LEFT;
    label_dsc.color = color;
    label_dsc.opa = opa;
    label_dsc.font = font ? font : lv_font_default();
    for (deg = 0; deg < 360; deg = (int16_t)(deg + angle_step_deg))
    {
        int32_t sin_a = (int32_t)lv_trigo_sin(deg);
        int32_t cos_a = (int32_t)lv_trigo_sin((int16_t)(deg + 90));
        lv_coord_t px = base_x + (lv_coord_t)((radius_px * cos_a) >> LV_TRIGO_SHIFT);
        lv_coord_t py = base_y + (lv_coord_t)((radius_px * sin_a) >> LV_TRIGO_SHIFT);

        lv_canvas_draw_text(canvas, px, py, text_max_w, &label_dsc, str);
    }
}

static void stroke_glow_canvas_draw_disk_text(lv_obj_t *canvas,
    lv_coord_t base_x, lv_coord_t base_y, lv_coord_t text_max_w,
    const char *str, const lv_font_t *font,
    lv_color_t color, lv_opa_t opa, lv_coord_t radius_px, int16_t angle_step_deg)
{
    lv_coord_t r;

    for (r = 1; r <= radius_px; r++)
    {
        stroke_glow_canvas_draw_ring_text(canvas, base_x, base_y, text_max_w,
            str, font, color, opa, r, angle_step_deg);
    }
}

static void stroke_glow_segment_size_wrapped(lv_obj_t *parent,
    const lv_label_segment_t *seg, lv_coord_t max_width,
    lv_coord_t *out_w, lv_coord_t *out_h);

static void stroke_glow_canvas_draw_all(lv_label_stroke_glow_handle_t *h,
    const lv_label_segment_t *segments, uint32_t segment_count, lv_coord_t max_width,
    lv_coord_t stroke_px, lv_color_t stroke_color, lv_opa_t stroke_opa,
    lv_color_t fill_color, lv_opa_t fill_opa, lv_coord_t glow_max_px, lv_color_t glow_color, lv_opa_t glow_opa)
{
    lv_obj_t *parent;
    lv_coord_t seg_ws[STROKE_GLOW_CANVAS_MAX_SEG];
    lv_coord_t seg_hs[STROKE_GLOW_CANVAS_MAX_SEG];
    lv_coord_t seg_x[STROKE_GLOW_CANVAS_MAX_SEG];
    lv_coord_t seg_y[STROKE_GLOW_CANVAS_MAX_SEG];
    uint32_t n;
    uint32_t s;
    uint32_t j;
    lv_coord_t pad;
    lv_coord_t seg_w_max;
    lv_coord_t cur_x;
    lv_coord_t cur_y;
    lv_coord_t line_h;
    uint32_t line_start;
    size_t buf_size;
    lv_draw_label_dsc_t label_dsc;
    lv_coord_t r;
    lv_coord_t glow_span;

    if (!h || !h->canvas || !h->buf || !segments || segment_count == 0)
    {
        return;
    }
    n = segment_count <= STROKE_GLOW_CANVAS_MAX_SEG ? segment_count : STROKE_GLOW_CANVAS_MAX_SEG;
    parent = lv_obj_get_parent(h->canvas);
    pad = h->pad;
    seg_w_max = (max_width > 0) ? (max_width + pad) : 0;

    for (s = 0; s < n; s++)
    {
        stroke_glow_segment_size_wrapped(parent, &segments[s], max_width, &seg_ws[s], &seg_hs[s]);
    }

    cur_x = pad;
    cur_y = pad;
    line_h = 0;
    line_start = 0;
    for (s = 0; s < n; s++)
    {
        if (seg_w_max > 0 && cur_x + seg_ws[s] > seg_w_max && cur_x > pad)
        {
            for (j = line_start; j < s; j++)
            {
                seg_y[j] = cur_y + (line_h - seg_hs[j]) / 2;
            }
            cur_y += line_h;
            cur_x = pad;
            line_start = s;
            line_h = 0;
        }
        seg_x[s] = cur_x;
        seg_y[s] = cur_y;
        cur_x += seg_ws[s];
        if (seg_hs[s] > line_h)
        {
            line_h = seg_hs[s];
        }
    }
    for (j = line_start; j < n; j++)
    {
        seg_y[j] = cur_y + (line_h - seg_hs[j]) / 2;
    }

    buf_size = divoom_pixel_format_size(h->w, h->h, LV_COLOR_FORMAT_RGB565A8);
    memset(h->buf, 0, buf_size);

    glow_span = glow_max_px - stroke_px;
    if (glow_opa > 0 && glow_span > 0)
    {
        for (r = stroke_px + 1; r <= glow_max_px; r++)
        {
            lv_opa_t ring_opa = (lv_opa_t)((uint32_t)glow_opa * (glow_max_px - r + 1) / glow_span);

            for (s = 0; s < n; s++)
            {
                const char *str = segments[s].text ? segments[s].text : "";
                lv_coord_t text_max_w = h->w - seg_x[s] - pad;

                if (text_max_w < 1)
                {
                    break;
                }
                stroke_glow_canvas_draw_ring_text(h->canvas, seg_x[s], seg_y[s], text_max_w,
                    str, segments[s].font, glow_color, ring_opa, r, STROKE_GLOW_ANGLE_STEP);
            }
        }
    }

    for (s = 0; s < n; s++)
    {
        const char *str = segments[s].text ? segments[s].text : "";
        lv_coord_t text_max_w = h->w - seg_x[s] - pad;

        if (text_max_w < 1)
        {
            break;
        }
        stroke_glow_canvas_draw_disk_text(h->canvas, seg_x[s], seg_y[s], text_max_w,
            str, segments[s].font, stroke_color, stroke_opa, stroke_px, STROKE_GLOW_ANGLE_STEP);
    }

    lv_draw_label_dsc_init(&label_dsc);
    label_dsc.align = LV_TEXT_ALIGN_LEFT;
    if (fill_opa > 0)
    {
        label_dsc.color = fill_color;
        label_dsc.opa = fill_opa;
        for (s = 0; s < n; s++)
        {
            const char *str = segments[s].text ? segments[s].text : "";
            lv_coord_t text_max_w = h->w - seg_x[s] - pad;

            label_dsc.font = segments[s].font ? segments[s].font : lv_font_default();
            if (text_max_w < 1)
            {
                break;
            }
            lv_canvas_draw_text(h->canvas, seg_x[s], seg_y[s], text_max_w, &label_dsc, str);
        }
    }
    lv_obj_invalidate(h->canvas);
}

/**
 * 测量 segment 在 max_width 内换行后的实际 bbox。
 *  - max_width <= 0：返回原始单行宽高
 *  - max_width >  0：先取原始单行宽 (raw_w)，若 raw_w > max_width 则用 lv_txt_get_size 按
 *    max_width 自动换行算出多行高度；out_w 上限到 max_width，避免后续 cur_x 累加溢出。
 *  与 lv_canvas_draw_text(canvas, x, y, max_width, ..., str) 的换行规则一致，
 *  保证 measure 的高度 == 实际渲染时该段占用的高度（含多行），不再出现行重叠。
 */
static void stroke_glow_segment_size_wrapped(lv_obj_t *parent,
    const lv_label_segment_t *seg, lv_coord_t max_width,
    lv_coord_t *out_w, lv_coord_t *out_h)
{
    const char *str;
    const lv_font_t *font;
    lv_point_t pt;

    if (!seg || !out_w || !out_h)
    {
        if (out_w) *out_w = 1;
        if (out_h) *out_h = 20;
        return;
    }
    segment_measure(parent, seg, out_w, out_h);
    if (*out_w < 1) *out_w = 1;
    if (*out_h < 1) *out_h = 20;
    str = seg->text ? seg->text : "";
    font = seg->font ? seg->font : lv_font_default();
    if (max_width > 0 && str[0] && *out_w > max_width)
    {
        lv_txt_get_size(&pt, str, font, 0, 0, max_width, 0);
        if (pt.y > *out_h) *out_h = pt.y;
        *out_w = max_width;
    }
}

static uint8_t stroke_glow_canvas_measure(const lv_label_segment_t *segments, uint32_t segment_count,
    lv_obj_t *parent, lv_coord_t max_width, lv_coord_t pad,
    lv_coord_t *out_w, lv_coord_t *out_h)
{
    lv_coord_t seg_ws[STROKE_GLOW_CANVAS_MAX_SEG];
    lv_coord_t seg_hs[STROKE_GLOW_CANVAS_MAX_SEG];
    uint32_t n;
    uint32_t s;
    lv_coord_t total_w;
    lv_coord_t max_h;
    lv_coord_t layout_h;
    lv_coord_t cur_x;
    lv_coord_t cur_y;
    lv_coord_t line_h;

    if (!segments || segment_count == 0 || !out_w || !out_h)
    {
        return 0;
    }
    n = segment_count <= STROKE_GLOW_CANVAS_MAX_SEG ? segment_count : STROKE_GLOW_CANVAS_MAX_SEG;
    for (s = 0; s < n; s++)
    {
        stroke_glow_segment_size_wrapped(parent, &segments[s], max_width, &seg_ws[s], &seg_hs[s]);
    }
    if (max_width > 0)
    {
        cur_x = 0;
        cur_y = 0;
        line_h = 0;
        for (s = 0; s < n; s++)
        {
            if (cur_x + seg_ws[s] > max_width && cur_x > 0)
            {
                cur_y += line_h;
                cur_x = 0;
                line_h = 0;
            }
            if (seg_hs[s] > line_h)
            {
                line_h = seg_hs[s];
            }
            cur_x += seg_ws[s];
        }
        layout_h = cur_y + line_h;
        if (layout_h < 1)
        {
            layout_h = 40;
        }
        *out_w = max_width + 2 * pad;
        *out_h = layout_h + 2 * pad;
    }
    else
    {
        total_w = 0;
        max_h = 0;
        for (s = 0; s < n; s++)
        {
            total_w += seg_ws[s];
            if (seg_hs[s] > max_h)
            {
                max_h = seg_hs[s];
            }
        }
        if (total_w < 1)
        {
            total_w = 200;
        }
        if (max_h < 1)
        {
            max_h = 40;
        }
        *out_w = total_w + 2 * pad;
        *out_h = max_h + 2 * pad;
    }
    return 1;
}

#endif /* LV_USE_CANVAS */

#if LV_USE_CANVAS
static void stroke_glow_typing_timer_cb(lv_timer_t *tmr)
{
    lv_label_stroke_glow_handle_t *h = (lv_label_stroke_glow_handle_t *)lv_timer_get_user_data(tmr);
    uint32_t elapsed;
    uint32_t duration;
    uint32_t total;
    uint32_t current_chars;
    uint32_t prefix;
    uint32_t i;
    uint32_t seg_chars;
    uint32_t show;
    const char *src;

    if (!h || !h->cont || !h->typing_segment_chars || !h->segments_own || !h->typing_buf || !h->typing_segments_display)
    {
        return;
    }
    elapsed = lv_tick_elaps(h->typing_start_tick);
    duration = h->typing_duration_ms;
    total = h->typing_total_chars;
    if (total == 0)
    {
        lv_timer_del(tmr);
        h->typing_timer = NULL;
        return;
    }
    current_chars = (elapsed >= duration) ? total : (uint32_t)((uint64_t)elapsed * (uint64_t)total / (uint64_t)duration);
    if (current_chars > total)
    {
        current_chars = total;
    }
    prefix = 0;
    for (i = 0; i < h->segment_count; i++)
    {
        seg_chars = h->typing_segment_chars[i];
        show = 0;
        if (current_chars > prefix)
        {
            show = (current_chars - prefix) < seg_chars ? (current_chars - prefix) : seg_chars;
        }
        src = h->segments_own[i].text ? h->segments_own[i].text : "";
        typing_utf8_prefix(h->typing_buf + i * TYPING_BUF_SIZE, TYPING_BUF_SIZE, src, show);
        prefix += seg_chars;
    }
    stroke_glow_canvas_draw_all(h, h->typing_segments_display, h->segment_count, h->max_width,
        h->stroke_px, h->stroke_color, h->stroke_opa, h->fill_color, h->fill_opa,
        h->glow_max_px, h->glow_color, h->glow_opa);
    lv_obj_invalidate(h->canvas);
    if (current_chars >= total)
    {
        lv_timer_del(tmr);
        h->typing_timer = NULL;
    }
}
#endif /* LV_USE_CANVAS */

void *lv_label_create_with_stroke_glow(lv_obj_t *parent,
    const lv_label_segment_t *segments,
    uint32_t segment_count,
    lv_coord_t max_width,
    lv_coord_t stroke_px,
    lv_color_t stroke_color,
    lv_opa_t stroke_opa,
    lv_color_t fill_color,
    lv_opa_t fill_opa,
    lv_coord_t glow_max_px,
    lv_color_t glow_color,
    lv_opa_t glow_opa,
    uint32_t typing_duration_ms)
{
    lv_label_stroke_glow_handle_t *h;
    lv_coord_t pad;
    uint8_t hollow_only;
    uint8_t use_typing;
    const lv_label_segment_t *draw_segments;

#if LV_USE_CANVAS
    size_t buf_size;
    uint32_t s;
#endif

    if (!parent || !segments || segment_count == 0)
    {
        return NULL;
    }
    if (stroke_px < 1)
    {
        stroke_px = 1;
    }
    if (glow_max_px < stroke_px)
    {
        glow_max_px = stroke_px;
    }
    hollow_only = (fill_opa == 0) ? 1 : 0;
    pad = glow_max_px + 2;
    use_typing = (typing_duration_ms > 0) ? 1 : 0;
    draw_segments = segments;

    h = (lv_label_stroke_glow_handle_t *)divoom_system_malloc(sizeof(lv_label_stroke_glow_handle_t));
    if (!h)
    {
        return NULL;
    }
    memset(h, 0, sizeof(*h));
    h->pad = pad;
    h->max_width = max_width;
    h->stroke_px = stroke_px;
    h->stroke_color = stroke_color;
    h->stroke_opa = stroke_opa;
    h->fill_color = fill_color;
    h->fill_opa = fill_opa;
    h->glow_max_px = glow_max_px;
    h->glow_color = glow_color;
    h->glow_opa = glow_opa;
    h->segment_count = segment_count;
    h->typing_duration_ms = typing_duration_ms;

    h->cont = lv_obj_create(parent);
    lv_obj_set_style_pad_all(h->cont, 0, 0);
    lv_obj_set_style_bg_opa(h->cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(h->cont, 0, 0);
    lv_obj_clear_flag(h->cont, LV_OBJ_FLAG_SCROLLABLE);

#if LV_USE_CANVAS
    if (!stroke_glow_canvas_measure(segments, segment_count, parent, max_width, pad, &h->w, &h->h))
    {
        lv_obj_del(h->cont);
        divoom_system_free(h);
        return NULL;
    }
    if ((size_t)h->w * (size_t)h->h > STROKE_GLOW_MAX_CANVAS_PIXELS)
    {
        divoom_print("[stroke_glow] fallback outline8 oversize w=%d h=%d px=%u fill_opa=%u fill=#%06X stroke=#%06X\n",
            (int)h->w, (int)h->h, (unsigned)stroke_px, (unsigned)fill_opa,
            (unsigned)(lv_color_to32(fill_color) & 0xFFFFFFU),
            (unsigned)(lv_color_to32(stroke_color) & 0xFFFFFFU));
        lv_obj_del(h->cont);
        divoom_system_free(h);
        return lv_label_create_with_outline8(parent, segments, segment_count, max_width,
            stroke_px, stroke_color, stroke_opa,
            hollow_only ? LV_LABEL_OUTLINE8_MODE_TRANSPARENT_CENTER : LV_LABEL_OUTLINE8_MODE_OPAQUE_FILL,
            fill_color, typing_duration_ms, NULL);
    }
    buf_size = divoom_pixel_format_size(h->w, h->h, LV_COLOR_FORMAT_RGB565A8);
    h->buf = divoom_system_malloc(buf_size);
    if (!h->buf)
    {
        divoom_print("[stroke_glow] fallback outline8 malloc fail bytes=%u\n", (unsigned)buf_size);
        lv_obj_del(h->cont);
        divoom_system_free(h);
        return lv_label_create_with_outline8(parent, segments, segment_count, max_width,
            stroke_px, stroke_color, stroke_opa,
            hollow_only ? LV_LABEL_OUTLINE8_MODE_TRANSPARENT_CENTER : LV_LABEL_OUTLINE8_MODE_OPAQUE_FILL,
            fill_color, typing_duration_ms, NULL);
    }
    h->canvas = lv_canvas_create(h->cont);
    lv_canvas_set_buffer(h->canvas, h->buf, h->w, h->h, LV_COLOR_FORMAT_RGB565A8);
    lv_obj_set_size(h->canvas, h->w, h->h);
    lv_obj_set_pos(h->canvas, 0, 0);
    lv_obj_set_style_img_opa(h->canvas, LV_OPA_COVER, 0);
    if (use_typing)
    {
        h->segments_own_buf = (char *)divoom_system_malloc((size_t)segment_count * TYPING_BUF_SIZE);
        h->segments_own = (lv_label_segment_t *)divoom_system_malloc(segment_count * sizeof(lv_label_segment_t));
        h->typing_segment_chars = (uint32_t *)divoom_system_malloc(segment_count * sizeof(uint32_t));
        h->typing_buf = (char *)divoom_system_malloc((size_t)segment_count * TYPING_BUF_SIZE);
        h->typing_segments_display = (lv_label_segment_t *)divoom_system_malloc(segment_count * sizeof(lv_label_segment_t));
        if (!h->segments_own_buf || !h->segments_own || !h->typing_segment_chars || !h->typing_buf || !h->typing_segments_display)
        {
            divoom_system_free(h->segments_own_buf);
            divoom_system_free(h->segments_own);
            divoom_system_free(h->typing_segment_chars);
            divoom_system_free(h->typing_buf);
            divoom_system_free(h->typing_segments_display);
            divoom_system_free(h->buf);
            lv_obj_del(h->cont);
            divoom_system_free(h);
            return lv_label_create_with_outline8(parent, segments, segment_count, max_width,
                stroke_px, stroke_color, stroke_opa,
                hollow_only ? LV_LABEL_OUTLINE8_MODE_TRANSPARENT_CENTER : LV_LABEL_OUTLINE8_MODE_OPAQUE_FILL,
                fill_color, typing_duration_ms, NULL);
        }
        h->typing_total_chars = 0;
        for (s = 0; s < segment_count; s++)
        {
            const char *t = segments[s].text ? segments[s].text : "";
            size_t len = strlen(t);
            char *dst;

            if (len >= TYPING_BUF_SIZE)
            {
                len = TYPING_BUF_SIZE - 1;
                while (len > 0 && ((unsigned char)t[len - 1] & 0xC0) == 0x80)
                {
                    len--;
                }
            }
            dst = h->segments_own_buf + s * TYPING_BUF_SIZE;
            memcpy(dst, t, len);
            dst[len] = '\0';
            h->segments_own[s].font = segments[s].font;
            h->segments_own[s].text = dst;
            h->typing_segment_chars[s] = typing_utf8_char_count(t);
            h->typing_total_chars += h->typing_segment_chars[s];
            h->typing_segments_display[s].font = segments[s].font;
            h->typing_segments_display[s].text = h->typing_buf + s * TYPING_BUF_SIZE;
            h->typing_buf[s * TYPING_BUF_SIZE] = '\0';
        }
        h->segments = h->segments_own;
        draw_segments = h->typing_segments_display;
    }
    else
    {
        h->segments = segments;
    }
    {
        int t0 = divoom_system_get_tickms();
        int draw_ms;

        stroke_glow_canvas_draw_all(h, draw_segments, segment_count, max_width,
            stroke_px, stroke_color, stroke_opa, fill_color, fill_opa, glow_max_px, glow_color, glow_opa);
        draw_ms = divoom_system_get_tickms() - t0;
        divoom_print("[stroke_glow] canvas w=%d h=%d seg=%u buf=%u draw_ms=%d fill_opa=%u fill=#%06X stroke=#%06X typing=%u\n",
            (int)h->w, (int)h->h, (unsigned)segment_count, (unsigned)buf_size,
            draw_ms, (unsigned)fill_opa,
            (unsigned)(lv_color_to32(fill_color) & 0xFFFFFFU),
            (unsigned)(lv_color_to32(stroke_color) & 0xFFFFFFU),
            (unsigned)typing_duration_ms);
        if (draw_ms > 100)
        {
            divoom_print("[stroke_glow] SLOW draw_ms=%d (may block lyric)\n", draw_ms);
        }
    }
    lv_obj_set_size(h->cont, h->w, h->h);
    h->main = NULL;
    if (use_typing && h->typing_total_chars > 0)
    {
        h->typing_start_tick = lv_tick_get();
        h->typing_timer = lv_timer_create(stroke_glow_typing_timer_cb, 40, h);
    }
#else
    {
        lv_obj_del(h->cont);
        divoom_system_free(h);
        return lv_label_create_with_outline8(parent, segments, segment_count, max_width,
            stroke_px, stroke_color, stroke_opa,
            hollow_only ? LV_LABEL_OUTLINE8_MODE_TRANSPARENT_CENTER : LV_LABEL_OUTLINE8_MODE_OPAQUE_FILL,
            fill_color, typing_duration_ms, NULL);
    }
#endif

    lv_obj_add_flag(h->cont, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    return h;
}

lv_coord_t lv_label_with_stroke_glow_get_height(void *h_obj)
{
    lv_label_stroke_glow_handle_t *h = (lv_label_stroke_glow_handle_t *)h_obj;

    if (!h || !h->cont)
    {
        return 0;
    }
    lv_obj_update_layout(h->cont);
    return lv_obj_get_height(h->cont);
}

void lv_label_with_stroke_glow_delete(void *h_obj)
{
    lv_label_stroke_glow_handle_t *h = (lv_label_stroke_glow_handle_t *)h_obj;

    if (!h)
    {
        return;
    }
    if (h->typing_timer)
    {
        lv_timer_del(h->typing_timer);
        h->typing_timer = NULL;
    }
    if (h->buf)
    {
        divoom_system_free(h->buf);
        h->buf = NULL;
    }
    divoom_system_free(h->segments_own_buf);
    divoom_system_free(h->segments_own);
    divoom_system_free(h->typing_segment_chars);
    divoom_system_free(h->typing_buf);
    divoom_system_free(h->typing_segments_display);
    if (h->cont)
    {
        lv_obj_del(h->cont);
        h->cont = NULL;
        h->canvas = NULL;
    }
    divoom_system_free(h);
}

/* ========== 每字独立旋转飞入、最终排成一行（立体描边 outline8） ========== */
#define SCATTER_CHAR_SLOT 5  /* 每字最多 4 字节 UTF-8 + \0 */

/** 每字起始状态：中心坐标 + 旋转。动画为 (start_x,start_y,start_angle) -> (end_x,end_y,0) */
typedef struct {
    lv_coord_t x;       /* 起始左上角 x（与 lv_obj_set_pos 一致） */
    lv_coord_t y;       /* 起始左上角 y */
    int32_t angle_01;   /* 起始旋转，0.1 度单位，如 200=20° */
} lv_scatter_char_start_t;

typedef struct {
    lv_obj_t *cont;
    lv_coord_t start_x, start_y, end_x, end_y;
    int32_t start_angle;  /* 0.1 度单位，最终为 0 */
} scatter_fly_ctx_t;

typedef struct {
    uint32_t num_chars;
    void **outline8_handles;  /* lv_label_outline8_handle_t * */
    char *char_buf;          /* 每字一槽 SCATTER_CHAR_SLOT 字节 */
    scatter_fly_ctx_t *ctx;
    lv_coord_t total_w;      /* 整句单行总宽度（不换行时的宽度），用于调试 */
} scatter_fly_handle_t;

static void scatter_fly_anim_exec(void *var, int32_t v)
{
    scatter_fly_ctx_t *c = (scatter_fly_ctx_t *)var;
    if (!c || !c->cont) return;
    /* v: 0..1000 */
    lv_coord_t x = (lv_coord_t)(c->start_x + ((int32_t)(c->end_x - c->start_x) * v) / 1000);
    lv_coord_t y = (lv_coord_t)(c->start_y + ((int32_t)(c->end_y - c->start_y) * v) / 1000);
    int32_t angle = c->start_angle + (int32_t)((0 - c->start_angle) * v) / 1000;
    lv_obj_set_pos(c->cont, x, y);
    lv_obj_set_style_transform_angle(c->cont, (lv_coord_t)angle, 0);
}

static void scatter_fly_anim_ready(lv_anim_t *a)
{
    scatter_fly_ctx_t *c = (scatter_fly_ctx_t *)a->user_data;
    if (c && c->cont) {
        lv_obj_set_pos(c->cont, c->end_x, c->end_y);
        lv_obj_set_style_transform_angle(c->cont, 0, 0);
    }
}

/**
 * 创建「每字独立从不同位置带旋转飞入，最终排成一行」效果，使用 lv_label_create_with_outline8 立体描边。
 * 文本与字体由 lv_label_segment_t 数组提供（与 lv_label_create_with_outline8 一致），可对不同段使用不同字号。
 * 动画：每字的 (中心坐标 + 旋转) 从起始变到终点一行；起始由 starts 或圆弧参数决定。
 *
 * @param segments 段数组，每段 .text + .font；按段顺序拼接成整句，每字飞入时使用该字所属段的字体
 * @param segment_count 段数量
 * @param max_width 最大行宽(px)；>0 时超宽自动换行（按断字字符，英文等单词不拆开）；0 不限制，单行
 * @param starts 每字起始 (x,y,angle_01)，共 n 个；NULL 则用下方 arc_* 算圆弧。非 NULL 时每字旋转角度从 starts[i].angle_01 取。
 * @param arc_rotate_angle_01 圆弧模式时每字旋转角度(0.1 度)，左负右正；仅 starts==NULL 有效，如 900 表示 90°
 * @param arc_radius, arc_center_deg, arc_spread_deg 当 starts==NULL 时有效
 * @return 句柄，失败 NULL；用 lv_label_outline8_scatter_fly_in_delete 释放
 */
void *lv_label_outline8_scatter_fly_in_create(
    lv_obj_t *parent,
    const lv_label_segment_t *segments,
    uint32_t segment_count,
    lv_coord_t max_width,
    lv_coord_t offset_px,
    lv_color_t outline_color,
    lv_opa_t outline_opa,
    uint8_t mode,
    lv_color_t fill_color,
    lv_coord_t row_x,
    lv_coord_t row_y,
    uint32_t duration_ms,
    uint32_t delay_per_char_ms,
    const lv_scatter_char_start_t *starts,
    int32_t arc_rotate_angle_01,
    lv_coord_t arc_radius,
    int16_t arc_center_deg,
    uint16_t arc_spread_deg)
{
    if (!parent || !segments || segment_count == 0) return NULL;

    /* 统计总字符数，并按段展开为 (char, font) 序列 */
    uint32_t n = 0;
    for (uint32_t s = 0; s < segment_count; s++) {
        const char *t = segments[s].text ? segments[s].text : "";
        n += _lv_txt_get_encoded_length(t);
    }
    if (n == 0) return NULL;

    scatter_fly_handle_t *sh = (scatter_fly_handle_t *)malloc(sizeof(scatter_fly_handle_t));
    if (!sh) return NULL;
    memset(sh, 0, sizeof(*sh));
    sh->num_chars = n;

    sh->char_buf = (char *)malloc((size_t)n * SCATTER_CHAR_SLOT);
    sh->outline8_handles = (void **)calloc(n, sizeof(void *));
    sh->ctx = (scatter_fly_ctx_t *)malloc(n * sizeof(scatter_fly_ctx_t));
    const lv_font_t **font_per_char = (const lv_font_t **)malloc(n * sizeof(const lv_font_t *));
    if (!sh->char_buf || !sh->outline8_handles || !sh->ctx || !font_per_char) {
        free(sh->char_buf);
        free(sh->outline8_handles);
        free(sh->ctx);
        free(font_per_char);
        free(sh);
        return NULL;
    }

    /* 按段展开：每段 UTF-8 拆成单字到 char_buf，并记录该字使用的字体 */
    uint32_t idx = 0;
    for (uint32_t s = 0; s < segment_count && idx < n; s++) {
        const char *text = segments[s].text ? segments[s].text : "";
        const lv_font_t *font = segments[s].font;
        uint32_t seg_chars = _lv_txt_get_encoded_length(text);
        for (uint32_t c = 0; c < seg_chars && idx < n; c++) {
            uint32_t byte_start = _lv_txt_encoded_get_byte_id(text, c);
            uint32_t byte_end   = _lv_txt_encoded_get_byte_id(text, c + 1);
            uint32_t len = byte_end - byte_start;
            if (len > (uint32_t)(SCATTER_CHAR_SLOT - 1)) len = SCATTER_CHAR_SLOT - 1;
            memcpy(sh->char_buf + idx * SCATTER_CHAR_SLOT, text + byte_start, len);
            sh->char_buf[idx * SCATTER_CHAR_SLOT + len] = '\0';
            font_per_char[idx] = font;
            idx++;
        }
    }

    /* 每字一个 outline8 label（单段、对应段字体，无打字机） */
    lv_label_segment_t seg_one;
    for (uint32_t i = 0; i < n; i++) {
        seg_one.text = sh->char_buf + i * SCATTER_CHAR_SLOT;
        seg_one.font = font_per_char[i];
        void *h8 = lv_label_create_with_outline8(parent, &seg_one, 1,
            0, offset_px, outline_color, outline_opa, mode, fill_color, 0, NULL);
        if (!h8) {
            for (uint32_t j = 0; j < i; j++) lv_label_with_outline8_delete(sh->outline8_handles[j]);
            free(sh->char_buf);
            free(sh->outline8_handles);
            free(sh->ctx);
            free(font_per_char);
            free(sh);
            return NULL;
        }
        sh->outline8_handles[i] = h8;
    }
    free(font_per_char);

    /* 每字宽高：lv_label_create_with_outline8 内部已按内容+描边设 cont 尺寸 */
    lv_coord_t *char_w = (lv_coord_t *)malloc(n * sizeof(lv_coord_t));
    lv_coord_t *char_h = (lv_coord_t *)malloc(n * sizeof(lv_coord_t));
    if (!char_w || !char_h) {
        free(char_w);
        free(char_h);
        for (uint32_t j = 0; j < n; j++) lv_label_with_outline8_delete(sh->outline8_handles[j]);
        free(sh->char_buf);
        free(sh->outline8_handles);
        free(sh->ctx);
        free(sh);
        return NULL;
    }
    lv_coord_t total_w = 0;
    for (uint32_t i = 0; i < n; i++) {
        lv_label_outline8_handle_t *hi = (lv_label_outline8_handle_t *)sh->outline8_handles[i];
        lv_obj_update_layout(hi->cont);
        char_w[i] = lv_obj_get_width(hi->cont);
        char_h[i] = lv_obj_get_height(hi->cont);
        if (char_w[i] < 1) char_w[i] = 1;
        if (char_h[i] < 1) char_h[i] = 20;
        total_w += char_w[i];
        sh->ctx[i].cont = hi->cont;
    }
    sh->total_w = total_w;

    /* 终点：单行或按 max_width 换行（按断字字符，英文等单词不拆开） */
    if (max_width <= 0) {
        lv_coord_t cur_x = row_x;
        for (uint32_t i = 0; i < n; i++) {
            sh->ctx[i].end_x = cur_x;
            sh->ctx[i].end_y = row_y;
            cur_x += char_w[i];
        }
    } else {
        lv_coord_t line_x = 0, line_y = 0, line_h = 0;
        uint32_t line_start = 0;
        uint32_t i = 0;
        while (i < n) {
            uint32_t word_end = i;
            lv_coord_t word_w = 0;
            while (word_end < n) {
                word_w += char_w[word_end];
                uint32_t ofs = 0;
                uint32_t letter = _lv_txt_encoded_next(sh->char_buf + word_end * SCATTER_CHAR_SLOT, &ofs);
                word_end++;
                if (word_end > i && _lv_txt_is_break_char(letter)) break;
            }
            if (line_x + word_w > max_width && line_x > 0) {
                /* 换行前：上一行各字垂直居中（上边下边留空）；行顶为 row_y+line_y，行高 line_h */
                for (uint32_t j = line_start; j < i; j++)
                    sh->ctx[j].end_y = row_y + line_y + (line_h - char_h[j]) / 2;
                divoom_print("[scatter_fly] wrap: line_x=%d word_w=%d max_width=%d (line_x+word_w=%d)\n",
                    (int)line_x, (int)word_w, (int)max_width, (int)(line_x + word_w));
                line_x = 0;
                line_y += line_h;
                line_start = i;
                line_h = 0;
            }
            for (uint32_t j = i; j < word_end; j++) {
                sh->ctx[j].end_x = row_x + line_x;
                sh->ctx[j].end_y = row_y + line_y;
                line_x += char_w[j];
                if (char_h[j] > line_h) line_h = char_h[j];
            }
            i = word_end;
        }
        /* 最后一行垂直居中 */
        if (line_h > 0) {
            for (uint32_t j = line_start; j < n; j++)
                sh->ctx[j].end_y = row_y + line_y + (line_h - char_h[j]) / 2;
        }
    }
    free(char_w);
    free(char_h);

    /* 起始：若调用方给了每字 starts，用之；否则用圆弧 + 默认旋转 */
    if (starts) {
        for (uint32_t i = 0; i < n; i++) {
            sh->ctx[i].start_x = starts[i].x;
            sh->ctx[i].start_y = starts[i].y;
            sh->ctx[i].start_angle = starts[i].angle_01;
        }
    } else {
        int16_t ac = (int16_t)arc_center_deg;
        if (ac < 0) ac += 360;
        uint16_t spread = arc_spread_deg;
        if (spread > 360) spread = 360;
        lv_coord_t cx = row_x + total_w / 2;  /* 一行中心 x，用于左右旋转方向 */
        lv_coord_t cy = row_y;
        for (uint32_t i = 0; i < n; i++) {
            int16_t angle_deg;
            if (n <= 1)
                angle_deg = ac;
            else
                angle_deg = (int16_t)(ac - (int32_t)spread / 2 + ((int32_t)spread * (int32_t)i) / (int32_t)(n - 1));
            int32_t sin_a = (int32_t)lv_trigo_sin(angle_deg);
            int32_t cos_a = (int32_t)lv_trigo_sin((int16_t)(angle_deg + 90));
            sh->ctx[i].start_x = cx + (lv_coord_t)((arc_radius * cos_a) >> LV_TRIGO_SHIFT);
            sh->ctx[i].start_y = cy + (lv_coord_t)((arc_radius * sin_a) >> LV_TRIGO_SHIFT);
            /* 每字旋转：中心左边的 -arc_rotate_angle_01，右边的 +arc_rotate_angle_01（0.1 度单位）；用 cont 宽避免换行时下一字 end_x 回退 */
            lv_coord_t char_w_i = lv_obj_get_width(sh->ctx[i].cont);
            lv_coord_t char_center_x = sh->ctx[i].end_x + char_w_i / 2;
            sh->ctx[i].start_angle = (char_center_x < cx) ? -arc_rotate_angle_01 : arc_rotate_angle_01;
        }
    }

    /* 先放到起始位置并设旋转 */
    for (uint32_t i = 0; i < n; i++) {
        lv_obj_set_pos(sh->ctx[i].cont, sh->ctx[i].start_x, sh->ctx[i].start_y);
        lv_obj_set_style_transform_angle(sh->ctx[i].cont, (lv_coord_t)sh->ctx[i].start_angle, 0);
    }

    /* 为每个字启动位移动画 */
    for (uint32_t i = 0; i < n; i++) {
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, &sh->ctx[i]);
        lv_anim_set_exec_cb(&a, scatter_fly_anim_exec);
        lv_anim_set_values(&a, 0, 1000);
        lv_anim_set_time(&a, (uint32_t)duration_ms);
        lv_anim_set_delay(&a, (uint32_t)i * delay_per_char_ms);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
        lv_anim_set_ready_cb(&a, scatter_fly_anim_ready);
        a.user_data = &sh->ctx[i];
        lv_anim_start(&a);
    }

    return sh;
}

/** 获取整句单行总宽度（不换行时的像素宽），用于调试 */
lv_coord_t lv_label_outline8_scatter_fly_in_get_total_width(void *handle)
{
    scatter_fly_handle_t *sh = (scatter_fly_handle_t *)handle;
    return sh ? sh->total_w : 0;
}

/** 获取 scatter 飞入所需容器高度：为内容底部坐标 y_max（各字 end_y + cont 高度之最大），
 * 使父容器 set_height(返回值) 后不裁切底部，且便于调用方用“起始Y+本高+间距”算下一行。
 * 此前曾返回 y_max - y_min（内容跨度），在首行垂直居中时 y_min>0，导致算出的高度偏小、底部被裁或下一行叠压。 */
lv_coord_t lv_label_outline8_scatter_fly_in_get_height(void *handle)
{
    scatter_fly_handle_t *sh = (scatter_fly_handle_t *)handle;
    if (!sh || sh->num_chars == 0) return 0;
    lv_coord_t y_max = sh->ctx[0].end_y + lv_obj_get_height(sh->ctx[0].cont);
    for (uint32_t i = 1; i < sh->num_chars; i++) {
        lv_coord_t bottom = sh->ctx[i].end_y + lv_obj_get_height(sh->ctx[i].cont);
        if (bottom > y_max) y_max = bottom;
    }
    return y_max;
}

void lv_label_outline8_scatter_fly_in_delete(void *handle)
{
    scatter_fly_handle_t *sh = (scatter_fly_handle_t *)handle;
    printf("[scatter_fly] delete entry handle=%p\n", (void *)handle);
    if (!sh) {
        printf("[scatter_fly] delete sh=NULL\n");
        return;
    }
    uint32_t num = sh->num_chars;
    printf("[scatter_fly] delete num_chars=%u ctx=%p outline8_handles=%p\n", (unsigned)num, (void *)sh->ctx, (void *)sh->outline8_handles);
    if (!sh->ctx || !sh->outline8_handles) {
        printf("[scatter_fly] delete BOUNDS ctx or outline8_handles NULL\n");
        if (sh->outline8_handles) free(sh->outline8_handles);
        if (sh->char_buf) free(sh->char_buf);
        if (sh->ctx) free(sh->ctx);
        free(sh);
        return;
    }
    /* 先删除所有字对应的飞入动画，避免删除对象后 scatter_fly_anim_exec 仍被调用导致异常 */
    for (uint32_t i = 0; i < num; i++) {
        lv_anim_del(&sh->ctx[i], NULL);
        sh->ctx[i].cont = NULL; /* 防止延迟回调访问已删对象 */
    }
    printf("[scatter_fly] delete anim_del done, deleting outline8\n");
    for (uint32_t i = 0; i < num; i++) {
        if (sh->outline8_handles[i]) {
            lv_label_with_outline8_delete(sh->outline8_handles[i]);
            sh->outline8_handles[i] = NULL;
        }
    }
    printf("[scatter_fly] delete outline8 done, freeing sh\n");
    free(sh->outline8_handles);
    sh->outline8_handles = NULL;
    free(sh->char_buf);
    sh->char_buf = NULL;
    free(sh->ctx);
    sh->ctx = NULL;
    free(sh);
    printf("[scatter_fly] delete done\n");
}

/** 立即结束飞入：删除所有字动画，并把每字设到终点位置，保证快照取到的是最终排版 */
void lv_label_outline8_scatter_fly_in_finish_now(void *handle)
{
    scatter_fly_handle_t *sh = (scatter_fly_handle_t *)handle;
    printf("[scatter_fly] finish_now entry handle=%p\n", (void *)handle);
    if (!sh) return;
    if (!sh->ctx) {
        printf("[scatter_fly] finish_now ctx=NULL\n");
        return;
    }
    uint32_t num = sh->num_chars;
    for (uint32_t i = 0; i < num; i++) {
        lv_anim_del(&sh->ctx[i], NULL);
        if (sh->ctx[i].cont) {
            lv_obj_set_pos(sh->ctx[i].cont, sh->ctx[i].end_x, sh->ctx[i].end_y);
            lv_obj_set_style_transform_angle(sh->ctx[i].cont, 0, 0);
        }
    }
    printf("[scatter_fly] finish_now done num_chars=%u\n", (unsigned)num);
}

/* ========== 每字飞入 scatter（描边+柔光 stroke_glow） ========== */
typedef struct {
    uint32_t num_chars;
    void **char_handles;
    char *char_buf;
    scatter_fly_ctx_t *ctx;
    lv_coord_t total_w;
} stroke_glow_scatter_fly_handle_t;

void *lv_label_stroke_glow_scatter_fly_in_create(
    lv_obj_t *parent,
    const lv_label_segment_t *segments,
    uint32_t segment_count,
    lv_coord_t max_width,
    lv_coord_t stroke_px,
    lv_color_t stroke_color,
    lv_opa_t stroke_opa,
    lv_color_t fill_color,
    lv_opa_t fill_opa,
    lv_coord_t glow_max_px,
    lv_color_t glow_color,
    lv_opa_t glow_opa,
    lv_coord_t row_x,
    lv_coord_t row_y,
    uint32_t duration_ms,
    uint32_t delay_per_char_ms,
    const lv_scatter_char_start_t *starts,
    int32_t arc_rotate_angle_01,
    lv_coord_t arc_radius,
    int16_t arc_center_deg,
    uint16_t arc_spread_deg)
{
    uint32_t n = 0;
    uint32_t s;
    uint32_t idx;
    stroke_glow_scatter_fly_handle_t *sh;
    const lv_font_t **font_per_char;
    lv_label_segment_t seg_one;
    uint32_t i;
    lv_coord_t *char_w;
    lv_coord_t *char_h;
    lv_coord_t total_w;

    if (!parent || !segments || segment_count == 0)
    {
        return NULL;
    }
    for (s = 0; s < segment_count; s++)
    {
        const char *t = segments[s].text ? segments[s].text : "";

        n += _lv_txt_get_encoded_length(t);
    }
    if (n == 0)
    {
        return NULL;
    }

    sh = (stroke_glow_scatter_fly_handle_t *)malloc(sizeof(stroke_glow_scatter_fly_handle_t));
    if (!sh)
    {
        return NULL;
    }
    memset(sh, 0, sizeof(*sh));
    sh->num_chars = n;

    sh->char_buf = (char *)malloc((size_t)n * SCATTER_CHAR_SLOT);
    sh->char_handles = (void **)calloc(n, sizeof(void *));
    sh->ctx = (scatter_fly_ctx_t *)malloc(n * sizeof(scatter_fly_ctx_t));
    font_per_char = (const lv_font_t **)malloc(n * sizeof(const lv_font_t *));
    if (!sh->char_buf || !sh->char_handles || !sh->ctx || !font_per_char)
    {
        free(sh->char_buf);
        free(sh->char_handles);
        free(sh->ctx);
        free(font_per_char);
        free(sh);
        return NULL;
    }

    idx = 0;
    for (s = 0; s < segment_count && idx < n; s++)
    {
        const char *text = segments[s].text ? segments[s].text : "";
        const lv_font_t *font = segments[s].font;
        uint32_t seg_chars = _lv_txt_get_encoded_length(text);
        uint32_t c;

        for (c = 0; c < seg_chars && idx < n; c++)
        {
            uint32_t byte_start = _lv_txt_encoded_get_byte_id(text, c);
            uint32_t byte_end = _lv_txt_encoded_get_byte_id(text, c + 1);
            uint32_t len = byte_end - byte_start;

            if (len > (uint32_t)(SCATTER_CHAR_SLOT - 1))
            {
                len = SCATTER_CHAR_SLOT - 1;
            }
            memcpy(sh->char_buf + idx * SCATTER_CHAR_SLOT, text + byte_start, len);
            sh->char_buf[idx * SCATTER_CHAR_SLOT + len] = '\0';
            font_per_char[idx] = font;
            idx++;
        }
    }

    seg_one.font = NULL;
    for (i = 0; i < n; i++)
    {
        seg_one.text = sh->char_buf + i * SCATTER_CHAR_SLOT;
        seg_one.font = font_per_char[i];
        sh->char_handles[i] = lv_label_create_with_stroke_glow(parent, &seg_one, 1,
            0, stroke_px, stroke_color, stroke_opa, fill_color, fill_opa, glow_max_px, glow_color, glow_opa, 0);
        if (!sh->char_handles[i])
        {
            for (uint32_t j = 0; j < i; j++)
            {
                lv_label_with_stroke_glow_delete(sh->char_handles[j]);
            }
            free(sh->char_buf);
            free(sh->char_handles);
            free(sh->ctx);
            free(font_per_char);
            free(sh);
            return NULL;
        }
    }
    free(font_per_char);

    char_w = (lv_coord_t *)malloc(n * sizeof(lv_coord_t));
    char_h = (lv_coord_t *)malloc(n * sizeof(lv_coord_t));
    if (!char_w || !char_h)
    {
        free(char_w);
        free(char_h);
        for (i = 0; i < n; i++)
        {
            lv_label_with_stroke_glow_delete(sh->char_handles[i]);
        }
        free(sh->char_buf);
        free(sh->char_handles);
        free(sh->ctx);
        free(sh);
        return NULL;
    }
    total_w = 0;
    for (i = 0; i < n; i++)
    {
        lv_label_stroke_glow_handle_t *hi = (lv_label_stroke_glow_handle_t *)sh->char_handles[i];

        lv_obj_update_layout(hi->cont);
        char_w[i] = lv_obj_get_width(hi->cont);
        char_h[i] = lv_obj_get_height(hi->cont);
        if (char_w[i] < 1)
        {
            char_w[i] = 1;
        }
        if (char_h[i] < 1)
        {
            char_h[i] = 20;
        }
        total_w += char_w[i];
        sh->ctx[i].cont = hi->cont;
    }
    sh->total_w = total_w;

    if (max_width <= 0)
    {
        lv_coord_t cur_x = row_x;

        for (i = 0; i < n; i++)
        {
            sh->ctx[i].end_x = cur_x;
            sh->ctx[i].end_y = row_y;
            cur_x += char_w[i];
        }
    }
    else
    {
        lv_coord_t line_x = 0;
        lv_coord_t line_y = 0;
        lv_coord_t line_h = 0;
        uint32_t line_start = 0;

        i = 0;
        while (i < n)
        {
            uint32_t word_end = i;
            lv_coord_t word_w = 0;

            while (word_end < n)
            {
                word_w += char_w[word_end];
                {
                    uint32_t ofs = 0;
                    uint32_t letter = _lv_txt_encoded_next(sh->char_buf + word_end * SCATTER_CHAR_SLOT, &ofs);

                    word_end++;
                    if (word_end > i && _lv_txt_is_break_char(letter))
                    {
                        break;
                    }
                }
            }
            if (line_x + word_w > max_width && line_x > 0)
            {
                uint32_t j;

                for (j = line_start; j < i; j++)
                {
                    sh->ctx[j].end_y = row_y + line_y + (line_h - char_h[j]) / 2;
                }
                line_x = 0;
                line_y += line_h;
                line_start = i;
                line_h = 0;
            }
            for (uint32_t j = i; j < word_end; j++)
            {
                sh->ctx[j].end_x = row_x + line_x;
                sh->ctx[j].end_y = row_y + line_y;
                line_x += char_w[j];
                if (char_h[j] > line_h)
                {
                    line_h = char_h[j];
                }
            }
            i = word_end;
        }
        if (line_h > 0)
        {
            for (uint32_t j = line_start; j < n; j++)
            {
                sh->ctx[j].end_y = row_y + line_y + (line_h - char_h[j]) / 2;
            }
        }
    }
    free(char_w);
    free(char_h);

    if (starts)
    {
        for (i = 0; i < n; i++)
        {
            sh->ctx[i].start_x = starts[i].x;
            sh->ctx[i].start_y = starts[i].y;
            sh->ctx[i].start_angle = starts[i].angle_01;
        }
    }
    else
    {
        int16_t ac = (int16_t)arc_center_deg;
        lv_coord_t cx;
        lv_coord_t cy;

        if (ac < 0)
        {
            ac += 360;
        }
        {
            uint16_t spread = arc_spread_deg;

            if (spread > 360)
            {
                spread = 360;
            }
            cx = row_x + total_w / 2;
            cy = row_y;
            for (i = 0; i < n; i++)
            {
                int16_t angle_deg;

                if (n <= 1)
                {
                    angle_deg = ac;
                }
                else
                {
                    angle_deg = (int16_t)(ac - (int32_t)spread / 2 + ((int32_t)spread * (int32_t)i) / (int32_t)(n - 1));
                }
                {
                    int32_t sin_a = (int32_t)lv_trigo_sin(angle_deg);
                    int32_t cos_a = (int32_t)lv_trigo_sin((int16_t)(angle_deg + 90));
                    lv_coord_t char_w_i = lv_obj_get_width(sh->ctx[i].cont);
                    lv_coord_t char_center_x = sh->ctx[i].end_x + char_w_i / 2;

                    sh->ctx[i].start_x = cx + (lv_coord_t)((arc_radius * cos_a) >> LV_TRIGO_SHIFT);
                    sh->ctx[i].start_y = cy + (lv_coord_t)((arc_radius * sin_a) >> LV_TRIGO_SHIFT);
                    sh->ctx[i].start_angle = (char_center_x < cx) ? -arc_rotate_angle_01 : arc_rotate_angle_01;
                }
            }
        }
    }

    for (i = 0; i < n; i++)
    {
        lv_obj_set_pos(sh->ctx[i].cont, sh->ctx[i].start_x, sh->ctx[i].start_y);
        lv_obj_set_style_transform_angle(sh->ctx[i].cont, (lv_coord_t)sh->ctx[i].start_angle, 0);
    }

    for (i = 0; i < n; i++)
    {
        lv_anim_t a;

        lv_anim_init(&a);
        lv_anim_set_var(&a, &sh->ctx[i]);
        lv_anim_set_exec_cb(&a, scatter_fly_anim_exec);
        lv_anim_set_values(&a, 0, 1000);
        lv_anim_set_time(&a, (uint32_t)duration_ms);
        lv_anim_set_delay(&a, (uint32_t)i * delay_per_char_ms);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
        lv_anim_set_ready_cb(&a, scatter_fly_anim_ready);
        a.user_data = &sh->ctx[i];
        lv_anim_start(&a);
    }

    return sh;
}

lv_coord_t lv_label_stroke_glow_scatter_fly_in_get_total_width(void *handle)
{
    stroke_glow_scatter_fly_handle_t *sh = (stroke_glow_scatter_fly_handle_t *)handle;

    return sh ? sh->total_w : 0;
}

lv_coord_t lv_label_stroke_glow_scatter_fly_in_get_height(void *handle)
{
    stroke_glow_scatter_fly_handle_t *sh = (stroke_glow_scatter_fly_handle_t *)handle;
    uint32_t i;

    if (!sh || sh->num_chars == 0)
    {
        return 0;
    }
    {
        lv_coord_t y_max = sh->ctx[0].end_y + lv_obj_get_height(sh->ctx[0].cont);

        for (i = 1; i < sh->num_chars; i++)
        {
            lv_coord_t bottom = sh->ctx[i].end_y + lv_obj_get_height(sh->ctx[i].cont);

            if (bottom > y_max)
            {
                y_max = bottom;
            }
        }
        return y_max;
    }
}

void lv_label_stroke_glow_scatter_fly_in_delete(void *handle)
{
    stroke_glow_scatter_fly_handle_t *sh = (stroke_glow_scatter_fly_handle_t *)handle;
    uint32_t num;
    uint32_t i;

    if (!sh)
    {
        return;
    }
    num = sh->num_chars;
    if (!sh->ctx || !sh->char_handles)
    {
        free(sh->char_handles);
        free(sh->char_buf);
        free(sh->ctx);
        free(sh);
        return;
    }
    for (i = 0; i < num; i++)
    {
        lv_anim_del(&sh->ctx[i], NULL);
        sh->ctx[i].cont = NULL;
    }
    for (i = 0; i < num; i++)
    {
        if (sh->char_handles[i])
        {
            lv_label_with_stroke_glow_delete(sh->char_handles[i]);
            sh->char_handles[i] = NULL;
        }
    }
    free(sh->char_handles);
    free(sh->char_buf);
    free(sh->ctx);
    free(sh);
}

void lv_label_stroke_glow_scatter_fly_in_finish_now(void *handle)
{
    stroke_glow_scatter_fly_handle_t *sh = (stroke_glow_scatter_fly_handle_t *)handle;
    uint32_t num;
    uint32_t i;

    if (!sh || !sh->ctx)
    {
        return;
    }
    num = sh->num_chars;
    for (i = 0; i < num; i++)
    {
        lv_anim_del(&sh->ctx[i], NULL);
        if (sh->ctx[i].cont)
        {
            lv_obj_set_pos(sh->ctx[i].cont, sh->ctx[i].end_x, sh->ctx[i].end_y);
            lv_obj_set_style_transform_angle(sh->ctx[i].cont, 0, 0);
        }
    }
}

// sobel_3d_rgba_int.c
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// Sobel算子整数核（放大16倍）
#define SOBEL_SCALE 16
static const int16_t SOBEL_X_INT[9] = {-16, 0, 16, -32, 0, 32, -16, 0, 16};
static const int16_t SOBEL_Y_INT[9] = {-16, -32, -16, 0, 0, 0, 16, 32, 16};

// 快速整数平方根
static inline uint32_t fast_isqrt(uint32_t x) {
    if (x < 2) return x;
    uint32_t y = x / 2;
    for (int i = 0; i < 3; i++) {
        y = (y + x / y) / 2;
    }
    return y;
}

/* 像素格式说明（按字节序）：
 * - RGBA: [R, G, B, A]
 * - BGRA(很多库/硬件/G2D/WEBP里也常被叫 ABGR8888): [B, G, R, A]
 */
static inline uint8_t gray_from_rgba8888(const uint8_t *px_rgba) {
    return (px_rgba[0] * 299 + px_rgba[1] * 587 + px_rgba[2] * 114) / 1000;
}
static inline uint8_t gray_from_bgra8888(const uint8_t *px_bgra) {
    return (px_bgra[2] * 299 + px_bgra[1] * 587 + px_bgra[0] * 114) / 1000;
}
static inline void swap_rb_inplace(uint8_t *buf, int total_px) {
    for (int i = 0; i < total_px; i++) {
        uint8_t *p = &buf[i * 4];
        uint8_t t = p[0]; /* R or B */
        p[0] = p[2];
        p[2] = t;
    }
}

// 生成深度图（从RGBA缓冲区）
static uint8_t* generate_depth_from_rgba(const uint8_t* rgba, int width, int height,
                                         uint8_t brightness_weight, uint8_t edge_weight) {
    int total = width * height;
    uint8_t* gray = malloc(total);          // 灰度图
    uint8_t* depth = malloc(total);         // 最终深度
    uint8_t* edge_map = malloc(total);      // 边缘图
    
    if (!gray || !depth || !edge_map) {
        free(gray); free(depth); free(edge_map);
        return NULL;
    }
    
    // 1. 转换为灰度图（按 RGBA 解释）
    for (int i = 0; i < total; i++) gray[i] = gray_from_rgba8888(&rgba[i * 4]);
    
    // 2. 计算边缘强度（使用Sobel）
    memset(edge_map, 0, total);
    
    // 跳过边缘像素
    for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            int idx = y * width + x;
            const uint8_t* w = &gray[(y-1) * width + (x-1)];
            
            // 展开的Sobel卷积（避免循环）
            int32_t gx = w[0]*SOBEL_X_INT[0] + w[1]*SOBEL_X_INT[1] + w[2]*SOBEL_X_INT[2] +
                        w[width]*SOBEL_X_INT[3] + w[width+1]*SOBEL_X_INT[4] + w[width+2]*SOBEL_X_INT[5] +
                        w[width*2]*SOBEL_X_INT[6] + w[width*2+1]*SOBEL_X_INT[7] + w[width*2+2]*SOBEL_X_INT[8];
            
            int32_t gy = w[0]*SOBEL_Y_INT[0] + w[1]*SOBEL_Y_INT[1] + w[2]*SOBEL_Y_INT[2] +
                        w[width]*SOBEL_Y_INT[3] + w[width+1]*SOBEL_Y_INT[4] + w[width+2]*SOBEL_Y_INT[5] +
                        w[width*2]*SOBEL_Y_INT[6] + w[width*2+1]*SOBEL_Y_INT[7] + w[width*2+2]*SOBEL_Y_INT[8];
            
            // 缩放回正确范围并计算梯度
            gx /= SOBEL_SCALE;
            gy /= SOBEL_SCALE;
            uint32_t grad_sq = gx*gx + gy*gy;
            uint32_t gradient = fast_isqrt(grad_sq);
            
            // 限制并存储边缘强度（梯度大=边缘强）
            if (gradient > 255) gradient = 255;
            edge_map[idx] = (uint8_t)gradient;
        }
    }
    
    // 3. 计算亮度深度并融合
    for (int i = 0; i < total; i++) {
        uint8_t alpha = rgba[i * 4 + 3];
        /* 完全透明：当作背景(最远) */
        if (alpha == 0) {
            depth[i] = 255;
            continue;
        }

        // 亮度深度 = 255 - 灰度值（亮=前景，深=背景）
        uint8_t brightness_depth = 255 - gray[i];
        
        // 边缘深度 = 255 - 边缘强度（边缘=前景）
        uint8_t edge_depth = 255 - edge_map[i];
        
        // 使用透明度影响权重（如果alpha<255）
        uint8_t effective_bw = brightness_weight;
        uint8_t effective_ew = edge_weight;
        
        if (alpha < 255) {
            // 透明度高的区域减少边缘权重
            uint16_t alpha_factor = alpha * 256 / 255;
            effective_ew = (edge_weight * alpha_factor) >> 8;
        }
        
        /* 归一化加权融合：避免 bw+ew 不等于 256 时深度分布失真 */
        uint16_t wsum = (uint16_t)effective_bw + (uint16_t)effective_ew;
        if (wsum == 0) wsum = 1;
        uint16_t combined = (uint16_t)(brightness_depth * effective_bw + edge_depth * effective_ew);
        combined = (combined + (wsum / 2)) / wsum; /* 四舍五入 */
        
        depth[i] = (combined > 255) ? 255 : (uint8_t)combined;
    }
    
    // 4. 深度图归一化（整数拉伸）
    uint8_t min_val = 255, max_val = 0;
    for (int i = 0; i < total; i++) {
        if (depth[i] < min_val) min_val = depth[i];
        if (depth[i] > max_val) max_val = depth[i];
    }
    
    uint8_t range = max_val - min_val;
    if (range > 20) {  // 有足够对比度才拉伸
        // 计算拉伸因子（放大256倍）
        uint16_t scale = (255 * 256) / range;
        
        for (int i = 0; i < total; i++) {
            uint16_t stretched = (depth[i] - min_val) * scale;
            depth[i] = (stretched >> 8) & 0xFF;
        }
    }

    /* 5. 深度图轻量平滑：减少 Sobel 噪声带来的“喷点/破碎”
     * 只做一次 3x3 box blur；alpha==0 的像素保持为背景(255) */
    {
        uint8_t *tmp = (uint8_t *)malloc((size_t)total);
        if (tmp) {
            memcpy(tmp, depth, (size_t)total);
            for (int y = 1; y < height - 1; y++) {
                for (int x = 1; x < width - 1; x++) {
                    int idx = y * width + x;
                    if (rgba[idx * 4 + 3] == 0) { tmp[idx] = 255; continue; }

                    int sum = 0;
                    int cnt = 0;
                    for (int yy = -1; yy <= 1; yy++) {
                        for (int xx = -1; xx <= 1; xx++) {
                            int ii = (y + yy) * width + (x + xx);
                            if (rgba[ii * 4 + 3] == 0) continue;
                            sum += depth[ii];
                            cnt++;
                        }
                    }
                    tmp[idx] = (cnt == 0) ? 255 : (uint8_t)((sum + cnt / 2) / cnt);
                }
            }
            memcpy(depth, tmp, (size_t)total);
            free(tmp);
        }
    }
    
    free(gray);
    free(edge_map);
    return depth;
}

// 生成深度图（从BGRA缓冲区，常用于 LVGL/WEBP/G2D 的“ABGR8888”命名）
static uint8_t* generate_depth_from_bgra(const uint8_t* bgra, int width, int height,
                                         uint8_t brightness_weight, uint8_t edge_weight) {
    int total = width * height;
    uint8_t* gray = malloc(total);
    uint8_t* depth = malloc(total);
    uint8_t* edge_map = malloc(total);
    if (!gray || !depth || !edge_map) {
        free(gray); free(depth); free(edge_map);
        return NULL;
    }

    for (int i = 0; i < total; i++) gray[i] = gray_from_bgra8888(&bgra[i * 4]);

    memset(edge_map, 0, total);
    for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            int idx = y * width + x;
            const uint8_t* w = &gray[(y-1) * width + (x-1)];
            int32_t gx = w[0]*SOBEL_X_INT[0] + w[1]*SOBEL_X_INT[1] + w[2]*SOBEL_X_INT[2] +
                        w[width]*SOBEL_X_INT[3] + w[width+1]*SOBEL_X_INT[4] + w[width+2]*SOBEL_X_INT[5] +
                        w[width*2]*SOBEL_X_INT[6] + w[width*2+1]*SOBEL_X_INT[7] + w[width*2+2]*SOBEL_X_INT[8];
            int32_t gy = w[0]*SOBEL_Y_INT[0] + w[1]*SOBEL_Y_INT[1] + w[2]*SOBEL_Y_INT[2] +
                        w[width]*SOBEL_Y_INT[3] + w[width+1]*SOBEL_Y_INT[4] + w[width+2]*SOBEL_Y_INT[5] +
                        w[width*2]*SOBEL_Y_INT[6] + w[width*2+1]*SOBEL_Y_INT[7] + w[width*2+2]*SOBEL_Y_INT[8];
            gx /= SOBEL_SCALE;
            gy /= SOBEL_SCALE;
            uint32_t grad_sq = gx*gx + gy*gy;
            uint32_t gradient = fast_isqrt(grad_sq);
            if (gradient > 255) gradient = 255;
            edge_map[idx] = (uint8_t)gradient;
        }
    }

    for (int i = 0; i < total; i++) {
        uint8_t alpha = bgra[i * 4 + 3];
        if (alpha == 0) { depth[i] = 255; continue; }

        uint8_t brightness_depth = 255 - gray[i];
        uint8_t edge_depth = 255 - edge_map[i];

        uint8_t effective_bw = brightness_weight;
        uint8_t effective_ew = edge_weight;
        if (alpha < 255) {
            uint16_t alpha_factor = alpha * 256 / 255;
            effective_ew = (edge_weight * alpha_factor) >> 8;
        }

        uint16_t wsum = (uint16_t)effective_bw + (uint16_t)effective_ew;
        if (wsum == 0) wsum = 1;
        uint16_t combined = (uint16_t)(brightness_depth * effective_bw + edge_depth * effective_ew);
        combined = (combined + (wsum / 2)) / wsum;
        depth[i] = (combined > 255) ? 255 : (uint8_t)combined;
    }

    uint8_t min_val = 255, max_val = 0;
    for (int i = 0; i < total; i++) {
        if (depth[i] < min_val) min_val = depth[i];
        if (depth[i] > max_val) max_val = depth[i];
    }
    uint8_t range = max_val - min_val;
    if (range > 20) {
        uint16_t scale = (255 * 256) / range;
        for (int i = 0; i < total; i++) {
            uint16_t stretched = (depth[i] - min_val) * scale;
            depth[i] = (stretched >> 8) & 0xFF;
        }
    }

    /* 与 RGBA 版本一致：一次 3x3 box blur 平滑 */
    {
        uint8_t *tmp = (uint8_t *)malloc((size_t)total);
        if (tmp) {
            memcpy(tmp, depth, (size_t)total);
            for (int y = 1; y < height - 1; y++) {
                for (int x = 1; x < width - 1; x++) {
                    int idx = y * width + x;
                    if (bgra[idx * 4 + 3] == 0) { tmp[idx] = 255; continue; }
                    int sum = 0, cnt = 0;
                    for (int yy = -1; yy <= 1; yy++) {
                        for (int xx = -1; xx <= 1; xx++) {
                            int ii = (y + yy) * width + (x + xx);
                            if (bgra[ii * 4 + 3] == 0) continue;
                            sum += depth[ii];
                            cnt++;
                        }
                    }
                    tmp[idx] = (cnt == 0) ? 255 : (uint8_t)((sum + cnt / 2) / cnt);
                }
            }
            memcpy(depth, tmp, (size_t)total);
            free(tmp);
        }
    }

    free(gray);
    free(edge_map);
    return depth;
}

// RGBA的3D投影渲染
void render_rgba_3d(const uint8_t* src_rgba, const uint8_t* depth,
                   int src_w, int src_h,
                   uint8_t* dst_rgba, int dst_w, int dst_h,
                   int scale_z, int use_perspective) {
    // 清空目标缓冲区（透明黑色）
    memset(dst_rgba, 0, dst_w * dst_h * 4);

    /* Z-buffer：存最“近”的深度（depth 小=近，大=远） */
    uint8_t *zbuf = (uint8_t *)malloc((size_t)dst_w * (size_t)dst_h);
    if (!zbuf) return;
    memset(zbuf, 255, (size_t)dst_w * (size_t)dst_h);
    
    // 预计算中心点
    int src_cx = src_w / 2;
    int src_cy = src_h / 2;
    int dst_cx = dst_w / 2;
    int dst_cy = dst_h / 2;
    
    // 定点数参数（放大256倍）
    #define FIX_SHIFT 8
    /* 让图像在 dst 中占据合理大小：按 dst/src 自动缩放（Q8）
     * 这样不会出现“输出缩成一小块”的问题 */
    int scale_x_q8 = ((dst_w - 2) << FIX_SHIFT) / (src_w ? src_w : 1);
    int scale_y_q8 = ((dst_h - 2) << FIX_SHIFT) / (src_h ? src_h : 1);
    int scale_xy_q8 = (scale_x_q8 < scale_y_q8) ? scale_x_q8 : scale_y_q8;
    if (scale_xy_q8 < 1) scale_xy_q8 = 1;

    int perspective_factor = use_perspective ? (1 << FIX_SHIFT) : 0; /* 1.0 in Q8 */

    /* 每行“连线填洞”用：记录上一个投影点 */
    int prev_valid = 0;
    int prev_x2d = 0, prev_y2d = 0, prev_depth = 255;
    
    for (int y = 0; y < src_h; y++) {
        prev_valid = 0;
        for (int x = 0; x < src_w; x++) {
            int src_idx = y * src_w + x;
            int rgba_idx = src_idx * 4;
            
            // 跳过完全透明的像素
            if (src_rgba[rgba_idx + 3] == 0) continue;
            
            // 获取深度值（0-255）并转换为定点数
            int depth_val = depth[src_idx];
            
            // 1. 计算3D坐标（定点数）
            int dx = x - src_cx;
            int dy = src_cy - y;  // Y轴向上
            
            // XY缩放
            int x3d_q8 = dx * scale_xy_q8;
            int y3d_q8 = dy * scale_xy_q8;
            
            // Z坐标：深度值 * 缩放系数 / 255
            int z3d = (depth_val * scale_z) / 255;
            
            // 2. 投影到2D
            int x2d, y2d;
            
            if (use_perspective && z3d > 10) {
                // 透视投影：近大远小
                int denom = (1 << FIX_SHIFT) + z3d;
                int x2d_q8 = (int)(((int64_t)x3d_q8 * perspective_factor) / denom);
                int y2d_q8 = (int)(((int64_t)y3d_q8 * perspective_factor) / denom);
                x2d = (x2d_q8 >> FIX_SHIFT) + dst_cx;
                y2d = (-y2d_q8 >> FIX_SHIFT) + dst_cy;
            } else {
                // 正交投影
                x2d = (x3d_q8 >> FIX_SHIFT) + dst_cx;
                y2d = (-y3d_q8 >> FIX_SHIFT) + dst_cy;  // Y反转
            }
            
            /* 连线填洞：将同一行相邻像素的投影点之间插值填充，减少空洞/破碎 */
            if (prev_valid) {
                int dxp = x2d - prev_x2d;
                int dyp = y2d - prev_y2d;
                int steps = abs(dxp);
                if (abs(dyp) > steps) steps = abs(dyp);
                if (steps > 1 && steps <= 10) { /* 过大不连，避免跨越错误 */
                    for (int s = 1; s < steps; s++) {
                        int xx = prev_x2d + (dxp * s) / steps;
                        int yy = prev_y2d + (dyp * s) / steps;
                        int dep = prev_depth + (depth_val - prev_depth) * s / steps;
                        if (xx < 0 || xx >= dst_w || yy < 0 || yy >= dst_h) continue;
                        int zidx = yy * dst_w + xx;
                        if (dep >= zbuf[zidx]) continue;
                        zbuf[zidx] = (uint8_t)dep;
                        int dst_idx = zidx * 4;
                        dst_rgba[dst_idx]     = src_rgba[rgba_idx];
                        dst_rgba[dst_idx + 1] = src_rgba[rgba_idx + 1];
                        dst_rgba[dst_idx + 2] = src_rgba[rgba_idx + 2];
                        dst_rgba[dst_idx + 3] = src_rgba[rgba_idx + 3];
                    }
                }
            }
            prev_valid = 1;
            prev_x2d = x2d; prev_y2d = y2d; prev_depth = depth_val;
            
            // 4. 写入输出像素（简单覆盖）
            /* Z-test + splat(3x3) 填洞：比 2x2 更能覆盖空洞 */
            for (int oy = -1; oy <= 1; oy++) {
                for (int ox = -1; ox <= 1; ox++) {
                    int xx = x2d + ox;
                    int yy = y2d + oy;
                    if (xx < 0 || xx >= dst_w || yy < 0 || yy >= dst_h) continue;

                    int zidx = yy * dst_w + xx;
                    if (depth_val >= zbuf[zidx]) continue; /* 更远则不覆盖 */
                    zbuf[zidx] = (uint8_t)depth_val;

                    int dst_idx = zidx * 4;
                    dst_rgba[dst_idx]     = src_rgba[rgba_idx];     // R
                    dst_rgba[dst_idx + 1] = src_rgba[rgba_idx + 1]; // G
                    dst_rgba[dst_idx + 2] = src_rgba[rgba_idx + 2]; // B
                    dst_rgba[dst_idx + 3] = src_rgba[rgba_idx + 3];
                }
            }
        }
    }

    free(zbuf);
}

// 主处理函数（直接处理RGBA缓冲区）
void process_rgba_3d_transform(const uint8_t* src_rgba, int src_w, int src_h,
                              uint8_t* dst_rgba, int dst_w, int dst_h,
                              uint8_t brightness_weight, uint8_t edge_weight,
                              int scale_z, int use_perspective) {
    
    printf("处理RGBA 3D变换: %dx%d -> %dx%d\n", src_w, src_h, dst_w, dst_h);
    
    // 1. 生成深度图
    uint8_t* depth = generate_depth_from_rgba(src_rgba, src_w, src_h,
                                             brightness_weight, edge_weight);
    if (!depth) {
        printf("深度图生成失败\n");
        return;
    }
    
    // 2. 执行3D投影渲染
    render_rgba_3d(src_rgba, depth, src_w, src_h,
                  dst_rgba, dst_w, dst_h,
                  scale_z, use_perspective);
    
    free(depth);
}

/* BGRA(“ABGR8888”) 版本：用于你们当前 LVGL / WebPEncodeBGRA 的缓冲区 */
void process_bgra_3d_transform(const uint8_t* src_bgra, int src_w, int src_h,
                               uint8_t* dst_bgra, int dst_w, int dst_h,
                               uint8_t brightness_weight, uint8_t edge_weight,
                               int scale_z, int use_perspective) {
    uint8_t* depth = generate_depth_from_bgra(src_bgra, src_w, src_h, brightness_weight, edge_weight);
    if (!depth) return;
    render_rgba_3d(src_bgra, depth, src_w, src_h, dst_bgra, dst_w, dst_h, scale_z, use_perspective);
    free(depth);
}

// 测试函数：创建RGBA测试图像
uint8_t* create_test_rgba(int width, int height) {
    uint8_t* rgba = malloc(width * height * 4);
    int center_x = width / 2;
    int center_y = height / 2;
    int radius = width / 3;
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = (y * width + x) * 4;
            int dx = x - center_x;
            int dy = y - center_y;
            int dist_sq = dx*dx + dy*dy;
            
            // 背景：蓝色渐变
            rgba[idx] = x * 255 / width;        // R渐变
            rgba[idx + 1] = y * 255 / height;   // G渐变
            rgba[idx + 2] = 200;                // B固定
            rgba[idx + 3] = 255;                // 不透明
            
            // 前景圆形：红色
            if (dist_sq < radius * radius) {
                rgba[idx] = 255;                // R
                rgba[idx + 1] = 100;            // G
                rgba[idx + 2] = 100;            // B
                rgba[idx + 3] = 230;            // 稍透明
                
                // 内圆：黄色
                if (dist_sq < (radius/2) * (radius/2)) {
                    rgba[idx] = 255;            // R
                    rgba[idx + 1] = 255;        // G  
                    rgba[idx + 2] = 100;        // B
                    rgba[idx + 3] = 255;        // 不透明
                }
            }
        }
    }
    
    return rgba;
}

// 保存为PPM（RGB）或PGM（灰度）用于查看
void save_rgb_as_ppm(const char* filename, const uint8_t* rgba, int w, int h) {
    FILE* fp = divoom_port_file_open(filename, "wb");
    fprintf(fp, "P6\n%d %d\n255\n", w, h);
    
    for (int i = 0; i < w * h; i++) {
        fputc(rgba[i*4], fp);      // R
        fputc(rgba[i*4+1], fp);    // G
        fputc(rgba[i*4+2], fp);    // B
    }
    
    divoom_port_file_close(fp);
}

/* 避免集成到工程时与系统 main 冲突：需要测试时编译加 -DSOBEL_3D_TEST_MAIN */
#ifdef SOBEL_3D_TEST_MAIN
int main() {
    printf("=== RGBA缓冲区3D变换（整数优化版） ===\n");
    
    int src_w = 480, src_h = 480;
    int dst_w = 600, dst_h = 500;
    
    // 创建测试RGBA图像
    uint8_t* src_rgba = create_test_rgba(src_w, src_h);
    uint8_t* dst_rgba = malloc(dst_w * dst_h * 4);
    
    if (!src_rgba || !dst_rgba) {
        printf("内存分配失败\n");
        return 1;
    }
    
    // 处理参数
    uint8_t brightness_weight = 180;  // ~0.7
    uint8_t edge_weight = 75;         // ~0.3
    int scale_z = 150;                // Z缩放强度
    int use_perspective = 1;          // 使用透视投影
    
    // 执行变换
    process_rgba_3d_transform(src_rgba, src_w, src_h,
                             dst_rgba, dst_w, dst_h,
                             brightness_weight, edge_weight,
                             scale_z, use_perspective);
    
    // 保存结果
    save_rgb_as_ppm("input_rgba.ppm", src_rgba, src_w, src_h);
    save_rgb_as_ppm("output_3d.ppm", dst_rgba, dst_w, dst_h);
    
    printf("处理完成！\n");
    printf("输入保存为: input_rgba.ppm\n");
    printf("输出保存为: output_3d.ppm\n");
    
    // 内存统计
    size_t mem_used = (src_w * src_h * 4) +      // 输入RGBA
                     (src_w * src_h) +          // 深度图
                     (dst_w * dst_h * 4) +      // 输出RGBA
                     (src_w * src_h * 2);       // 临时缓冲区
    
    printf("峰值内存: %.2f MB\n", mem_used / (1024.0 * 1024.0));
    
    // 清理
    free(src_rgba);
    free(dst_rgba);
    
    return 0;
}
#endif

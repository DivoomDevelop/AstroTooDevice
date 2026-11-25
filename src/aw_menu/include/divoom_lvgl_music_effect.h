/**
 * @file divoom_lvgl_music_effect.h
 * @brief 带阴影/8 向描边/每字飞入等 label 效果接口（多段多字号、换行、打字机、笔位置等）
 */

#ifndef DIVOOM_LVGL_MUSIC_EFFECT_H
#define DIVOOM_LVGL_MUSIC_EFFECT_H

#include "lvgl.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========== 多段文本（字串 + 字体） ========== */
typedef struct {
    const char *text;
    const lv_font_t *font;
} lv_label_segment_t;

/* ========== 带阴影的 label（支持多段多字号、max_width 换行） ========== */
typedef struct {
    lv_obj_t *cont;
    lv_obj_t *main;
} lv_label_shadow_handle_t;

/** 创建带阴影 label；max_width>0 时超宽自动换行（按断字，英文单词不拆开） */
void *lv_label_create_with_shadow(lv_obj_t *parent, const lv_label_segment_t *segments,
    uint32_t segment_count, lv_coord_t max_width, lv_coord_t shadow_offset_x, lv_coord_t shadow_offset_y,
    lv_color_t shadow_color, lv_opa_t shadow_opa, lv_color_t text_color, uint32_t typing_duration_ms);

/** 获取实际使用高度（换行后总高度） */
lv_coord_t lv_label_with_shadow_get_height(void *h);

void lv_label_with_shadow_set_text(void *h, const char *text);
void lv_label_with_shadow_set_segments(void *h, const lv_label_segment_t *segments, uint32_t segment_count);
void lv_label_with_shadow_delete(void *h);

/* ========== 8 方向描边 label（支持多段多字号、max_width 换行、打字机、cursor_img 笔位置） ========== */
typedef struct {
    lv_obj_t *cont;
    lv_obj_t *main;
} lv_label_outline8_handle_t;

#define LV_LABEL_OUTLINE8_MODE_TRANSPARENT_CENTER  0
#define LV_LABEL_OUTLINE8_MODE_OPAQUE_FILL         1

/** 创建 8 向描边 label；max_width>0 时超宽自动换行；cursor_img 非空时打字机中更新其位置到“下一个字”处 */
void *lv_label_create_with_outline8(lv_obj_t *parent, const lv_label_segment_t *segments,
    uint32_t segment_count, lv_coord_t max_width, lv_coord_t offset_px, lv_color_t outline_color, lv_opa_t outline_opa,
    uint8_t mode, lv_color_t fill_color, uint32_t typing_duration_ms, lv_obj_t *cursor_img);

/** 获取实际使用高度（换行后总高度） */
lv_coord_t lv_label_with_outline8_get_height(void *h);

void lv_label_with_outline8_set_text(void *h, const char *text);
void lv_label_with_outline8_set_segments(void *h, const lv_label_segment_t *segments, uint32_t segment_count);
void lv_label_with_outline8_delete(void *h);

typedef struct {
    lv_coord_t x;
    lv_coord_t y;
    int32_t angle_01;   /* 0.1 度单位，如 900=90° */
} lv_scatter_char_start_t;

/* ========== 描边 + 柔光（多层 label：外圈半透明白光晕 + 内层深色描边 + 白填充） ========== */
typedef struct {
    lv_obj_t *cont;
    lv_obj_t *main;
} lv_label_stroke_glow_handle_t;

/** 创建描边+柔光 label；canvas 圆周密集采样连续包边（非 8 向 label 描边）；fill_opa=0 仅外描边 */
void *lv_label_create_with_stroke_glow(lv_obj_t *parent, const lv_label_segment_t *segments,
    uint32_t segment_count, lv_coord_t max_width, lv_coord_t stroke_px, lv_color_t stroke_color,
    lv_opa_t stroke_opa, lv_color_t fill_color, lv_opa_t fill_opa,
    lv_coord_t glow_max_px, lv_color_t glow_color, lv_opa_t glow_opa, uint32_t typing_duration_ms);

lv_coord_t lv_label_with_stroke_glow_get_height(void *h);
void lv_label_with_stroke_glow_delete(void *h);

/** 每字飞入 scatter，字形使用 lv_label_create_with_stroke_glow */
void *lv_label_stroke_glow_scatter_fly_in_create(lv_obj_t *parent, const lv_label_segment_t *segments,
    uint32_t segment_count, lv_coord_t max_width, lv_coord_t stroke_px, lv_color_t stroke_color,
    lv_opa_t stroke_opa, lv_color_t fill_color, lv_opa_t fill_opa,
    lv_coord_t glow_max_px, lv_color_t glow_color, lv_opa_t glow_opa, lv_coord_t row_x, lv_coord_t row_y, uint32_t duration_ms, uint32_t delay_per_char_ms,
    const lv_scatter_char_start_t *starts, int32_t arc_rotate_angle_01, lv_coord_t arc_radius,
    int16_t arc_center_deg, uint16_t arc_spread_deg);

lv_coord_t lv_label_stroke_glow_scatter_fly_in_get_height(void *handle);
lv_coord_t lv_label_stroke_glow_scatter_fly_in_get_total_width(void *handle);
void lv_label_stroke_glow_scatter_fly_in_delete(void *handle);
void lv_label_stroke_glow_scatter_fly_in_finish_now(void *handle);

/* ========== 每字独立旋转飞入、最终排成一行/多行（立体描边 outline8） ========== */

/** 创建每字飞入效果；max_width>0 时按行宽与断字换行；starts 非空时每字起始 (x,y,angle) 从 starts 取 */
void *lv_label_outline8_scatter_fly_in_create(lv_obj_t *parent, const lv_label_segment_t *segments, uint32_t segment_count,
    lv_coord_t max_width, lv_coord_t offset_px, lv_color_t outline_color, lv_opa_t outline_opa, uint8_t mode, lv_color_t fill_color,
    lv_coord_t row_x, lv_coord_t row_y, uint32_t duration_ms, uint32_t delay_per_char_ms,
    const lv_scatter_char_start_t *starts, int32_t arc_rotate_angle_01,
    lv_coord_t arc_radius, int16_t arc_center_deg, uint16_t arc_spread_deg);

/** 获取 scatter 飞入实际占用高度（换行后总高度） */
lv_coord_t lv_label_outline8_scatter_fly_in_get_height(void *handle);

/** 获取整句单行总宽度（不换行时的像素宽），用于调试换行原因 */
lv_coord_t lv_label_outline8_scatter_fly_in_get_total_width(void *handle);

void lv_label_outline8_scatter_fly_in_delete(void *handle);

/** 若当前句仍在飞入中：立即结束飞入动画，把所有字移到最终位置（用于新句到来时先定格再取快照） */
void lv_label_outline8_scatter_fly_in_finish_now(void *handle);

/* ========== 音乐律动透明控件（LOW / MID / HIGH 三频段精灵 + 能量柱 + 粒子） ========== */
typedef struct {
    uint8_t low;     /* 低频能量，范围 0..100 */
    uint8_t mid;     /* 中频能量，范围 0..100 */
    uint8_t high;    /* 高频能量，范围 0..100 */
} divoom_lvgl_beat_creatures_energy_t;

/** 创建音乐律动控件。根对象为透明底，适合叠加在表盘或音乐页任意背景上。 */
void *divoom_lvgl_beat_creatures_create(lv_obj_t *parent, lv_coord_t x, lv_coord_t y,
    lv_coord_t w, lv_coord_t h);

/** 获取控件根对象，调用方可继续设置层级或对齐。 */
lv_obj_t *divoom_lvgl_beat_creatures_get_obj(void *handle);

/** 设置三频段能量；不同频段使用不同精灵和不同动作幅度。 */
void divoom_lvgl_beat_creatures_set_energy(void *handle,
    const divoom_lvgl_beat_creatures_energy_t *energy);

/** 开关自动演示律动。真实音乐输入接入时应关闭，由 set_energy 驱动。 */
void divoom_lvgl_beat_creatures_set_auto_demo(void *handle, uint8_t enable);

/** 删除控件并释放内部 timer 与状态。 */
void divoom_lvgl_beat_creatures_delete(void *handle);

#ifdef __cplusplus
}
#endif

#endif /* DIVOOM_LVGL_MUSIC_EFFECT_H */

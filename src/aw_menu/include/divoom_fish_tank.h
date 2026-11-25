#ifndef _divoom_fish_tank_h_
#define _divoom_fish_tank_h_

#include "lvgl.h"
#include "divoom_type.h"

/*
 * 赛博鱼缸自治游动控件（lv_obj_class 子类）：
 * - create 后用 lv_obj_set_pos / lv_obj_set_size 摆放，content area 即游动区域。
 * - 内部单张 lv_img 精灵 + steering 定时器；向右 PNG 多摆尾动作帧表自动切帧。
 * - 姿态资源只表示不同摆尾动作；向左由内部镜像生成，上/下游只做有限俯仰。
 * - 运动学：heading + speed（恒>=0），鱼不后退。
 */

#define DIVOOM_FISH_TANK_IMG_PATH_MAX       96

typedef struct
{
    lv_coord_t margin;
    lv_coord_t fish_w;
    lv_coord_t fish_h;
    float v_min;
    float v_max;
    float accel_max;
    float turn_rate_deg;
    float arrive_radius;
    float wall_influence;
    float dwell_chance;
    uint32_t dwell_min_ms;
    uint32_t dwell_max_ms;
    float dart_chance;
    uint32_t dart_ms;
    float max_pitch_deg;            /* 俯仰旋转上限（度），写 lv_img_set_angle */
} divoom_fish_tank_params_t;

void divoom_fish_tank_params_init_default(divoom_fish_tank_params_t *p);

lv_obj_t *divoom_fish_tank_create(lv_obj_t *parent);
void divoom_fish_tank_set_seed(lv_obj_t *obj, uint32_t seed);
void divoom_fish_tank_set_params(lv_obj_t *obj, const divoom_fish_tank_params_t *p);
/* 可选：覆盖旧单帧 PNG；当前右向素材模式只使用 img_right，img_left 保留兼容。 */
void divoom_fish_tank_set_sprite(lv_obj_t *obj, const char *img_right, const char *img_left);
void divoom_fish_tank_set_active(lv_obj_t *obj, uint8 active);

#endif

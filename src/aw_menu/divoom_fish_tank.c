/*
 * divoom_fish_tank：赛博鱼缸自治游动 LVGL 自定义控件。
 * 单张 lv_img 精灵 + 右向摆尾动作帧表 + 内部左向镜像 + 有限俯仰。
 */
#include "divoom_fish_tank.h"
#include "divoom_menu_control.h"
#include "divoom_log.h"
#include "divoom.h"
#include "middle/include/divoom_lvgl9_compat.h"
#include "divoom_pixel_format.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define DIVOOM_FISH_TANK_CLASS                &divoom_fish_tank_class
#define DIVOOM_FISH_TANK_TICK_MS              30
#define DIVOOM_FISH_TANK_IMG_R_DEFAULT        "fish_tank/fish_tank_fish_r.png"
#define DIVOOM_FISH_TANK_IMG_L_DEFAULT        "fish_tank/fish_tank_fish_l.png"
#define DIVOOM_FISH_TANK_IMAGE_BASE_PATH      "A:/usr/share/divoom_app/ui/"
#define DIVOOM_FISH_TANK_ANIM_ACTION_MAX      4
#define DIVOOM_FISH_TANK_ANIM_FRAME_MAX       8
#define DIVOOM_FISH_TANK_ACTION_MIN_MS        1500u
#define DIVOOM_FISH_TANK_ACTION_MAX_MS        3500u
#define DIVOOM_FISH_TANK_FRAME_MS_DWELL       220u
#define DIVOOM_FISH_TANK_FRAME_MS_MIN         70u
#define DIVOOM_FISH_TANK_FRAME_MS_MAX         170u
#define DIVOOM_FISH_TANK_FALLBACK_W           40
#define DIVOOM_FISH_TANK_FALLBACK_H           40
#define DIVOOM_FISH_TANK_FRAME_CACHE_MAX      32
#define DIVOOM_FISH_TANK_Q8_SHIFT             8
#define DIVOOM_FISH_TANK_Q8_ONE               (1 << DIVOOM_FISH_TANK_Q8_SHIFT)
#define DIVOOM_FISH_TANK_TRIG_Q14             14
#define DIVOOM_FISH_TANK_TRIG_ONE             (1 << DIVOOM_FISH_TANK_TRIG_Q14)
#define DIVOOM_FISH_TANK_ANGLE_FULL           4096
#define DIVOOM_FISH_TANK_ANGLE_HALF           2048
#define DIVOOM_FISH_TANK_ANGLE_QUARTER        1024
#define DIVOOM_FISH_TANK_PROB_ONE             32768u
#define DIVOOM_FISH_TANK_ANGLE_ABS_BACK       1126
#define DIVOOM_FISH_TANK_ANGLE_ABS_SLOW_1     614
#define DIVOOM_FISH_TANK_ANGLE_ABS_SLOW_2     307
#define DIVOOM_FISH_TANK_ANGLE_DART_OK        717
#define DIVOOM_FISH_TANK_ANGLE_DART_START     922
#define DIVOOM_FISH_TANK_ANGLE_MIN_SPEED      451
#define DIVOOM_FISH_TANK_BUBBLE_MAX           8
#define DIVOOM_FISH_TANK_BUBBLE_BOTTOM_MIN_MS 1500u
#define DIVOOM_FISH_TANK_BUBBLE_BOTTOM_MAX_MS 3200u
#define DIVOOM_FISH_TANK_BUBBLE_FISH_PROB_MS  1u
#define DIVOOM_FISH_TANK_BUBBLE_TURN_PROB_MS  3u
#define DIVOOM_FISH_TANK_DECOR_MAX            14
#define DIVOOM_FISH_TANK_DECOR_FRAME_MAX      16
#define DIVOOM_FISH_TANK_DECOR_UPDATE_MS      30u
#define DIVOOM_FISH_TANK_DECOR_PAD            "", "", "", "", "", "", "", "", "", "", "", "", "", "", ""

typedef struct
{
    lv_obj_t *obj;
    int32_t x_q8;
    int32_t y_q8;
    int32_t drift_q8;
    int32_t speed_q8;
    int32_t accel_q8_s2;
    int32_t terminal_q8;
    uint16_t age_ms;
    uint16_t life_ms;
    uint16_t phase_ang;
    uint16_t phase_step;
    uint8 radius;
    uint8 opacity;
    uint8 active;
} divoom_fish_tank_bubble_t;

typedef struct
{
    const char *frames[DIVOOM_FISH_TANK_DECOR_FRAME_MAX];
    lv_coord_t w;
    lv_coord_t h;
    lv_coord_t x_offset;
    lv_coord_t bottom_offset;
    uint16_t frame_ms;
    uint16_t sway_amp_deg10;
    uint16_t sway_period_ms;
    uint16_t sway_phase_ang;
    uint8 frame_count;
    uint8 start_frame;
    uint8 anchor_right;
} divoom_fish_tank_decor_cfg_t;

typedef struct
{
    lv_obj_t *obj;
    char full_path[160];
    uint16_t elapsed_ms;
    uint16_t frame_ms_current;
    uint16_t jitter_remain_ms;
    uint16_t sway_phase_ang;
    uint16_t sway_period_current_ms;
    uint16_t sway_jitter_remain_ms;
    uint16_t sway_amp_current_deg10;
    uint8 anim_frame;
    uint8 ready;
} divoom_fish_tank_decor_t;

typedef enum
{
    DIVOOM_FISH_TANK_ANIM_NORMAL = 0,
    DIVOOM_FISH_TANK_ANIM_SOFT,
    DIVOOM_FISH_TANK_ANIM_WIDE,
    DIVOOM_FISH_TANK_ANIM_QUICK,
} divoom_fish_tank_anim_action_t;

typedef struct
{
    char key[160];
    lv_img_dsc_t dsc;
    uint8 used;
} divoom_fish_tank_frame_cache_entry_t;

static divoom_fish_tank_frame_cache_entry_t s_fish_frame_cache[DIVOOM_FISH_TANK_FRAME_CACHE_MAX];

static const divoom_fish_tank_decor_cfg_t s_fish_decor_cfg[DIVOOM_FISH_TANK_DECOR_MAX] = {
    {
        { "fish_tank/tank_water_bg.png", DIVOOM_FISH_TANK_DECOR_PAD },
        396, 356, 0, 0, 1000, 0, 0, 0, 1, 0, 0
    },
    {
        { "fish_tank/rock_left.png", DIVOOM_FISH_TANK_DECOR_PAD },
        92, 44, 44, 6, 1000, 0, 0, 0, 1, 0, 0
    },
    {
        { "fish_tank/weed_l_back_tall.png", DIVOOM_FISH_TANK_DECOR_PAD },
        72, 292, 6, 8, 1000, 28, 4700, 180, 1, 0, 0
    },
    {
        { "fish_tank/weed_l_back_short.png", DIVOOM_FISH_TANK_DECOR_PAD },
        62, 178, 58, 8, 1000, 22, 3900, 1210, 1, 0, 0
    },
    {
        { "fish_tank/weed_l_mid.png", DIVOOM_FISH_TANK_DECOR_PAD },
        66, 210, 92, 4, 1000, 18, 5600, 2440, 1, 0, 0
    },
    {
        { "fish_tank/weed_l_front_tall.png", DIVOOM_FISH_TANK_DECOR_PAD },
        74, 262, 16, 2, 1000, 34, 3300, 2820, 1, 0, 0
    },
    {
        { "fish_tank/weed_l_front_short.png", DIVOOM_FISH_TANK_DECOR_PAD },
        58, 156, 82, 2, 1000, 24, 4300, 650, 1, 0, 0
    },
    {
        { "fish_tank/coral_right.png", DIVOOM_FISH_TANK_DECOR_PAD },
        80, 116, 82, 2, 1000, 0, 0, 0, 1, 0, 1
    },
    {
        { "fish_tank/weed_r_back_tall.png", DIVOOM_FISH_TANK_DECOR_PAD },
        70, 280, 12, 8, 1000, 26, 5100, 910, 1, 0, 1
    },
    {
        { "fish_tank/weed_r_back_short.png", DIVOOM_FISH_TANK_DECOR_PAD },
        58, 168, 72, 8, 1000, 20, 4200, 2160, 1, 0, 1
    },
    {
        { "fish_tank/weed_r_mid.png", DIVOOM_FISH_TANK_DECOR_PAD },
        64, 196, 64, 4, 1000, 22, 5900, 3370, 1, 0, 1
    },
    {
        { "fish_tank/weed_r_front_tall.png", DIVOOM_FISH_TANK_DECOR_PAD },
        70, 242, 10, 2, 1000, 32, 3500, 1570, 1, 0, 1
    },
    {
        { "fish_tank/weed_r_front_short.png", DIVOOM_FISH_TANK_DECOR_PAD },
        54, 148, 82, 2, 1000, 24, 4700, 2990, 1, 0, 1
    },
    {
        { "fish_tank/tank_floor.png", DIVOOM_FISH_TANK_DECOR_PAD },
        396, 58, 0, 0, 1000, 0, 0, 0, 1, 0, 0
    },
};

/*
 * 多摆尾动作资源表：
 * - action 代表一种摆尾动作，不代表方向或上/下游姿态。
 * - 美术只需要提供向右游的 PNG；向左由内存镜像生成，上/下游用有限俯仰表现。
 * - frame 为空字符串表示该帧缺省；整个 action 为空时会自动跳过。
 * - 后续美术补图时，直接把对应路径填到 s_fish_anim_r 二维表即可。
 */
static const char *s_fish_anim_r[DIVOOM_FISH_TANK_ANIM_ACTION_MAX][DIVOOM_FISH_TANK_ANIM_FRAME_MAX] = {
    {
        "fish_tank/space_fish_pose1_r_f0.png",
        "fish_tank/space_fish_pose1_r_f1.png",
        "fish_tank/space_fish_pose1_r_f2.png",
        "fish_tank/space_fish_pose1_r_f3.png",
        "fish_tank/space_fish_pose1_r_f4.png",
        "fish_tank/space_fish_pose1_r_f5.png",
        "fish_tank/space_fish_pose1_r_f6.png",
        "fish_tank/space_fish_pose1_r_f7.png",
    },
    { "", "", "", "", "", "", "", "" },
    {
        "fish_tank/space_fish_pose2_r_f0.png",
        "fish_tank/space_fish_pose2_r_f1.png",
        "fish_tank/space_fish_pose2_r_f2.png",
        "fish_tank/space_fish_pose2_r_f3.png",
        "fish_tank/space_fish_pose2_r_f4.png",
        "fish_tank/space_fish_pose2_r_f5.png",
        "fish_tank/space_fish_pose2_r_f6.png",
        "fish_tank/space_fish_pose2_r_f7.png",
    },
    { "", "", "", "", "", "", "", "" },
};

typedef struct
{
    lv_obj_t obj;
    lv_obj_t *fish_img;
    lv_obj_t *fish_img2;
    lv_obj_t *fish_fallback;
    lv_obj_t *fish_fallback_body;
    lv_obj_t *fish_fallback_tail;
    lv_obj_t *fish_fallback_fin;
    lv_obj_t *fish_fallback_eye;
    divoom_fish_tank_bubble_t bubbles[DIVOOM_FISH_TANK_BUBBLE_MAX];
    divoom_fish_tank_decor_t decor[DIVOOM_FISH_TANK_DECOR_MAX];
    lv_timer_t *steer_timer;
    divoom_fish_tank_params_t params;
    int32_t pos_x_q8;
    int32_t pos_y_q8;
    int32_t target_x_q8;
    int32_t target_y_q8;
    int32_t speed_q8;
    int32_t vel_x_q8;
    int32_t vel_y_q8;
    int32_t pos2_x_q8;
    int32_t pos2_y_q8;
    int32_t target2_x_q8;
    int32_t target2_y_q8;
    int32_t speed2_q8;
    int32_t vel2_x_q8;
    int32_t vel2_y_q8;
    int32_t v_min_q8;
    int32_t v_max_q8;
    int32_t accel_q8_s2;
    int32_t arrive_radius_q8;
    int32_t wall_influence_q8;
    int16_t heading_ang;
    int16_t heading2_ang;
    int16_t turn_rate_ang_s;
    int16_t disp_pitch_deg10;
    int16_t disp2_pitch_deg10;
    int16_t turn_visual_deg10;
    int16_t turn2_visual_deg10;
    int16_t max_pitch_deg10;
    uint16_t dwell_chance_q15;
    uint16_t dart_chance_q15;
    uint8 has_target;
    uint8 has2_target;
    uint8 facing_lr;            /* 1 = 使用右向资源，0 = 使用内部镜像左向资源 */
    uint8 facing2_lr;
    uint8 sprite_lr_applied;    /* 兼容旧资源标记；右向素材模式下固定为 1 */
    uint8 sprite_action_applied;
    uint8 sprite_frame_applied;
    uint8 sprite2_lr_applied;
    uint8 sprite2_action_applied;
    uint8 sprite2_frame_applied;
    uint8 anim_action;
    uint8 anim_frame;
    uint8 anim2_action;
    uint8 anim2_frame;
    uint8 cruise_pct;
    uint8 cruise2_pct;
    uint32_t anim_elapsed_ms;
    uint32_t anim2_elapsed_ms;
    uint32_t action_remain_ms;
    uint32_t action2_remain_ms;
    uint32_t dwell_remain_ms;
    uint32_t dwell2_remain_ms;
    uint32_t dart_remain_ms;
    uint32_t dart2_remain_ms;
    uint32_t cruise_remain_ms;
    uint32_t cruise2_remain_ms;
    uint32_t rand_seed;
    uint32_t bob_phase_ms;
    uint32_t bubble_spawn_ms;
    uint32_t decor_elapsed_ms;
    char img_r[DIVOOM_FISH_TANK_IMG_PATH_MAX];
    char img_l[DIVOOM_FISH_TANK_IMG_PATH_MAX];
    lv_coord_t area_w;
    lv_coord_t area_h;
    uint8 inner_ready;
    uint8 active;
    uint8 styles_inited;
    uint8 root_style_added;
    lv_style_t root_style;
} divoom_fish_tank_t;

static void divoom_fish_tank_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj);
static void divoom_fish_tank_destructor(const lv_obj_class_t *class_p, lv_obj_t *obj);
static void divoom_fish_tank_event_cb(lv_event_t *e);
static void divoom_fish_tank_steering_tick(lv_timer_t *timer);
static int divoom_fish_tank_ensure_inner(divoom_fish_tank_t *ph);
static void divoom_fish_tank_update_area_from_obj(divoom_fish_tank_t *ph, lv_obj_t *obj);
static void divoom_fish_tank_apply_fish_visual(divoom_fish_tank_t *ph);
static void divoom_fish_tank_apply_fish_src(divoom_fish_tank_t *ph, const char *src, uint8 flip_x);
static int divoom_fish_tank_apply_img_src(divoom_fish_tank_t *ph, lv_obj_t *img,
    const char *src, uint8 flip_x);
static void divoom_fish_tank_steering_step(divoom_fish_tank_t *ph, uint32_t dt_ms);
static void divoom_fish_tank_pick_target(divoom_fish_tank_t *ph);
static void divoom_fish_tank_update_facing(divoom_fish_tank_t *ph);
static void divoom_fish_tank_update_anim(divoom_fish_tank_t *ph, uint32_t dt_ms);
static void divoom_fish_tank_params_sync_fixed(divoom_fish_tank_t *ph);
static int divoom_fish_tank_ensure_bubbles(divoom_fish_tank_t *ph, lv_obj_t *root);
static void divoom_fish_tank_update_bubbles(divoom_fish_tank_t *ph, uint32_t dt_ms);
static void divoom_fish_tank_reset_bubbles(divoom_fish_tank_t *ph);
static int divoom_fish_tank_ensure_decor(divoom_fish_tank_t *ph, lv_obj_t *root);
static void divoom_fish_tank_layout_decor(divoom_fish_tank_t *ph);
static void divoom_fish_tank_update_decor(divoom_fish_tank_t *ph, uint32_t dt_ms);
static void divoom_fish_tank_second_init(divoom_fish_tank_t *ph);
static void divoom_fish_tank_second_step(divoom_fish_tank_t *ph, uint32_t dt_ms);
static void divoom_fish_tank_second_apply_visual(divoom_fish_tank_t *ph);

const lv_obj_class_t divoom_fish_tank_class = {
    .constructor_cb = divoom_fish_tank_constructor,
    .destructor_cb = divoom_fish_tank_destructor,
    .width_def = 360,
    .height_def = 320,
    .instance_size = sizeof(divoom_fish_tank_t),
    .base_class = &lv_obj_class,
};

void divoom_fish_tank_params_init_default(divoom_fish_tank_params_t *p)
{
    if (!p)
    {
        return;
    }

    memset(p, 0, sizeof(*p));
    p->margin = 8;
    p->fish_w = 40;
    p->fish_h = 40;
    p->v_min = 14.0f;
    p->v_max = 52.0f;
    p->accel_max = 78.0f;
    p->turn_rate_deg = 160.0f;
    p->arrive_radius = 20.0f;
    p->wall_influence = 32.0f;
    p->dwell_chance = 0.16f;
    p->dwell_min_ms = 500;
    p->dwell_max_ms = 2000;
    p->dart_chance = 0.04f;
    p->dart_ms = 400;
    p->max_pitch_deg = 28.0f;
}

static divoom_fish_tank_t *divoom_fish_tank_get_obj(lv_obj_t *obj)
{
    if (!obj)
    {
        return NULL;
    }

    return (divoom_fish_tank_t *)obj;
}

static int32_t divoom_fish_tank_iabs32(int32_t v)
{
    return v < 0 ? -v : v;
}

static int32_t divoom_fish_tank_q8_from_i32(int32_t v)
{
    return v << DIVOOM_FISH_TANK_Q8_SHIFT;
}

static int32_t divoom_fish_tank_q8_from_float(float v)
{
    return (int32_t)((v * (float)DIVOOM_FISH_TANK_Q8_ONE) + (v >= 0.0f ? 0.5f : -0.5f));
}

static int16_t divoom_fish_tank_deg_to_angle(float deg)
{
    return (int16_t)((deg * (float)DIVOOM_FISH_TANK_ANGLE_FULL / 360.0f) +
        (deg >= 0.0f ? 0.5f : -0.5f));
}

static int16_t divoom_fish_tank_deg_to_deg10(float deg)
{
    return (int16_t)((deg * 10.0f) + (deg >= 0.0f ? 0.5f : -0.5f));
}

static uint16_t divoom_fish_tank_prob_to_q15(float p)
{
    if (p <= 0.0f)
    {
        return 0;
    }
    if (p >= 1.0f)
    {
        return (uint16_t)DIVOOM_FISH_TANK_PROB_ONE;
    }
    return (uint16_t)((p * (float)DIVOOM_FISH_TANK_PROB_ONE) + 0.5f);
}

static lv_coord_t divoom_fish_tank_q8_to_coord(int32_t v)
{
    if (v >= 0)
    {
        return (lv_coord_t)((v + (DIVOOM_FISH_TANK_Q8_ONE / 2)) >> DIVOOM_FISH_TANK_Q8_SHIFT);
    }
    return (lv_coord_t)-(((-v) + (DIVOOM_FISH_TANK_Q8_ONE / 2)) >> DIVOOM_FISH_TANK_Q8_SHIFT);
}

static int16_t divoom_fish_tank_normalize_angle(int32_t a)
{
    while (a >= DIVOOM_FISH_TANK_ANGLE_HALF)
    {
        a -= DIVOOM_FISH_TANK_ANGLE_FULL;
    }
    while (a < -DIVOOM_FISH_TANK_ANGLE_HALF)
    {
        a += DIVOOM_FISH_TANK_ANGLE_FULL;
    }
    return (int16_t)a;
}

static uint64_t divoom_fish_tank_isqrt64(uint64_t v)
{
    uint64_t bit;
    uint64_t res;

    bit = (uint64_t)1 << 62;
    while (bit > v)
    {
        bit >>= 2;
    }

    res = 0;
    while (bit != 0)
    {
        if (v >= res + bit)
        {
            v -= res + bit;
            res = (res >> 1) + bit;
        }
        else
        {
            res >>= 1;
        }
        bit >>= 2;
    }

    return res;
}

static uint64_t divoom_fish_tank_dist_sq_q8(int32_t dx, int32_t dy)
{
    int64_t x;
    int64_t y;

    x = dx;
    y = dy;
    return (uint64_t)(x * x + y * y);
}

static int32_t divoom_fish_tank_dist_q8(int32_t dx, int32_t dy)
{
    return (int32_t)divoom_fish_tank_isqrt64(divoom_fish_tank_dist_sq_q8(dx, dy));
}

static const int16_t s_fish_cordic_atan[11] = {
    512, 302, 160, 81, 41, 20, 10, 5, 3, 1, 1
};

static void divoom_fish_tank_sincos_q14(int16_t angle, int32_t *sin_q14, int32_t *cos_q14)
{
    int32_t x;
    int32_t y;
    int32_t z;
    int32_t x_new;
    int32_t y_new;
    int32_t a;
    uint8 invert;
    int i;

    a = angle % DIVOOM_FISH_TANK_ANGLE_FULL;
    if (a < 0)
    {
        a += DIVOOM_FISH_TANK_ANGLE_FULL;
    }
    if (a >= DIVOOM_FISH_TANK_ANGLE_HALF)
    {
        a -= DIVOOM_FISH_TANK_ANGLE_FULL;
    }

    invert = 0;
    if (a > DIVOOM_FISH_TANK_ANGLE_QUARTER)
    {
        a -= DIVOOM_FISH_TANK_ANGLE_HALF;
        invert = 1;
    }
    else if (a < -DIVOOM_FISH_TANK_ANGLE_QUARTER)
    {
        a += DIVOOM_FISH_TANK_ANGLE_HALF;
        invert = 1;
    }

    x = 9949;
    y = 0;
    z = a;
    for (i = 0; i < (int)(sizeof(s_fish_cordic_atan) / sizeof(s_fish_cordic_atan[0])); i++)
    {
        if (z >= 0)
        {
            x_new = x - (y >> i);
            y_new = y + (x >> i);
            z -= s_fish_cordic_atan[i];
        }
        else
        {
            x_new = x + (y >> i);
            y_new = y - (x >> i);
            z += s_fish_cordic_atan[i];
        }
        x = x_new;
        y = y_new;
    }

    if (invert)
    {
        x = -x;
        y = -y;
    }

    if (sin_q14)
    {
        *sin_q14 = y;
    }
    if (cos_q14)
    {
        *cos_q14 = x;
    }
}

static int16_t divoom_fish_tank_atan2_angle(int32_t y, int32_t x)
{
    int32_t x_new;
    int32_t y_new;
    int32_t z;
    int i;

    if (x == 0 && y == 0)
    {
        return 0;
    }
    if (x == 0)
    {
        return y >= 0 ? DIVOOM_FISH_TANK_ANGLE_QUARTER : -DIVOOM_FISH_TANK_ANGLE_QUARTER;
    }
    if (x < 0)
    {
        if (y >= 0)
        {
            return divoom_fish_tank_normalize_angle(
                divoom_fish_tank_atan2_angle(-y, -x) + DIVOOM_FISH_TANK_ANGLE_HALF);
        }
        return divoom_fish_tank_normalize_angle(
            divoom_fish_tank_atan2_angle(-y, -x) - DIVOOM_FISH_TANK_ANGLE_HALF);
    }

    z = 0;
    for (i = 0; i < (int)(sizeof(s_fish_cordic_atan) / sizeof(s_fish_cordic_atan[0])); i++)
    {
        if (y > 0)
        {
            x_new = x + (y >> i);
            y_new = y - (x >> i);
            z += s_fish_cordic_atan[i];
        }
        else
        {
            x_new = x - (y >> i);
            y_new = y + (x >> i);
            z -= s_fish_cordic_atan[i];
        }
        x = x_new;
        y = y_new;
    }

    return divoom_fish_tank_normalize_angle(z);
}

static uint32_t divoom_fish_tank_rand_u32(divoom_fish_tank_t *ph)
{
    ph->rand_seed = ph->rand_seed * 1103515245u + 12345u;
    return ph->rand_seed;
}

static int32_t divoom_fish_tank_rand_range_i32(divoom_fish_tank_t *ph, int32_t lo, int32_t hi)
{
    uint32_t span;

    if (hi <= lo)
    {
        return lo;
    }

    span = (uint32_t)(hi - lo + 1);
    return lo + (int32_t)(divoom_fish_tank_rand_u32(ph) % span);
}

static int32_t divoom_fish_tank_rand_range_q8(divoom_fish_tank_t *ph, int32_t lo, int32_t hi)
{
    return divoom_fish_tank_rand_range_i32(ph, lo, hi);
}

static uint32_t divoom_fish_tank_rand_range_u32(divoom_fish_tank_t *ph, uint32_t lo, uint32_t hi)
{
    uint32_t span;

    if (hi <= lo)
    {
        return lo;
    }

    span = hi - lo + 1u;
    return lo + (divoom_fish_tank_rand_u32(ph) % span);
}

static void divoom_fish_tank_params_sync_fixed(divoom_fish_tank_t *ph)
{
    if (!ph)
    {
        return;
    }

    ph->v_min_q8 = divoom_fish_tank_q8_from_float(ph->params.v_min);
    ph->v_max_q8 = divoom_fish_tank_q8_from_float(ph->params.v_max);
    ph->accel_q8_s2 = divoom_fish_tank_q8_from_float(ph->params.accel_max);
    ph->arrive_radius_q8 = divoom_fish_tank_q8_from_float(ph->params.arrive_radius);
    ph->wall_influence_q8 = divoom_fish_tank_q8_from_float(ph->params.wall_influence);
    ph->turn_rate_ang_s = divoom_fish_tank_deg_to_angle(ph->params.turn_rate_deg);
    ph->dwell_chance_q15 = divoom_fish_tank_prob_to_q15(ph->params.dwell_chance);
    ph->dart_chance_q15 = divoom_fish_tank_prob_to_q15(ph->params.dart_chance);
    ph->max_pitch_deg10 = divoom_fish_tank_deg_to_deg10(ph->params.max_pitch_deg);

    if (ph->v_min_q8 < 0)
    {
        ph->v_min_q8 = 0;
    }
    if (ph->v_max_q8 < ph->v_min_q8)
    {
        ph->v_max_q8 = ph->v_min_q8;
    }
    if (ph->accel_q8_s2 < 0)
    {
        ph->accel_q8_s2 = 0;
    }
    if (ph->arrive_radius_q8 < divoom_fish_tank_q8_from_i32(6))
    {
        ph->arrive_radius_q8 = divoom_fish_tank_q8_from_i32(6);
    }
    if (ph->wall_influence_q8 < 0)
    {
        ph->wall_influence_q8 = 0;
    }
    if (ph->turn_rate_ang_s < 0)
    {
        ph->turn_rate_ang_s = 0;
    }
    if (ph->max_pitch_deg10 < 40)
    {
        ph->max_pitch_deg10 = 40;
    }
    if (ph->max_pitch_deg10 > 350)
    {
        ph->max_pitch_deg10 = 350;
    }
}

static int divoom_fish_tank_anim_frame_count(uint8 action)
{
    int i;
    int count;

    if (action >= DIVOOM_FISH_TANK_ANIM_ACTION_MAX)
    {
        return 0;
    }

    count = 0;
    for (i = 0; i < DIVOOM_FISH_TANK_ANIM_FRAME_MAX; i++)
    {
        if (s_fish_anim_r[action][i] && s_fish_anim_r[action][i][0] != '\0')
        {
            count = i + 1;
        }
    }

    return count;
}

static uint8 divoom_fish_tank_anim_action_available(uint8 action)
{
    return divoom_fish_tank_anim_frame_count(action) > 0 ? 1 : 0;
}

static const char *divoom_fish_tank_anim_frame_path(divoom_fish_tank_t *ph, uint8 action, uint8 frame)
{
    const char *src;

    if (!ph)
    {
        return DIVOOM_FISH_TANK_IMG_R_DEFAULT;
    }

    if (action >= DIVOOM_FISH_TANK_ANIM_ACTION_MAX)
    {
        action = DIVOOM_FISH_TANK_ANIM_NORMAL;
    }

    if (frame >= DIVOOM_FISH_TANK_ANIM_FRAME_MAX)
    {
        frame = 0;
    }

    src = s_fish_anim_r[action][frame];
    if (!src || src[0] == '\0')
    {
        src = s_fish_anim_r[action][0];
    }

    if ((!src || src[0] == '\0') && action != DIVOOM_FISH_TANK_ANIM_NORMAL)
    {
        src = s_fish_anim_r[DIVOOM_FISH_TANK_ANIM_NORMAL][0];
    }

    if (!src || src[0] == '\0')
    {
        return ph->img_r;
    }

    if (action == DIVOOM_FISH_TANK_ANIM_NORMAL && frame == 0)
    {
        if (strcmp(src, DIVOOM_FISH_TANK_IMG_R_DEFAULT) == 0)
        {
            return ph->img_r;
        }
    }

    return src;
}

static void divoom_fish_tank_set_anim_action(divoom_fish_tank_t *ph, uint8 action)
{
    int frame_count;

    if (!ph || !divoom_fish_tank_anim_action_available(action))
    {
        return;
    }

    frame_count = divoom_fish_tank_anim_frame_count(action);
    if (frame_count <= 0)
    {
        return;
    }

    if (ph->anim_action != action)
    {
        ph->anim_action = action;
        ph->anim_frame = (uint8)(ph->anim_frame % frame_count);
        ph->anim_elapsed_ms = 0;
        ph->sprite_action_applied = 0xFF;
        ph->sprite_frame_applied = 0xFF;
    }

    ph->action_remain_ms = divoom_fish_tank_rand_range_u32(ph,
        DIVOOM_FISH_TANK_ACTION_MIN_MS, DIVOOM_FISH_TANK_ACTION_MAX_MS);
}

static uint8 divoom_fish_tank_pick_random_anim_action(divoom_fish_tank_t *ph)
{
    uint8 avail[DIVOOM_FISH_TANK_ANIM_ACTION_MAX];
    uint8 count;
    uint8 i;
    uint8 idx;

    if (!ph)
    {
        return DIVOOM_FISH_TANK_ANIM_NORMAL;
    }

    count = 0;
    for (i = 0; i < DIVOOM_FISH_TANK_ANIM_ACTION_MAX; i++)
    {
        if (i == DIVOOM_FISH_TANK_ANIM_QUICK)
        {
            continue;
        }

        if (divoom_fish_tank_anim_action_available(i))
        {
            avail[count] = i;
            count++;
        }
    }

    if (count == 0 && divoom_fish_tank_anim_action_available(DIVOOM_FISH_TANK_ANIM_QUICK))
    {
        avail[count] = DIVOOM_FISH_TANK_ANIM_QUICK;
        count++;
    }

    if (count == 0)
    {
        return DIVOOM_FISH_TANK_ANIM_NORMAL;
    }

    idx = (uint8)(divoom_fish_tank_rand_u32(ph) % count);
    return avail[idx];
}

static void divoom_fish_tank_update_anim_action(divoom_fish_tank_t *ph, uint32_t dt_ms)
{
    uint8 preferred;
    uint8 next_action;

    if (!ph)
    {
        return;
    }

    preferred = 0xFF;
    if (ph->dart_remain_ms > 0 && divoom_fish_tank_anim_action_available(DIVOOM_FISH_TANK_ANIM_QUICK))
    {
        preferred = DIVOOM_FISH_TANK_ANIM_QUICK;
    }
    else if (ph->dwell_remain_ms > 0 && divoom_fish_tank_anim_action_available(DIVOOM_FISH_TANK_ANIM_SOFT))
    {
        preferred = DIVOOM_FISH_TANK_ANIM_SOFT;
    }

    if (preferred != 0xFF)
    {
        if (ph->anim_action != preferred)
        {
            divoom_fish_tank_set_anim_action(ph, preferred);
        }
        return;
    }

    if (!divoom_fish_tank_anim_action_available(ph->anim_action))
    {
        next_action = divoom_fish_tank_pick_random_anim_action(ph);
        divoom_fish_tank_set_anim_action(ph, next_action);
        return;
    }

    if (ph->anim_action == DIVOOM_FISH_TANK_ANIM_QUICK)
    {
        next_action = divoom_fish_tank_pick_random_anim_action(ph);
        divoom_fish_tank_set_anim_action(ph, next_action);
        return;
    }

    if (ph->action_remain_ms > dt_ms)
    {
        ph->action_remain_ms -= dt_ms;
        return;
    }

    next_action = divoom_fish_tank_pick_random_anim_action(ph);
    divoom_fish_tank_set_anim_action(ph, next_action);
}

static uint32_t divoom_fish_tank_anim_frame_ms(divoom_fish_tank_t *ph)
{
    int32_t span;
    int32_t speed_over_min;
    uint32_t ms;

    if (!ph || ph->dwell_remain_ms > 0)
    {
        return DIVOOM_FISH_TANK_FRAME_MS_DWELL;
    }

    if (ph->dart_remain_ms > 0)
    {
        return DIVOOM_FISH_TANK_FRAME_MS_MIN;
    }

    span = ph->v_max_q8 - ph->v_min_q8;
    if (span <= DIVOOM_FISH_TANK_Q8_ONE)
    {
        return (DIVOOM_FISH_TANK_FRAME_MS_MIN + DIVOOM_FISH_TANK_FRAME_MS_MAX) / 2u;
    }

    speed_over_min = ph->speed_q8 - ph->v_min_q8;
    if (speed_over_min < 0)
    {
        speed_over_min = 0;
    }
    if (speed_over_min > span)
    {
        speed_over_min = span;
    }

    ms = DIVOOM_FISH_TANK_FRAME_MS_MAX -
        (uint32_t)(((uint64_t)(DIVOOM_FISH_TANK_FRAME_MS_MAX - DIVOOM_FISH_TANK_FRAME_MS_MIN) *
        (uint32_t)speed_over_min) / (uint32_t)span);
    if (ms < DIVOOM_FISH_TANK_FRAME_MS_MIN)
    {
        ms = DIVOOM_FISH_TANK_FRAME_MS_MIN;
    }

    return ms;
}

static void divoom_fish_tank_update_anim(divoom_fish_tank_t *ph, uint32_t dt_ms)
{
    int frame_count;
    uint32_t frame_ms;

    if (!ph)
    {
        return;
    }

    divoom_fish_tank_update_anim_action(ph, dt_ms);
    frame_count = divoom_fish_tank_anim_frame_count(ph->anim_action);
    if (frame_count <= 0)
    {
        ph->anim_action = DIVOOM_FISH_TANK_ANIM_NORMAL;
        frame_count = divoom_fish_tank_anim_frame_count(ph->anim_action);
    }

    if (frame_count <= 1)
    {
        ph->anim_frame = 0;
        return;
    }

    frame_ms = divoom_fish_tank_anim_frame_ms(ph);
    ph->anim_elapsed_ms += dt_ms;
    while (ph->anim_elapsed_ms >= frame_ms)
    {
        ph->anim_elapsed_ms -= frame_ms;
        ph->anim_frame = (uint8)((ph->anim_frame + 1) % frame_count);
        ph->sprite_frame_applied = 0xFF;
    }
}

static void divoom_fish_tank_sync_vel(divoom_fish_tank_t *ph)
{
    int32_t sin_q14;
    int32_t cos_q14;

    if (!ph)
    {
        return;
    }

    divoom_fish_tank_sincos_q14(ph->heading_ang, &sin_q14, &cos_q14);
    ph->vel_x_q8 = (int32_t)(((int64_t)cos_q14 * ph->speed_q8) >> DIVOOM_FISH_TANK_TRIG_Q14);
    ph->vel_y_q8 = (int32_t)(((int64_t)sin_q14 * ph->speed_q8) >> DIVOOM_FISH_TANK_TRIG_Q14);
}

static void divoom_fish_tank_clamp_pos(divoom_fish_tank_t *ph)
{
    int32_t min_x;
    int32_t min_y;
    int32_t max_x;
    int32_t max_y;

    if (!ph || ph->area_w <= 0 || ph->area_h <= 0)
    {
        return;
    }

    min_x = divoom_fish_tank_q8_from_i32(ph->params.margin);
    min_y = divoom_fish_tank_q8_from_i32(ph->params.margin);
    max_x = divoom_fish_tank_q8_from_i32(ph->area_w - ph->params.fish_w - ph->params.margin);
    max_y = divoom_fish_tank_q8_from_i32(ph->area_h - ph->params.fish_h - ph->params.margin);

    if (max_x < min_x)
    {
        max_x = min_x;
    }

    if (max_y < min_y)
    {
        max_y = min_y;
    }

    if (ph->pos_x_q8 < min_x)
    {
        ph->pos_x_q8 = min_x;
    }

    if (ph->pos_x_q8 > max_x)
    {
        ph->pos_x_q8 = max_x;
    }

    if (ph->pos_y_q8 < min_y)
    {
        ph->pos_y_q8 = min_y;
    }

    if (ph->pos_y_q8 > max_y)
    {
        ph->pos_y_q8 = max_y;
    }
}

static void divoom_fish_tank_update_facing(divoom_fish_tank_t *ph)
{
    int32_t sin_q14;
    int32_t cos_q14;

    if (!ph)
    {
        return;
    }

    if (ph->speed_q8 < divoom_fish_tank_q8_from_i32(3))
    {
        return;
    }

    divoom_fish_tank_sincos_q14(ph->heading_ang, &sin_q14, &cos_q14);
    (void)sin_q14;
    if (cos_q14 > 1966)
    {
        ph->facing_lr = 1;
    }
    else if (cos_q14 < -1966)
    {
        ph->facing_lr = 0;
    }
}

static void divoom_fish_tank_debug_log(const char *msg)
{
    FILE *f;

    if (!msg)
    {
        return;
    }

    f = divoom_port_file_open("resource/userdata/debug.txt", "a");
    if (f)
    {
        fprintf(f, "%s\n", msg);
        divoom_port_file_close(f);
    }
}

static divoom_fish_tank_frame_cache_entry_t *divoom_fish_tank_frame_cache_find(const char *key)
{
    int i;

    if (!key || key[0] == '\0')
    {
        return NULL;
    }

    for (i = 0; i < DIVOOM_FISH_TANK_FRAME_CACHE_MAX; i++)
    {
        if (s_fish_frame_cache[i].used && strcmp(s_fish_frame_cache[i].key, key) == 0)
        {
            return &s_fish_frame_cache[i];
        }
    }

    return NULL;
}

static void divoom_fish_tank_frame_cache_free_entry(divoom_fish_tank_frame_cache_entry_t *ent)
{
    if (!ent)
    {
        return;
    }

    if (ent->dsc.data)
    {
        divoom_system_free((void *)ent->dsc.data);
        ent->dsc.data = NULL;
        ent->dsc.data_size = 0;
    }

    ent->used = 0;
    ent->key[0] = '\0';
}

static divoom_fish_tank_frame_cache_entry_t *divoom_fish_tank_frame_cache_alloc(const char *key)
{
    int i;
    divoom_fish_tank_frame_cache_entry_t *hit;

    hit = divoom_fish_tank_frame_cache_find(key);
    if (hit)
    {
        return hit;
    }

    for (i = 0; i < DIVOOM_FISH_TANK_FRAME_CACHE_MAX; i++)
    {
        if (!s_fish_frame_cache[i].used)
        {
            strncpy(s_fish_frame_cache[i].key, key, sizeof(s_fish_frame_cache[i].key) - 1);
            s_fish_frame_cache[i].key[sizeof(s_fish_frame_cache[i].key) - 1] = '\0';
            s_fish_frame_cache[i].used = 1;
            return &s_fish_frame_cache[i];
        }
    }

    divoom_fish_tank_frame_cache_free_entry(&s_fish_frame_cache[0]);
    strncpy(s_fish_frame_cache[0].key, key, sizeof(s_fish_frame_cache[0].key) - 1);
    s_fish_frame_cache[0].key[sizeof(s_fish_frame_cache[0].key) - 1] = '\0';
    s_fish_frame_cache[0].used = 1;
    return &s_fish_frame_cache[0];
}

static void divoom_fish_tank_mirror_frame_x(lv_img_dsc_t *dsc)
{
    uint8_t *data;
    uint32_t stride;
    uint32_t x;
    uint32_t y;
    uint32_t w;
    uint32_t h;

    if (!dsc || !dsc->data)
    {
        return;
    }

    data = (uint8_t *)dsc->data;
    w = (uint32_t)dsc->header.w;
    h = (uint32_t)dsc->header.h;

    if (dsc->header.cf == LV_COLOR_FORMAT_RGB565A8)
    {
        uint8_t *alpha;

        stride = divoom_pixel_format_stride(w, LV_COLOR_FORMAT_RGB565A8);
        alpha = data + stride * h;
        for (y = 0; y < h; y++)
        {
            uint8_t *rgb_line = data + y * stride;
            uint8_t *alpha_line = alpha + y * w;
            for (x = 0; x < w / 2; x++)
            {
                uint8_t *left = rgb_line + x * 2u;
                uint8_t *right = rgb_line + (w - 1u - x) * 2u;
                uint8_t tmp0 = left[0];
                uint8_t tmp1 = left[1];
                uint8_t tmpa = alpha_line[x];

                left[0] = right[0];
                left[1] = right[1];
                right[0] = tmp0;
                right[1] = tmp1;

                alpha_line[x] = alpha_line[w - 1u - x];
                alpha_line[w - 1u - x] = tmpa;
            }
        }
    }
}

static int divoom_fish_tank_decode_frame_file(const char *rel_path, lv_img_dsc_t *out_dsc, uint8 flip_x)
{
    char full_path[160];
    lv_img_decoder_dsc_t decoder_dsc;
    lv_img_header_t header;
    lv_img_dsc_t src_dsc;
    uint32_t size;

    if (!rel_path || !out_dsc)
    {
        return -1;
    }

    snprintf(full_path, sizeof(full_path), "%s%s", DIVOOM_FISH_TANK_IMAGE_BASE_PATH, rel_path);
    if (lv_img_decoder_open(&decoder_dsc, full_path, lv_color_hex(0), 0) != LV_RES_OK)
    {
        return -1;
    }

    if (lv_img_decoder_get_info(full_path, &header) != LV_RES_OK || !decoder_dsc.img_data)
    {
        lv_img_decoder_close(&decoder_dsc);
        return -1;
    }

    size = divoom_pixel_format_size((uint32_t)header.w, (uint32_t)header.h, LV_COLOR_FORMAT_RGB565A8);
    out_dsc->data = divoom_system_malloc(size);
    if (!out_dsc->data)
    {
        lv_img_decoder_close(&decoder_dsc);
        return -1;
    }

    divoom_lv_img_dsc_init_fmt(&src_dsc, header.w, header.h,
        (lv_color_format_t)decoder_dsc.header.cf, decoder_dsc.img_data);
    if (!divoom_lv_img_dsc_convert_buf(&src_dsc, (uint8_t *)out_dsc->data, LV_COLOR_FORMAT_RGB565A8))
    {
        divoom_system_free((void *)out_dsc->data);
        out_dsc->data = NULL;
        lv_img_decoder_close(&decoder_dsc);
        return -1;
    }
    divoom_lv_img_dsc_init_fmt(out_dsc, header.w, header.h, LV_COLOR_FORMAT_RGB565A8, out_dsc->data);
    if (flip_x)
    {
        divoom_fish_tank_mirror_frame_x(out_dsc);
    }
    lv_img_decoder_close(&decoder_dsc);
    return 0;
}

static divoom_fish_tank_frame_cache_entry_t *divoom_fish_tank_get_frame_cache(const char *src,
    uint8 flip_x, uint8 *loaded)
{
    char key[180];
    divoom_fish_tank_frame_cache_entry_t *ent;

    if (loaded)
    {
        *loaded = 0;
    }

    if (!src || src[0] == '\0')
    {
        return NULL;
    }

    snprintf(key, sizeof(key), "%s|%c", src, flip_x ? 'L' : 'R');
    ent = divoom_fish_tank_frame_cache_find(key);
    if (ent && ent->dsc.data)
    {
        return ent;
    }

    ent = divoom_fish_tank_frame_cache_alloc(key);
    if (!ent)
    {
        return NULL;
    }

    if (divoom_fish_tank_decode_frame_file(src, &ent->dsc, flip_x) != 0)
    {
        divoom_fish_tank_frame_cache_free_entry(ent);
        return NULL;
    }

    if (loaded)
    {
        *loaded = 1;
    }
    return ent;
}

static void divoom_fish_tank_draw_fallback(divoom_fish_tank_t *ph)
{
    int tail_swing;
    int32_t sin_q14;
    int16_t phase_ang;

    if (!ph || !ph->fish_fallback)
    {
        return;
    }

    phase_ang = (int16_t)(((uint64_t)ph->bob_phase_ms * 12017u / 1024u) &
        (DIVOOM_FISH_TANK_ANGLE_FULL - 1));
    divoom_fish_tank_sincos_q14(phase_ang, &sin_q14, NULL);
    tail_swing = (int)((sin_q14 * 4) >> DIVOOM_FISH_TANK_TRIG_Q14);
    if (ph->fish_fallback_body)
    {
        lv_obj_set_pos(ph->fish_fallback_body, 10, 12);
    }
    if (ph->fish_fallback_tail)
    {
        lv_obj_set_pos(ph->fish_fallback_tail, 2, 14 + tail_swing);
        lv_obj_set_style_transform_angle(ph->fish_fallback_tail,
            450 + tail_swing * 18, 0);
    }
    if (ph->fish_fallback_fin)
    {
        lv_obj_set_pos(ph->fish_fallback_fin, 19, 9);
    }
    if (ph->fish_fallback_eye)
    {
        lv_obj_set_pos(ph->fish_fallback_eye, 31, 16);
    }
    lv_obj_invalidate(ph->fish_fallback);
}

static void divoom_fish_tank_set_fallback_visible(divoom_fish_tank_t *ph, uint8 visible)
{
    if (!ph || !ph->fish_fallback)
    {
        return;
    }

    if (visible)
    {
        lv_obj_clear_flag(ph->fish_fallback, LV_OBJ_FLAG_HIDDEN);
        if (ph->fish_img)
        {
            lv_obj_clear_flag(ph->fish_img, LV_OBJ_FLAG_HIDDEN);
            lv_obj_move_foreground(ph->fish_img);
        }
        lv_obj_move_foreground(ph->fish_fallback);
    }
    else
    {
        lv_obj_add_flag(ph->fish_fallback, LV_OBJ_FLAG_HIDDEN);
        if (ph->fish_img)
        {
            lv_obj_clear_flag(ph->fish_img, LV_OBJ_FLAG_HIDDEN);
            lv_obj_move_foreground(ph->fish_img);
        }
    }
}

static void divoom_fish_tank_apply_fish_src(divoom_fish_tank_t *ph, const char *src, uint8 flip_x)
{
    if (divoom_fish_tank_apply_img_src(ph, ph ? ph->fish_img : NULL, src, flip_x) != 0)
    {
        divoom_fish_tank_set_fallback_visible(ph, 1);
        return;
    }

    divoom_fish_tank_set_fallback_visible(ph, 0);
}

static int divoom_fish_tank_apply_img_src(divoom_fish_tank_t *ph, lv_obj_t *img,
    const char *src, uint8 flip_x)
{
    char full_path[160];
    char log_msg[256];
    divoom_fish_tank_frame_cache_entry_t *frame;
    uint8 loaded;

    if (!ph || !img || !src || src[0] == '\0')
    {
        return -1;
    }

    snprintf(full_path, sizeof(full_path), "%s%s", DIVOOM_FISH_TANK_IMAGE_BASE_PATH, src);
    loaded = 0;
    frame = divoom_fish_tank_get_frame_cache(src, flip_x, &loaded);
    if (!frame || !frame->dsc.data)
    {
        snprintf(log_msg, sizeof(log_msg), "[fish_tank] image cache failed src=%s full=%s flip=%d",
            src, full_path, (int)flip_x);
        divoom_fish_tank_debug_log(log_msg);
        divoom_print("%s\n", log_msg);
        return -1;
    }

    if (loaded)
    {
        snprintf(log_msg, sizeof(log_msg), "[fish_tank] image cache ok src=%s full=%s flip=%d size=%dx%d cf=%d",
            src, full_path, (int)flip_x, (int)frame->dsc.header.w, (int)frame->dsc.header.h,
            (int)frame->dsc.header.cf);
        divoom_fish_tank_debug_log(log_msg);
        divoom_print("%s\n", log_msg);
    }
    lv_img_set_src(img, &frame->dsc);
    lv_obj_clear_flag(img, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_img_opa(img, LV_OPA_COVER, 0);
    lv_obj_set_style_opa(img, LV_OPA_COVER, 0);
    lv_obj_move_foreground(img);
    return 0;
}

static void divoom_fish_tank_apply_fish_visual(divoom_fish_tank_t *ph)
{
    int32_t bob_y_q8;
    int32_t sin_q14;
    int32_t angle_target_deg10;
    int32_t wobble_limit_deg10;
    int32_t sway_deg10;
    int32_t turn_deg10;
    int16_t phase_ang;
    int16_t pitch_ang;
    int16_t ang;
    lv_coord_t ix;
    lv_coord_t iy;
    const char *src;

    if (!ph || !ph->fish_img)
    {
        return;
    }

    bob_y_q8 = 0;
    if (ph->dwell_remain_ms > 0)
    {
        phase_ang = (int16_t)(((uint64_t)ph->bob_phase_ms * 2670u / 1024u) &
            (DIVOOM_FISH_TANK_ANGLE_FULL - 1));
        divoom_fish_tank_sincos_q14(phase_ang, &sin_q14, NULL);
        bob_y_q8 = (int32_t)(((int64_t)sin_q14 * 640) >> DIVOOM_FISH_TANK_TRIG_Q14);
    }

    ix = divoom_fish_tank_q8_to_coord(ph->pos_x_q8);
    iy = divoom_fish_tank_q8_to_coord(ph->pos_y_q8 + bob_y_q8);
    lv_obj_set_pos(ph->fish_img, ix, iy);

    /* 右向动作组 + 帧号决定 PNG；向左由内存镜像生成，避免鱼肚翻到上面。 */
    if (ph->sprite_lr_applied != ph->facing_lr ||
        ph->sprite_action_applied != ph->anim_action ||
        ph->sprite_frame_applied != ph->anim_frame)
    {
        src = divoom_fish_tank_anim_frame_path(ph, ph->anim_action, ph->anim_frame);
        if (src && src[0] != '\0')
        {
            divoom_fish_tank_apply_fish_src(ph, src, ph->facing_lr ? 0 : 1);
        }
        ph->sprite_lr_applied = ph->facing_lr;
        ph->sprite_action_applied = ph->anim_action;
        ph->sprite_frame_applied = ph->anim_frame;
    }

    if (ph->fish_fallback)
    {
        divoom_fish_tank_draw_fallback(ph);
        lv_obj_set_pos(ph->fish_fallback, ix, iy);
    }

    /* 侧面鱼不能整圈翻滚：方向用左右镜像，角度只表现有限俯仰/摆动。 */
    wobble_limit_deg10 = ph->max_pitch_deg10;
    pitch_ang = divoom_fish_tank_atan2_angle(ph->vel_y_q8,
        divoom_fish_tank_iabs32(ph->vel_x_q8) + divoom_fish_tank_q8_from_i32(4));
    angle_target_deg10 = ((int32_t)pitch_ang * 3600) / DIVOOM_FISH_TANK_ANGLE_FULL;
    if (angle_target_deg10 > wobble_limit_deg10)
    {
        angle_target_deg10 = wobble_limit_deg10;
    }
    if (angle_target_deg10 < -wobble_limit_deg10)
    {
        angle_target_deg10 = -wobble_limit_deg10;
    }

    if (ph->dwell_remain_ms > 0)
    {
        phase_ang = (int16_t)(((uint64_t)ph->bob_phase_ms * 4005u / 1024u) &
            (DIVOOM_FISH_TANK_ANGLE_FULL - 1));
        divoom_fish_tank_sincos_q14(phase_ang, &sin_q14, NULL);
        angle_target_deg10 += (int32_t)(((int64_t)sin_q14 * (wobble_limit_deg10 * 35 / 100)) >>
            DIVOOM_FISH_TANK_TRIG_Q14);
    }
    if (ph->facing_lr == 0)
    {
        angle_target_deg10 = -angle_target_deg10;
    }

    phase_ang = (int16_t)(((uint64_t)ph->bob_phase_ms * 6675u / 1024u) &
        (DIVOOM_FISH_TANK_ANGLE_FULL - 1));
    divoom_fish_tank_sincos_q14(phase_ang, &sin_q14, NULL);
    sway_deg10 = (int32_t)(((int64_t)sin_q14 * 18) >> DIVOOM_FISH_TANK_TRIG_Q14);
    if (ph->speed_q8 < divoom_fish_tank_q8_from_i32(2))
    {
        sway_deg10 = (sway_deg10 * 4) / 10;
    }
    if (ph->facing_lr == 0)
    {
        sway_deg10 = -sway_deg10;
    }

    turn_deg10 = ph->turn_visual_deg10;
    if (ph->facing_lr == 0)
    {
        turn_deg10 = -turn_deg10;
    }

    angle_target_deg10 += sway_deg10 + turn_deg10;
    if (angle_target_deg10 > wobble_limit_deg10)
    {
        angle_target_deg10 = wobble_limit_deg10;
    }
    if (angle_target_deg10 < -wobble_limit_deg10)
    {
        angle_target_deg10 = -wobble_limit_deg10;
    }

    ph->disp_pitch_deg10 += (int16_t)(((int32_t)angle_target_deg10 - ph->disp_pitch_deg10) * 18 / 100);
    ang = ph->disp_pitch_deg10;
    lv_img_set_angle(ph->fish_img, 0);
    lv_obj_set_style_transform_angle(ph->fish_img, ang, 0);
    if (ph->fish_fallback)
    {
        lv_obj_set_style_transform_angle(ph->fish_fallback, ang, 0);
    }
}

static int divoom_fish_tank_ensure_decor(divoom_fish_tank_t *ph, lv_obj_t *root)
{
    divoom_fish_tank_decor_t *decor;
    const divoom_fish_tank_decor_cfg_t *cfg;
    int i;

    if (!ph || !root)
    {
        return -1;
    }

    for (i = 0; i < DIVOOM_FISH_TANK_DECOR_MAX; i++)
    {
        decor = &ph->decor[i];
        cfg = &s_fish_decor_cfg[i];
        if (decor->obj)
        {
            continue;
        }

        decor->obj = lv_img_create(root);
        if (!decor->obj)
        {
            return -1;
        }

        decor->anim_frame = cfg->frame_count > 0 ? (uint8)(cfg->start_frame % cfg->frame_count) : 0;
        decor->elapsed_ms = 0;
        decor->frame_ms_current = cfg->frame_ms > 0 ? cfg->frame_ms : 180u;
        decor->jitter_remain_ms = (uint16_t)divoom_fish_tank_rand_range_u32(ph, 900u, 2400u);
        decor->sway_phase_ang = cfg->sway_phase_ang & (DIVOOM_FISH_TANK_ANGLE_FULL - 1);
        decor->sway_period_current_ms = cfg->sway_period_ms;
        decor->sway_amp_current_deg10 = cfg->sway_amp_deg10;
        decor->sway_jitter_remain_ms = (uint16_t)divoom_fish_tank_rand_range_u32(ph, 1200u, 3600u);
        snprintf(decor->full_path, sizeof(decor->full_path), "%s%s",
            DIVOOM_FISH_TANK_IMAGE_BASE_PATH, cfg->frames[decor->anim_frame]);
        lv_obj_clear_flag(decor->obj, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(decor->obj, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_size(decor->obj, cfg->w, cfg->h);
        lv_img_set_size_mode(decor->obj, LV_IMG_SIZE_MODE_REAL);
        lv_img_set_antialias(decor->obj, true);
        lv_img_set_pivot(decor->obj, cfg->w / 2, cfg->sway_period_ms > 0 ? cfg->h : cfg->h / 2);
        lv_obj_set_style_transform_pivot_x(decor->obj, cfg->w / 2, 0);
        lv_obj_set_style_transform_pivot_y(decor->obj, cfg->sway_period_ms > 0 ? cfg->h : cfg->h / 2, 0);
        lv_img_set_angle(decor->obj, 0);
        lv_obj_set_style_transform_angle(decor->obj, 0, 0);
        lv_obj_set_style_img_opa(decor->obj, LV_OPA_COVER, 0);
        lv_obj_set_style_opa(decor->obj, LV_OPA_COVER, 0);
        lv_img_set_src(decor->obj, decor->full_path);
        decor->ready = 1;
    }

    for (i = DIVOOM_FISH_TANK_DECOR_MAX - 1; i >= 0; i--)
    {
        if (ph->decor[i].obj)
        {
            lv_obj_move_background(ph->decor[i].obj);
        }
    }

    divoom_fish_tank_layout_decor(ph);
    divoom_fish_tank_update_decor(ph, DIVOOM_FISH_TANK_DECOR_UPDATE_MS);
    return 0;
}

static void divoom_fish_tank_layout_decor(divoom_fish_tank_t *ph)
{
    divoom_fish_tank_decor_t *decor;
    const divoom_fish_tank_decor_cfg_t *cfg;
    lv_coord_t x;
    lv_coord_t y;
    int i;

    if (!ph)
    {
        return;
    }

    for (i = 0; i < DIVOOM_FISH_TANK_DECOR_MAX; i++)
    {
        decor = &ph->decor[i];
        cfg = &s_fish_decor_cfg[i];
        if (!decor->obj || !decor->ready)
        {
            continue;
        }

        if (cfg->anchor_right)
        {
            x = ph->area_w - cfg->w - cfg->x_offset;
        }
        else
        {
            x = cfg->x_offset;
        }
        y = ph->area_h - cfg->h - cfg->bottom_offset;
        if (x < 0)
        {
            x = 0;
        }
        if (y < 0)
        {
            y = 0;
        }
        lv_obj_set_pos(decor->obj, x, y);
    }
}

static void divoom_fish_tank_update_decor(divoom_fish_tank_t *ph, uint32_t dt_ms)
{
    divoom_fish_tank_decor_t *decor;
    const divoom_fish_tank_decor_cfg_t *cfg;
    uint16_t frame_ms;
    uint16_t elapsed;
    uint32_t step_ms;
    uint8 frame_count;
    uint8 next_frame;
    int i;

    if (!ph || !ph->active)
    {
        return;
    }

    ph->decor_elapsed_ms += dt_ms;
    if (ph->decor_elapsed_ms < DIVOOM_FISH_TANK_DECOR_UPDATE_MS)
    {
        return;
    }
    step_ms = ph->decor_elapsed_ms;
    ph->decor_elapsed_ms = 0;

    for (i = 0; i < DIVOOM_FISH_TANK_DECOR_MAX; i++)
    {
        decor = &ph->decor[i];
        cfg = &s_fish_decor_cfg[i];
        if (!decor->obj || !decor->ready)
        {
            continue;
        }

        if (cfg->sway_period_ms > 0 && cfg->sway_amp_deg10 > 0)
        {
            uint32_t period;
            uint32_t phase_step;
            uint32_t pct;
            int32_t sin_q14;
            int32_t sin2_q14;
            int32_t angle_deg10;
            int16_t phase2_ang;

            if (decor->sway_period_current_ms == 0)
            {
                decor->sway_period_current_ms = cfg->sway_period_ms;
            }
            if (decor->sway_amp_current_deg10 == 0)
            {
                decor->sway_amp_current_deg10 = cfg->sway_amp_deg10;
            }
            if (decor->sway_jitter_remain_ms <= step_ms)
            {
                pct = divoom_fish_tank_rand_range_u32(ph, 88u, 123u);
                decor->sway_period_current_ms =
                    (uint16_t)((cfg->sway_period_ms * pct + 50u) / 100u);
                pct = divoom_fish_tank_rand_range_u32(ph, 88u, 116u);
                decor->sway_amp_current_deg10 =
                    (uint16_t)((cfg->sway_amp_deg10 * pct + 50u) / 100u);
                decor->sway_jitter_remain_ms =
                    (uint16_t)divoom_fish_tank_rand_range_u32(ph, 1200u, 3600u);
            }
            else
            {
                decor->sway_jitter_remain_ms =
                    (uint16_t)(decor->sway_jitter_remain_ms - step_ms);
            }

            period = decor->sway_period_current_ms > 0 ?
                decor->sway_period_current_ms : cfg->sway_period_ms;
            phase_step = (step_ms * DIVOOM_FISH_TANK_ANGLE_FULL + period / 2u) / period;
            decor->sway_phase_ang = (uint16_t)((decor->sway_phase_ang + phase_step) &
                (DIVOOM_FISH_TANK_ANGLE_FULL - 1));
            divoom_fish_tank_sincos_q14((int16_t)decor->sway_phase_ang, &sin_q14, NULL);
            phase2_ang = (int16_t)((decor->sway_phase_ang * 2u + cfg->sway_phase_ang / 3u) &
                (DIVOOM_FISH_TANK_ANGLE_FULL - 1));
            divoom_fish_tank_sincos_q14(phase2_ang, &sin2_q14, NULL);
            angle_deg10 = (int32_t)(((int64_t)sin_q14 * decor->sway_amp_current_deg10) >>
                DIVOOM_FISH_TANK_TRIG_Q14);
            angle_deg10 += (int32_t)(((int64_t)sin2_q14 *
                (decor->sway_amp_current_deg10 / 5)) >> DIVOOM_FISH_TANK_TRIG_Q14);
            lv_obj_set_style_transform_angle(decor->obj, angle_deg10, 0);
        }

        frame_count = cfg->frame_count;
        if (frame_count <= 1 || frame_count > DIVOOM_FISH_TANK_DECOR_FRAME_MAX)
        {
            continue;
        }

        frame_ms = cfg->frame_ms > 0 ? cfg->frame_ms : 180u;
        if (decor->frame_ms_current == 0)
        {
            decor->frame_ms_current = frame_ms;
        }
        if (decor->jitter_remain_ms <= step_ms)
        {
            uint32_t pct;

            pct = divoom_fish_tank_rand_range_u32(ph, 88u, 122u);
            if (divoom_fish_tank_rand_range_u32(ph, 0u, 99u) < 14u)
            {
                pct = divoom_fish_tank_rand_range_u32(ph, 128u, 168u);
            }
            decor->frame_ms_current = (uint16_t)((frame_ms * pct + 50u) / 100u);
            decor->jitter_remain_ms = (uint16_t)divoom_fish_tank_rand_range_u32(ph, 900u, 2400u);
        }
        else
        {
            decor->jitter_remain_ms = (uint16_t)(decor->jitter_remain_ms - step_ms);
        }
        frame_ms = decor->frame_ms_current;
        elapsed = (uint16_t)(decor->elapsed_ms + step_ms);
        if (elapsed < frame_ms)
        {
            decor->elapsed_ms = elapsed;
            continue;
        }

        next_frame = (uint8)((decor->anim_frame + (elapsed / frame_ms)) % frame_count);
        decor->elapsed_ms = (uint16_t)(elapsed % frame_ms);
        if (next_frame == decor->anim_frame)
        {
            continue;
        }

        decor->anim_frame = next_frame;
        if (!cfg->frames[decor->anim_frame] || cfg->frames[decor->anim_frame][0] == '\0')
        {
            decor->anim_frame = 0;
        }
        snprintf(decor->full_path, sizeof(decor->full_path), "%s%s",
            DIVOOM_FISH_TANK_IMAGE_BASE_PATH, cfg->frames[decor->anim_frame]);
        lv_img_set_src(decor->obj, decor->full_path);
    }
}

static void divoom_fish_tank_bubble_hide(divoom_fish_tank_bubble_t *bubble)
{
    if (!bubble)
    {
        return;
    }

    bubble->active = 0;
    if (bubble->obj)
    {
        lv_obj_add_flag(bubble->obj, LV_OBJ_FLAG_HIDDEN);
    }
}

static int divoom_fish_tank_ensure_bubbles(divoom_fish_tank_t *ph, lv_obj_t *root)
{
    int i;

    if (!ph || !root)
    {
        return -1;
    }

    for (i = 0; i < DIVOOM_FISH_TANK_BUBBLE_MAX; i++)
    {
        if (ph->bubbles[i].obj)
        {
            continue;
        }

        ph->bubbles[i].obj = lv_obj_create(root);
        if (!ph->bubbles[i].obj)
        {
            return -1;
        }

        lv_obj_remove_style_all(ph->bubbles[i].obj);
        lv_obj_clear_flag(ph->bubbles[i].obj, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(ph->bubbles[i].obj, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_radius(ph->bubbles[i].obj, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(ph->bubbles[i].obj, lv_color_hex(0xD7FAFF), 0);
        lv_obj_set_style_bg_opa(ph->bubbles[i].obj, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(ph->bubbles[i].obj, 1, 0);
        lv_obj_set_style_border_color(ph->bubbles[i].obj, lv_color_hex(0xF3FFFF), 0);
        lv_obj_set_style_border_opa(ph->bubbles[i].obj, LV_OPA_TRANSP, 0);
        lv_obj_set_style_pad_all(ph->bubbles[i].obj, 0, 0);
        lv_obj_move_background(ph->bubbles[i].obj);
    }

    return 0;
}

static divoom_fish_tank_bubble_t *divoom_fish_tank_bubble_alloc(divoom_fish_tank_t *ph)
{
    int i;

    if (!ph)
    {
        return NULL;
    }

    for (i = 0; i < DIVOOM_FISH_TANK_BUBBLE_MAX; i++)
    {
        if (!ph->bubbles[i].active && ph->bubbles[i].obj)
        {
            return &ph->bubbles[i];
        }
    }

    return NULL;
}

static void divoom_fish_tank_bubble_apply_visual(divoom_fish_tank_bubble_t *bubble)
{
    int32_t sin_q14;
    int32_t draw_x_q8;
    uint32_t remain;
    uint32_t opa;
    uint32_t fade_ms;

    if (!bubble || !bubble->obj || !bubble->active || bubble->life_ms == 0)
    {
        return;
    }

    remain = bubble->age_ms < bubble->life_ms ? (uint32_t)(bubble->life_ms - bubble->age_ms) : 0u;
    fade_ms = 700u;
    if (bubble->life_ms < 1800u)
    {
        fade_ms = bubble->life_ms / 3u;
    }

    opa = bubble->opacity;
    if (bubble->age_ms < fade_ms && fade_ms > 0)
    {
        opa = ((uint32_t)bubble->opacity * bubble->age_ms) / fade_ms;
    }
    if (remain < fade_ms && fade_ms > 0)
    {
        opa = ((uint32_t)bubble->opacity * remain) / fade_ms;
    }
    if (opa > LV_OPA_COVER)
    {
        opa = LV_OPA_COVER;
    }

    divoom_fish_tank_sincos_q14((int16_t)bubble->phase_ang, &sin_q14, NULL);
    draw_x_q8 = bubble->x_q8 +
        (int32_t)(((int64_t)sin_q14 * bubble->drift_q8) >> DIVOOM_FISH_TANK_TRIG_Q14);

    lv_obj_set_pos(bubble->obj,
        divoom_fish_tank_q8_to_coord(draw_x_q8) - bubble->radius,
        divoom_fish_tank_q8_to_coord(bubble->y_q8) - bubble->radius);
    lv_obj_set_style_bg_opa(bubble->obj, (lv_opa_t)(opa * 55u / 100u), 0);
    lv_obj_set_style_border_opa(bubble->obj, (lv_opa_t)opa, 0);
    lv_obj_clear_flag(bubble->obj, LV_OBJ_FLAG_HIDDEN);
}

static void divoom_fish_tank_spawn_bubble(divoom_fish_tank_t *ph,
    int32_t x_q8, int32_t y_q8, uint8 from_fish)
{
    divoom_fish_tank_bubble_t *bubble;
    int32_t min_x;
    int32_t max_x;
    int32_t min_y;
    int32_t max_y;
    uint8 radius;
    lv_obj_t *obj;
    int32_t travel_q8;
    int32_t avg_speed_q8;
    uint32_t life_ms;

    if (!ph || ph->area_w <= 0 || ph->area_h <= 0)
    {
        return;
    }

    bubble = divoom_fish_tank_bubble_alloc(ph);
    if (!bubble)
    {
        return;
    }

    radius = (uint8)divoom_fish_tank_rand_range_i32(ph, 2, from_fish ? 4 : 6);
    min_x = divoom_fish_tank_q8_from_i32(ph->params.margin + radius);
    max_x = divoom_fish_tank_q8_from_i32(ph->area_w - ph->params.margin - radius);
    min_y = divoom_fish_tank_q8_from_i32(ph->params.margin + radius);
    max_y = divoom_fish_tank_q8_from_i32(ph->area_h - ph->params.margin - radius);
    if (max_x <= min_x || max_y <= min_y)
    {
        return;
    }

    if (x_q8 < min_x)
    {
        x_q8 = min_x;
    }
    if (x_q8 > max_x)
    {
        x_q8 = max_x;
    }
    if (y_q8 < min_y)
    {
        y_q8 = min_y;
    }
    if (y_q8 > max_y)
    {
        y_q8 = max_y;
    }

    obj = bubble->obj;
    memset(bubble, 0, sizeof(*bubble));
    bubble->obj = obj;
    bubble->active = 1;
    bubble->x_q8 = x_q8;
    bubble->y_q8 = y_q8;
    bubble->radius = radius;
    bubble->drift_q8 = divoom_fish_tank_q8_from_i32(
        divoom_fish_tank_rand_range_i32(ph, from_fish ? 1 : radius, from_fish ? 5 : radius + 5));
    bubble->speed_q8 = divoom_fish_tank_q8_from_i32(
        divoom_fish_tank_rand_range_i32(ph, from_fish ? 16 : 10, from_fish ? 26 : 18));
    bubble->terminal_q8 = divoom_fish_tank_q8_from_i32(
        divoom_fish_tank_rand_range_i32(ph, 28 + radius * 4, 36 + radius * 6 + (from_fish ? 6 : 0)));
    bubble->accel_q8_s2 = divoom_fish_tank_q8_from_i32(
        divoom_fish_tank_rand_range_i32(ph, 14 + radius * 3, 24 + radius * 5));
    travel_q8 = y_q8 - min_y;
    avg_speed_q8 = (bubble->speed_q8 + bubble->terminal_q8) / 2;
    if (avg_speed_q8 < divoom_fish_tank_q8_from_i32(8))
    {
        avg_speed_q8 = divoom_fish_tank_q8_from_i32(8);
    }
    life_ms = (uint32_t)(((uint64_t)travel_q8 * 1000u) / (uint32_t)avg_speed_q8);
    life_ms += (uint32_t)divoom_fish_tank_rand_range_i32(ph, from_fish ? 700 : 1000, from_fish ? 1300 : 1800);
    if (life_ms < (from_fish ? 1800u : 3200u))
    {
        life_ms = from_fish ? 1800u : 3200u;
    }
    if (life_ms > 12000u)
    {
        life_ms = 12000u;
    }
    bubble->life_ms = (uint16_t)life_ms;
    bubble->phase_ang = (uint16_t)divoom_fish_tank_rand_range_i32(ph, 0,
        DIVOOM_FISH_TANK_ANGLE_FULL - 1);
    bubble->phase_step = (uint16_t)divoom_fish_tank_rand_range_i32(ph,
        from_fish ? 520 : 260, from_fish ? 980 : 760);
    bubble->opacity = (uint8)divoom_fish_tank_rand_range_i32(ph,
        from_fish ? 120 : 95, from_fish ? 205 : 185);

    lv_obj_set_size(bubble->obj, radius * 2, radius * 2);
    lv_obj_set_style_radius(bubble->obj, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(bubble->obj, from_fish ? lv_color_hex(0xE8FDFF) : lv_color_hex(0xBFEFFF), 0);
    lv_obj_set_style_border_color(bubble->obj, lv_color_hex(0xF6FFFF), 0);
    divoom_fish_tank_bubble_apply_visual(bubble);
}

static void divoom_fish_tank_spawn_bottom_bubble(divoom_fish_tank_t *ph)
{
    int32_t min_x;
    int32_t max_x;
    int32_t x_q8;
    int32_t y_q8;

    if (!ph)
    {
        return;
    }

    min_x = divoom_fish_tank_q8_from_i32(ph->params.margin + 8);
    max_x = divoom_fish_tank_q8_from_i32(ph->area_w - ph->params.margin - 8);
    if (max_x <= min_x)
    {
        return;
    }

    x_q8 = divoom_fish_tank_rand_range_q8(ph, min_x, max_x);
    y_q8 = divoom_fish_tank_q8_from_i32(ph->area_h - ph->params.margin - 5);
    divoom_fish_tank_spawn_bubble(ph, x_q8, y_q8, 0);
}

static void divoom_fish_tank_spawn_fish_bubble(divoom_fish_tank_t *ph)
{
    int32_t x_q8;
    int32_t y_q8;
    int32_t head_offset_q8;

    if (!ph)
    {
        return;
    }

    head_offset_q8 = divoom_fish_tank_q8_from_i32(ph->params.fish_w * 38 / 100);
    x_q8 = ph->pos_x_q8 + divoom_fish_tank_q8_from_i32(ph->params.fish_w / 2);
    if (ph->facing_lr)
    {
        x_q8 += head_offset_q8;
    }
    else
    {
        x_q8 -= head_offset_q8;
    }

    y_q8 = ph->pos_y_q8 + divoom_fish_tank_q8_from_i32(ph->params.fish_h * 45 / 100);
    y_q8 -= divoom_fish_tank_q8_from_i32(4);
    divoom_fish_tank_spawn_bubble(ph, x_q8, y_q8, 1);
}

static void divoom_fish_tank_update_bubbles(divoom_fish_tank_t *ph, uint32_t dt_ms)
{
    divoom_fish_tank_bubble_t *bubble;
    uint32_t threshold;
    int32_t min_y;
    int i;

    if (!ph || !ph->inner_ready || !ph->active)
    {
        return;
    }

    min_y = divoom_fish_tank_q8_from_i32(ph->params.margin - 8);
    for (i = 0; i < DIVOOM_FISH_TANK_BUBBLE_MAX; i++)
    {
        bubble = &ph->bubbles[i];
        if (!bubble->active)
        {
            continue;
        }

        if ((uint32_t)bubble->age_ms + dt_ms >= bubble->life_ms)
        {
            divoom_fish_tank_bubble_hide(bubble);
            continue;
        }

        bubble->age_ms = (uint16_t)(bubble->age_ms + dt_ms);
        bubble->speed_q8 += (int32_t)(((int64_t)bubble->accel_q8_s2 * dt_ms) / 1000);
        if (bubble->speed_q8 > bubble->terminal_q8)
        {
            bubble->speed_q8 = bubble->terminal_q8;
        }
        bubble->y_q8 -= (int32_t)(((int64_t)bubble->speed_q8 * dt_ms) / 1000);
        bubble->phase_ang = (uint16_t)(((uint32_t)bubble->phase_ang +
            ((uint32_t)bubble->phase_step * dt_ms) / 1000u) &
            (DIVOOM_FISH_TANK_ANGLE_FULL - 1));

        if (bubble->y_q8 < min_y)
        {
            divoom_fish_tank_bubble_hide(bubble);
            continue;
        }

        divoom_fish_tank_bubble_apply_visual(bubble);
    }

    if (ph->bubble_spawn_ms <= dt_ms)
    {
        divoom_fish_tank_spawn_bottom_bubble(ph);
        ph->bubble_spawn_ms = divoom_fish_tank_rand_range_u32(ph,
            DIVOOM_FISH_TANK_BUBBLE_BOTTOM_MIN_MS, DIVOOM_FISH_TANK_BUBBLE_BOTTOM_MAX_MS);
    }
    else
    {
        ph->bubble_spawn_ms -= dt_ms;
    }

    if (ph->speed_q8 > divoom_fish_tank_q8_from_i32(8))
    {
        threshold = dt_ms * DIVOOM_FISH_TANK_BUBBLE_FISH_PROB_MS;
        if (divoom_fish_tank_iabs32(ph->turn_visual_deg10) > 25)
        {
            threshold += dt_ms * DIVOOM_FISH_TANK_BUBBLE_TURN_PROB_MS;
        }
        if (threshold > 0 && (divoom_fish_tank_rand_u32(ph) % 10000u) < threshold)
        {
            divoom_fish_tank_spawn_fish_bubble(ph);
        }
    }
}

static void divoom_fish_tank_reset_bubbles(divoom_fish_tank_t *ph)
{
    int i;

    if (!ph)
    {
        return;
    }

    for (i = 0; i < DIVOOM_FISH_TANK_BUBBLE_MAX; i++)
    {
        divoom_fish_tank_bubble_hide(&ph->bubbles[i]);
    }

    ph->bubble_spawn_ms = divoom_fish_tank_rand_range_u32(ph,
        DIVOOM_FISH_TANK_BUBBLE_BOTTOM_MIN_MS, DIVOOM_FISH_TANK_BUBBLE_BOTTOM_MAX_MS);
}

static void divoom_fish_tank_update_cruise_mod(divoom_fish_tank_t *ph, uint32_t dt_ms)
{
    if (!ph)
    {
        return;
    }

    if (ph->cruise_remain_ms <= dt_ms)
    {
        ph->cruise_pct = (uint8)divoom_fish_tank_rand_range_i32(ph, 92, 104);
        ph->cruise_remain_ms = divoom_fish_tank_rand_range_u32(ph, 2200u, 5200u);
    }
    else
    {
        ph->cruise_remain_ms -= dt_ms;
    }

    if (ph->cruise2_remain_ms <= dt_ms)
    {
        ph->cruise2_pct = (uint8)divoom_fish_tank_rand_range_i32(ph, 82, 96);
        ph->cruise2_remain_ms = divoom_fish_tank_rand_range_u32(ph, 2600u, 5800u);
    }
    else
    {
        ph->cruise2_remain_ms -= dt_ms;
    }
}

static void divoom_fish_tank_second_sync_vel(divoom_fish_tank_t *ph)
{
    int32_t sin_q14;
    int32_t cos_q14;

    if (!ph)
    {
        return;
    }

    divoom_fish_tank_sincos_q14(ph->heading2_ang, &sin_q14, &cos_q14);
    ph->vel2_x_q8 = (int32_t)(((int64_t)cos_q14 * ph->speed2_q8) >> DIVOOM_FISH_TANK_TRIG_Q14);
    ph->vel2_y_q8 = (int32_t)(((int64_t)sin_q14 * ph->speed2_q8) >> DIVOOM_FISH_TANK_TRIG_Q14);
}

static void divoom_fish_tank_second_clamp_pos(divoom_fish_tank_t *ph)
{
    int32_t min_x;
    int32_t min_y;
    int32_t max_x;
    int32_t max_y;

    if (!ph)
    {
        return;
    }

    min_x = divoom_fish_tank_q8_from_i32(ph->params.margin);
    min_y = divoom_fish_tank_q8_from_i32(ph->params.margin);
    max_x = divoom_fish_tank_q8_from_i32(ph->area_w - ph->params.fish_w - ph->params.margin);
    max_y = divoom_fish_tank_q8_from_i32(ph->area_h - ph->params.fish_h - ph->params.margin);

    if (ph->pos2_x_q8 < min_x)
    {
        ph->pos2_x_q8 = min_x;
    }
    if (ph->pos2_x_q8 > max_x)
    {
        ph->pos2_x_q8 = max_x;
    }
    if (ph->pos2_y_q8 < min_y)
    {
        ph->pos2_y_q8 = min_y;
    }
    if (ph->pos2_y_q8 > max_y)
    {
        ph->pos2_y_q8 = max_y;
    }
}

static void divoom_fish_tank_second_update_facing(divoom_fish_tank_t *ph)
{
    int32_t cos_q14;

    if (!ph || ph->speed2_q8 < divoom_fish_tank_q8_from_i32(3))
    {
        return;
    }

    divoom_fish_tank_sincos_q14(ph->heading2_ang, NULL, &cos_q14);
    if (cos_q14 > 1966)
    {
        ph->facing2_lr = 1;
    }
    else if (cos_q14 < -1966)
    {
        ph->facing2_lr = 0;
    }
}

static uint32_t divoom_fish_tank_second_frame_ms(divoom_fish_tank_t *ph)
{
    int32_t span;
    int32_t speed_over_min;
    uint32_t ms;

    if (!ph)
    {
        return DIVOOM_FISH_TANK_FRAME_MS_MAX;
    }

    if (ph->dwell2_remain_ms > 0)
    {
        return DIVOOM_FISH_TANK_FRAME_MS_DWELL;
    }
    if (ph->dart2_remain_ms > 0 || ph->anim2_action == DIVOOM_FISH_TANK_ANIM_QUICK)
    {
        return DIVOOM_FISH_TANK_FRAME_MS_MIN;
    }

    span = ph->v_max_q8 - ph->v_min_q8;
    speed_over_min = ph->speed2_q8 - ph->v_min_q8;
    if (span <= DIVOOM_FISH_TANK_Q8_ONE)
    {
        return (DIVOOM_FISH_TANK_FRAME_MS_MIN + DIVOOM_FISH_TANK_FRAME_MS_MAX) / 2u;
    }
    if (speed_over_min < 0)
    {
        speed_over_min = 0;
    }
    if (speed_over_min > span)
    {
        speed_over_min = span;
    }

    ms = DIVOOM_FISH_TANK_FRAME_MS_MAX -
        (uint32_t)(((uint64_t)(DIVOOM_FISH_TANK_FRAME_MS_MAX - DIVOOM_FISH_TANK_FRAME_MS_MIN) *
        (uint32_t)speed_over_min) / (uint32_t)span);
    if (ms < DIVOOM_FISH_TANK_FRAME_MS_MIN)
    {
        ms = DIVOOM_FISH_TANK_FRAME_MS_MIN;
    }
    return ms;
}

static void divoom_fish_tank_second_set_anim_action(divoom_fish_tank_t *ph, uint8 action)
{
    int frame_count;

    if (!ph || !divoom_fish_tank_anim_action_available(action))
    {
        action = DIVOOM_FISH_TANK_ANIM_NORMAL;
    }

    frame_count = divoom_fish_tank_anim_frame_count(action);
    if (frame_count <= 0)
    {
        action = DIVOOM_FISH_TANK_ANIM_NORMAL;
        frame_count = divoom_fish_tank_anim_frame_count(action);
    }

    if (ph->anim2_action != action)
    {
        ph->anim2_action = action;
        ph->anim2_frame = frame_count > 0 ? (uint8)(ph->anim2_frame % frame_count) : 0;
        ph->anim2_elapsed_ms = 0;
        ph->sprite2_action_applied = 0xFF;
        ph->sprite2_frame_applied = 0xFF;
    }

    ph->action2_remain_ms = divoom_fish_tank_rand_range_u32(ph,
        DIVOOM_FISH_TANK_ACTION_MIN_MS, DIVOOM_FISH_TANK_ACTION_MAX_MS);
}

static void divoom_fish_tank_second_update_anim(divoom_fish_tank_t *ph, uint32_t dt_ms)
{
    int frame_count;
    uint8 next_action;
    uint32_t frame_ms;

    if (!ph)
    {
        return;
    }

    if (ph->dart2_remain_ms > 0 && divoom_fish_tank_anim_action_available(DIVOOM_FISH_TANK_ANIM_QUICK))
    {
        divoom_fish_tank_second_set_anim_action(ph, DIVOOM_FISH_TANK_ANIM_QUICK);
    }
    else if (ph->dwell2_remain_ms > 0 && divoom_fish_tank_anim_action_available(DIVOOM_FISH_TANK_ANIM_SOFT))
    {
        divoom_fish_tank_second_set_anim_action(ph, DIVOOM_FISH_TANK_ANIM_SOFT);
    }
    else if (ph->action2_remain_ms <= dt_ms)
    {
        next_action = divoom_fish_tank_pick_random_anim_action(ph);
        divoom_fish_tank_second_set_anim_action(ph, next_action);
    }
    else
    {
        ph->action2_remain_ms -= dt_ms;
    }

    frame_count = divoom_fish_tank_anim_frame_count(ph->anim2_action);
    if (frame_count <= 0)
    {
        ph->anim2_action = DIVOOM_FISH_TANK_ANIM_NORMAL;
        frame_count = divoom_fish_tank_anim_frame_count(ph->anim2_action);
    }
    if (frame_count <= 1)
    {
        ph->anim2_frame = 0;
        return;
    }

    frame_ms = divoom_fish_tank_second_frame_ms(ph);
    ph->anim2_elapsed_ms += dt_ms;
    while (ph->anim2_elapsed_ms >= frame_ms)
    {
        ph->anim2_elapsed_ms -= frame_ms;
        ph->anim2_frame = (uint8)((ph->anim2_frame + 1) % frame_count);
        ph->sprite2_frame_applied = 0xFF;
    }
}

static void divoom_fish_tank_second_pick_target(divoom_fish_tank_t *ph)
{
    int32_t min_x;
    int32_t min_y;
    int32_t max_x;
    int32_t max_y;
    int32_t inner_w;
    int32_t inner_h;
    int32_t tx;
    int32_t ty;
    int32_t raw_tx;
    int32_t raw_ty;
    int32_t dx;
    int32_t dy;
    int32_t dist;
    int32_t min_dist;
    int32_t sin_q14;
    int32_t cos_q14;
    int16_t cur_h;
    int16_t bias_h;
    int16_t pick_h;
    int tries;

    if (!ph)
    {
        return;
    }

    min_x = divoom_fish_tank_q8_from_i32(ph->params.margin);
    min_y = divoom_fish_tank_q8_from_i32(ph->params.margin);
    max_x = divoom_fish_tank_q8_from_i32(ph->area_w - ph->params.fish_w - ph->params.margin);
    max_y = divoom_fish_tank_q8_from_i32(ph->area_h - ph->params.fish_h - ph->params.margin);
    if (max_x <= min_x || max_y <= min_y)
    {
        ph->has2_target = 0;
        return;
    }

    inner_w = max_x - min_x;
    inner_h = max_y - min_y;
    min_dist = (inner_w < inner_h ? inner_w : inner_h) * 26 / 100;
    if (min_dist < divoom_fish_tank_q8_from_i32(55))
    {
        min_dist = divoom_fish_tank_q8_from_i32(55);
    }

    cur_h = ph->heading2_ang;
    if (ph->speed2_q8 < divoom_fish_tank_q8_from_i32(2) ||
        (divoom_fish_tank_rand_u32(ph) % 10000u) < 3800u)
    {
        cur_h = (int16_t)divoom_fish_tank_rand_range_i32(ph,
            -DIVOOM_FISH_TANK_ANGLE_HALF, DIVOOM_FISH_TANK_ANGLE_HALF - 1);
    }

    tx = ph->pos2_x_q8;
    ty = ph->pos2_y_q8;
    for (tries = 0; tries < 16; tries++)
    {
        bias_h = (int16_t)divoom_fish_tank_rand_range_i32(ph, -1150, 1150);
        pick_h = divoom_fish_tank_normalize_angle((int32_t)cur_h + bias_h);
        dist = divoom_fish_tank_rand_range_q8(ph, min_dist, min_dist + divoom_fish_tank_q8_from_i32(105));
        divoom_fish_tank_sincos_q14(pick_h, &sin_q14, &cos_q14);
        raw_tx = ph->pos2_x_q8 + (int32_t)(((int64_t)cos_q14 * dist) >> DIVOOM_FISH_TANK_TRIG_Q14);
        raw_ty = ph->pos2_y_q8 + (int32_t)(((int64_t)sin_q14 * dist) >> DIVOOM_FISH_TANK_TRIG_Q14);
        if (raw_tx < min_x || raw_tx > max_x || raw_ty < min_y || raw_ty > max_y)
        {
            continue;
        }

        dx = raw_tx - ph->pos2_x_q8;
        dy = raw_ty - ph->pos2_y_q8;
        if (divoom_fish_tank_dist_sq_q8(dx, dy) >= (uint64_t)min_dist * (uint64_t)min_dist)
        {
            tx = raw_tx;
            ty = raw_ty;
            break;
        }
    }

    if (tries >= 16)
    {
        tx = divoom_fish_tank_rand_range_q8(ph, min_x + inner_w / 10, max_x - inner_w / 10);
        ty = divoom_fish_tank_rand_range_q8(ph, min_y + inner_h / 10, max_y - inner_h / 10);
    }

    ph->target2_x_q8 = tx;
    ph->target2_y_q8 = ty;
    ph->has2_target = 1;
}

static void divoom_fish_tank_second_apply_visual(divoom_fish_tank_t *ph)
{
    int32_t bob_y_q8;
    int32_t sin_q14;
    int32_t angle_target_deg10;
    int32_t wobble_limit_deg10;
    int32_t sway_deg10;
    int32_t turn_deg10;
    int16_t phase_ang;
    int16_t pitch_ang;
    int16_t ang;
    lv_coord_t ix;
    lv_coord_t iy;
    const char *src;

    if (!ph || !ph->inner_ready || !ph->active || !ph->fish_img2)
    {
        return;
    }

    bob_y_q8 = 0;
    if (ph->dwell2_remain_ms > 0)
    {
        phase_ang = (int16_t)(((uint64_t)(ph->bob_phase_ms + 440u) * 2670u / 1024u) &
            (DIVOOM_FISH_TANK_ANGLE_FULL - 1));
        divoom_fish_tank_sincos_q14(phase_ang, &sin_q14, NULL);
        bob_y_q8 = (int32_t)(((int64_t)sin_q14 * 560) >> DIVOOM_FISH_TANK_TRIG_Q14);
    }

    ix = divoom_fish_tank_q8_to_coord(ph->pos2_x_q8);
    iy = divoom_fish_tank_q8_to_coord(ph->pos2_y_q8 + bob_y_q8);
    lv_obj_set_pos(ph->fish_img2, ix, iy);

    if (ph->sprite2_lr_applied != ph->facing2_lr ||
        ph->sprite2_action_applied != ph->anim2_action ||
        ph->sprite2_frame_applied != ph->anim2_frame)
    {
        src = divoom_fish_tank_anim_frame_path(ph, ph->anim2_action, ph->anim2_frame);
        if (src && src[0] != '\0')
        {
            divoom_fish_tank_apply_img_src(ph, ph->fish_img2, src, ph->facing2_lr ? 0 : 1);
        }
        ph->sprite2_lr_applied = ph->facing2_lr;
        ph->sprite2_action_applied = ph->anim2_action;
        ph->sprite2_frame_applied = ph->anim2_frame;
    }

    wobble_limit_deg10 = ph->max_pitch_deg10;
    pitch_ang = divoom_fish_tank_atan2_angle(ph->vel2_y_q8,
        divoom_fish_tank_iabs32(ph->vel2_x_q8) + divoom_fish_tank_q8_from_i32(4));
    angle_target_deg10 = ((int32_t)pitch_ang * 3600) / DIVOOM_FISH_TANK_ANGLE_FULL;
    if (angle_target_deg10 > wobble_limit_deg10)
    {
        angle_target_deg10 = wobble_limit_deg10;
    }
    if (angle_target_deg10 < -wobble_limit_deg10)
    {
        angle_target_deg10 = -wobble_limit_deg10;
    }
    if (ph->facing2_lr == 0)
    {
        angle_target_deg10 = -angle_target_deg10;
    }

    phase_ang = (int16_t)(((uint64_t)(ph->bob_phase_ms + 690u) * 6675u / 1024u) &
        (DIVOOM_FISH_TANK_ANGLE_FULL - 1));
    divoom_fish_tank_sincos_q14(phase_ang, &sin_q14, NULL);
    sway_deg10 = (int32_t)(((int64_t)sin_q14 * 14) >> DIVOOM_FISH_TANK_TRIG_Q14);
    if (ph->facing2_lr == 0)
    {
        sway_deg10 = -sway_deg10;
    }
    turn_deg10 = ph->facing2_lr ? ph->turn2_visual_deg10 : -ph->turn2_visual_deg10;
    angle_target_deg10 += sway_deg10 + turn_deg10;
    if (angle_target_deg10 > wobble_limit_deg10)
    {
        angle_target_deg10 = wobble_limit_deg10;
    }
    if (angle_target_deg10 < -wobble_limit_deg10)
    {
        angle_target_deg10 = -wobble_limit_deg10;
    }

    ph->disp2_pitch_deg10 += (int16_t)(((int32_t)angle_target_deg10 - ph->disp2_pitch_deg10) * 18 / 100);
    ang = ph->disp2_pitch_deg10;
    lv_img_set_angle(ph->fish_img2, 0);
    lv_obj_set_style_transform_angle(ph->fish_img2, ang, 0);
    lv_obj_move_foreground(ph->fish_img2);
}

static void divoom_fish_tank_second_step(divoom_fish_tank_t *ph, uint32_t dt_ms)
{
    int32_t dx;
    int32_t dy;
    int32_t dist;
    int32_t desired_speed;
    int32_t cur_speed;
    int32_t speed_delta;
    int32_t max_delta;
    int32_t sin_q14;
    int32_t cos_q14;
    int32_t turn_visual_target;
    int16_t des_h;
    int16_t diff;
    int16_t max_turn;
    int16_t abs_diff;
    uint32_t dart_threshold;

    if (!ph || !ph->fish_img2)
    {
        return;
    }

    if (ph->dwell2_remain_ms > 0)
    {
        ph->dwell2_remain_ms = ph->dwell2_remain_ms > dt_ms ? ph->dwell2_remain_ms - dt_ms : 0;
        if (ph->dwell2_remain_ms == 0)
        {
            divoom_fish_tank_second_pick_target(ph);
        }
        ph->speed2_q8 = ph->speed2_q8 * 84 / 100;
        if (ph->speed2_q8 < (DIVOOM_FISH_TANK_Q8_ONE / 2))
        {
            ph->speed2_q8 = 0;
        }
        divoom_fish_tank_second_sync_vel(ph);
        ph->turn2_visual_deg10 = (int16_t)(ph->turn2_visual_deg10 * 85 / 100);
        divoom_fish_tank_second_update_anim(ph, dt_ms);
        divoom_fish_tank_second_apply_visual(ph);
        return;
    }

    if (!ph->has2_target)
    {
        divoom_fish_tank_second_pick_target(ph);
    }

    dx = ph->target2_x_q8 - ph->pos2_x_q8;
    dy = ph->target2_y_q8 - ph->pos2_y_q8;
    dist = divoom_fish_tank_dist_q8(dx, dy);
    if (dist < divoom_fish_tank_q8_from_i32(5))
    {
        if ((divoom_fish_tank_rand_u32(ph) % DIVOOM_FISH_TANK_PROB_ONE) <= (ph->dwell_chance_q15 / 2))
        {
            ph->dwell2_remain_ms = divoom_fish_tank_rand_range_u32(ph,
                ph->params.dwell_min_ms, ph->params.dwell_max_ms);
            ph->speed2_q8 = 0;
            ph->has2_target = 0;
            divoom_fish_tank_second_sync_vel(ph);
            divoom_fish_tank_second_update_anim(ph, dt_ms);
            divoom_fish_tank_second_apply_visual(ph);
            return;
        }
        divoom_fish_tank_second_pick_target(ph);
        dx = ph->target2_x_q8 - ph->pos2_x_q8;
        dy = ph->target2_y_q8 - ph->pos2_y_q8;
        dist = divoom_fish_tank_dist_q8(dx, dy);
    }

    des_h = divoom_fish_tank_atan2_angle(dy, dx);
    diff = divoom_fish_tank_normalize_angle((int32_t)des_h - ph->heading2_ang);
    abs_diff = (int16_t)divoom_fish_tank_iabs32(diff);
    max_turn = (int16_t)(((int32_t)ph->turn_rate_ang_s * (int32_t)dt_ms) / 1000);
    if (ph->cruise2_pct == 0)
    {
        ph->cruise2_pct = 90;
    }
    desired_speed = ph->v_max_q8 * ph->cruise2_pct / 100;
    if (dist < ph->arrive_radius_q8)
    {
        desired_speed = ph->v_min_q8 + (int32_t)(((int64_t)(desired_speed - ph->v_min_q8) * dist) /
            ph->arrive_radius_q8);
    }
    if (abs_diff > DIVOOM_FISH_TANK_ANGLE_ABS_BACK)
    {
        desired_speed = 0;
    }
    else if (abs_diff > DIVOOM_FISH_TANK_ANGLE_ABS_SLOW_1)
    {
        desired_speed /= 2;
    }
    else if (abs_diff > DIVOOM_FISH_TANK_ANGLE_ABS_SLOW_2)
    {
        desired_speed = desired_speed * 3 / 4;
    }

    if (ph->dart2_remain_ms > 0)
    {
        if (abs_diff < DIVOOM_FISH_TANK_ANGLE_DART_OK)
        {
            desired_speed = ph->v_max_q8 * 9 / 10;
        }
        ph->dart2_remain_ms = ph->dart2_remain_ms > dt_ms ? ph->dart2_remain_ms - dt_ms : 0;
    }
    else if (abs_diff < DIVOOM_FISH_TANK_ANGLE_DART_START)
    {
        dart_threshold = (uint32_t)(((uint64_t)ph->dart_chance_q15 * dt_ms) / 1400u);
        if ((divoom_fish_tank_rand_u32(ph) % DIVOOM_FISH_TANK_PROB_ONE) <= dart_threshold)
        {
            ph->dart2_remain_ms = ph->params.dart_ms;
            desired_speed = ph->v_max_q8 * 9 / 10;
        }
    }

    if (diff > max_turn)
    {
        ph->heading2_ang = divoom_fish_tank_normalize_angle((int32_t)ph->heading2_ang + max_turn);
    }
    else if (diff < -max_turn)
    {
        ph->heading2_ang = divoom_fish_tank_normalize_angle((int32_t)ph->heading2_ang - max_turn);
    }
    else
    {
        ph->heading2_ang = des_h;
    }

    cur_speed = ph->speed2_q8;
    max_delta = (int32_t)(((int64_t)ph->accel_q8_s2 * dt_ms) / 1100);
    if (desired_speed > cur_speed + max_delta)
    {
        desired_speed = cur_speed + max_delta;
    }
    else if (desired_speed < cur_speed - max_delta)
    {
        desired_speed = cur_speed - max_delta;
    }
    if (desired_speed < 0)
    {
        desired_speed = 0;
    }
    if (desired_speed > 0 && desired_speed < ph->v_min_q8 && abs_diff < DIVOOM_FISH_TANK_ANGLE_MIN_SPEED)
    {
        desired_speed = ph->v_min_q8;
    }

    ph->speed2_q8 = desired_speed;
    divoom_fish_tank_sincos_q14(ph->heading2_ang, &sin_q14, &cos_q14);
    speed_delta = (int32_t)(((int64_t)ph->speed2_q8 * dt_ms) / 1000);
    ph->pos2_x_q8 += (int32_t)(((int64_t)cos_q14 * speed_delta) >> DIVOOM_FISH_TANK_TRIG_Q14);
    ph->pos2_y_q8 += (int32_t)(((int64_t)sin_q14 * speed_delta) >> DIVOOM_FISH_TANK_TRIG_Q14);
    divoom_fish_tank_second_clamp_pos(ph);
    divoom_fish_tank_second_sync_vel(ph);
    divoom_fish_tank_second_update_facing(ph);
    turn_visual_target = ((int32_t)diff * 250) / DIVOOM_FISH_TANK_ANGLE_FULL;
    if (turn_visual_target > 55)
    {
        turn_visual_target = 55;
    }
    if (turn_visual_target < -55)
    {
        turn_visual_target = -55;
    }
    ph->turn2_visual_deg10 += (int16_t)((turn_visual_target - ph->turn2_visual_deg10) * 18 / 100);
    divoom_fish_tank_second_update_anim(ph, dt_ms);
    divoom_fish_tank_second_apply_visual(ph);
}

static void divoom_fish_tank_second_init(divoom_fish_tank_t *ph)
{
    if (!ph)
    {
        return;
    }

    ph->facing2_lr = 0;
    ph->sprite2_lr_applied = 0xFF;
    ph->sprite2_action_applied = 0xFF;
    ph->sprite2_frame_applied = 0xFF;
    ph->anim2_action = DIVOOM_FISH_TANK_ANIM_WIDE;
    if (!divoom_fish_tank_anim_action_available(ph->anim2_action))
    {
        ph->anim2_action = DIVOOM_FISH_TANK_ANIM_NORMAL;
    }
    ph->anim2_frame = 3;
    ph->anim2_elapsed_ms = 0;
    ph->action2_remain_ms = divoom_fish_tank_rand_range_u32(ph,
        DIVOOM_FISH_TANK_ACTION_MIN_MS, DIVOOM_FISH_TANK_ACTION_MAX_MS);
    ph->dwell2_remain_ms = 0;
    ph->dart2_remain_ms = 0;
    ph->pos2_x_q8 = divoom_fish_tank_q8_from_i32((ph->area_w - ph->params.fish_w) * 68 / 100);
    ph->pos2_y_q8 = divoom_fish_tank_q8_from_i32((ph->area_h - ph->params.fish_h) * 35 / 100);
    ph->heading2_ang = DIVOOM_FISH_TANK_ANGLE_HALF;
    ph->speed2_q8 = ph->v_min_q8 * 9 / 10;
    ph->disp2_pitch_deg10 = 0;
    ph->turn2_visual_deg10 = 0;
    ph->has2_target = 0;
    divoom_fish_tank_second_clamp_pos(ph);
    divoom_fish_tank_second_sync_vel(ph);
    divoom_fish_tank_second_pick_target(ph);
    divoom_fish_tank_second_apply_visual(ph);
}

static void divoom_fish_tank_pick_target(divoom_fish_tank_t *ph)
{
    int32_t min_x;
    int32_t min_y;
    int32_t max_x;
    int32_t max_y;
    int32_t inner_w;
    int32_t inner_h;
    int16_t cur_h;
    int16_t bias_h;
    int16_t pick_h;
    int32_t dist;
    int32_t tx;
    int32_t ty;
    int32_t raw_tx;
    int32_t raw_ty;
    int32_t min_dist;
    int32_t dx;
    int32_t dy;
    int32_t sin_q14;
    int32_t cos_q14;
    int tries;

    if (!ph)
    {
        return;
    }

    min_x = divoom_fish_tank_q8_from_i32(ph->params.margin);
    min_y = divoom_fish_tank_q8_from_i32(ph->params.margin);
    max_x = divoom_fish_tank_q8_from_i32(ph->area_w - ph->params.fish_w - ph->params.margin);
    max_y = divoom_fish_tank_q8_from_i32(ph->area_h - ph->params.fish_h - ph->params.margin);

    if (max_x <= min_x || max_y <= min_y)
    {
        ph->has_target = 0;
        return;
    }

    inner_w = max_x - min_x;
    inner_h = max_y - min_y;
    min_dist = (inner_w < inner_h ? inner_w : inner_h) * 32 / 100;
    if (min_dist < divoom_fish_tank_q8_from_i32(70))
    {
        min_dist = divoom_fish_tank_q8_from_i32(70);
    }

    if (ph->pos_x_q8 < min_x + ph->wall_influence_q8 ||
        ph->pos_x_q8 > max_x - ph->wall_influence_q8 ||
        ph->pos_y_q8 < min_y + ph->wall_influence_q8 ||
        ph->pos_y_q8 > max_y - ph->wall_influence_q8)
    {
        tx = divoom_fish_tank_rand_range_q8(ph, min_x + inner_w * 30 / 100, min_x + inner_w * 70 / 100);
        ty = divoom_fish_tank_rand_range_q8(ph, min_y + inner_h * 30 / 100, min_y + inner_h * 70 / 100);
        ph->target_x_q8 = tx;
        ph->target_y_q8 = ty;
        ph->has_target = 1;
        return;
    }

    cur_h = ph->heading_ang;
    if (ph->speed_q8 < divoom_fish_tank_q8_from_i32(2))
    {
        cur_h = (int16_t)divoom_fish_tank_rand_range_i32(ph,
            -DIVOOM_FISH_TANK_ANGLE_HALF, DIVOOM_FISH_TANK_ANGLE_HALF - 1);
    }

    if ((divoom_fish_tank_rand_u32(ph) % 10000u) < 3500u)
    {
        cur_h = (int16_t)divoom_fish_tank_rand_range_i32(ph,
            -DIVOOM_FISH_TANK_ANGLE_HALF, DIVOOM_FISH_TANK_ANGLE_HALF - 1);
    }

    tx = ph->pos_x_q8;
    ty = ph->pos_y_q8;
    for (tries = 0; tries < 16; tries++)
    {
        bias_h = (int16_t)divoom_fish_tank_rand_range_i32(ph, -1081, 1081);
        pick_h = divoom_fish_tank_normalize_angle((int32_t)cur_h + bias_h);
        dist = divoom_fish_tank_rand_range_q8(ph, min_dist, min_dist + divoom_fish_tank_q8_from_i32(120));
        divoom_fish_tank_sincos_q14(pick_h, &sin_q14, &cos_q14);
        raw_tx = ph->pos_x_q8 + (int32_t)(((int64_t)cos_q14 * dist) >> DIVOOM_FISH_TANK_TRIG_Q14);
        raw_ty = ph->pos_y_q8 + (int32_t)(((int64_t)sin_q14 * dist) >> DIVOOM_FISH_TANK_TRIG_Q14);
        if (raw_tx < min_x || raw_tx > max_x || raw_ty < min_y || raw_ty > max_y)
        {
            continue;
        }

        dx = raw_tx - ph->pos_x_q8;
        dy = raw_ty - ph->pos_y_q8;
        if (divoom_fish_tank_dist_sq_q8(dx, dy) >= (uint64_t)min_dist * (uint64_t)min_dist)
        {
            tx = raw_tx;
            ty = raw_ty;
            break;
        }
    }

    if (tries >= 16)
    {
        for (tries = 0; tries < 12; tries++)
        {
            tx = divoom_fish_tank_rand_range_q8(ph, min_x, max_x);
            ty = divoom_fish_tank_rand_range_q8(ph, min_y, max_y);
            dx = tx - ph->pos_x_q8;
            dy = ty - ph->pos_y_q8;
            if (divoom_fish_tank_dist_sq_q8(dx, dy) >= (uint64_t)min_dist * (uint64_t)min_dist)
            {
                break;
            }
        }
    }

    ph->target_x_q8 = tx;
    ph->target_y_q8 = ty;
    ph->has_target = 1;
}

static void divoom_fish_tank_steering_step(divoom_fish_tank_t *ph, uint32_t dt_ms)
{
    int32_t dx;
    int32_t dy;
    int32_t dist;
    int32_t desired_speed;
    int32_t cur_speed;
    int32_t speed_delta;
    int32_t max_delta;
    int16_t des_h;
    int16_t diff;
    int16_t max_turn;
    int16_t abs_diff;
    int32_t min_x;
    int32_t min_y;
    int32_t max_x;
    int32_t max_y;
    int32_t wall_dx;
    int32_t wall_dy;
    int32_t sin_q14;
    int32_t cos_q14;
    int32_t turn_visual_target;
    uint32_t dart_threshold;

    if (!ph || !ph->inner_ready || !ph->active)
    {
        return;
    }

    if (dt_ms == 0)
    {
        dt_ms = DIVOOM_FISH_TANK_TICK_MS;
    }

    ph->bob_phase_ms += dt_ms;
    divoom_fish_tank_update_cruise_mod(ph, dt_ms);

    if (ph->dwell_remain_ms > 0)
    {
        if (ph->dwell_remain_ms > dt_ms)
        {
            ph->dwell_remain_ms -= dt_ms;
        }
        else
        {
            ph->dwell_remain_ms = 0;
            divoom_fish_tank_pick_target(ph);
        }

        ph->speed_q8 = ph->speed_q8 * 82 / 100;
        if (ph->speed_q8 < (DIVOOM_FISH_TANK_Q8_ONE / 2))
        {
            ph->speed_q8 = 0;
        }

        divoom_fish_tank_sync_vel(ph);
        ph->turn_visual_deg10 = (int16_t)(ph->turn_visual_deg10 * 85 / 100);
        divoom_fish_tank_update_bubbles(ph, dt_ms);
        divoom_fish_tank_update_anim(ph, dt_ms);
        divoom_fish_tank_apply_fish_visual(ph);
        return;
    }

    if (!ph->has_target)
    {
        divoom_fish_tank_pick_target(ph);
    }

    dx = ph->target_x_q8 - ph->pos_x_q8;
    dy = ph->target_y_q8 - ph->pos_y_q8;
    dist = divoom_fish_tank_dist_q8(dx, dy);

    if (dist < divoom_fish_tank_q8_from_i32(5))
    {
        if ((divoom_fish_tank_rand_u32(ph) % DIVOOM_FISH_TANK_PROB_ONE) <= ph->dwell_chance_q15)
        {
            ph->dwell_remain_ms = divoom_fish_tank_rand_range_u32(ph,
                ph->params.dwell_min_ms, ph->params.dwell_max_ms);
            ph->speed_q8 = 0;
            ph->has_target = 0;
            divoom_fish_tank_sync_vel(ph);
            divoom_fish_tank_update_bubbles(ph, dt_ms);
            divoom_fish_tank_update_anim(ph, dt_ms);
            divoom_fish_tank_apply_fish_visual(ph);
            return;
        }

        divoom_fish_tank_pick_target(ph);
        dx = ph->target_x_q8 - ph->pos_x_q8;
        dy = ph->target_y_q8 - ph->pos_y_q8;
        dist = divoom_fish_tank_dist_q8(dx, dy);
    }

    des_h = divoom_fish_tank_atan2_angle(dy, dx);
    diff = divoom_fish_tank_normalize_angle((int32_t)des_h - ph->heading_ang);
    abs_diff = (int16_t)divoom_fish_tank_iabs32(diff);
    max_turn = (int16_t)(((int32_t)ph->turn_rate_ang_s * (int32_t)dt_ms) / 1000);

    min_x = divoom_fish_tank_q8_from_i32(ph->params.margin);
    min_y = divoom_fish_tank_q8_from_i32(ph->params.margin);
    max_x = divoom_fish_tank_q8_from_i32(ph->area_w - ph->params.fish_w - ph->params.margin);
    max_y = divoom_fish_tank_q8_from_i32(ph->area_h - ph->params.fish_h - ph->params.margin);

    wall_dx = 0;
    wall_dy = 0;
    if (ph->pos_x_q8 < min_x + ph->wall_influence_q8)
    {
        wall_dx += 1;
    }
    else if (ph->pos_x_q8 > max_x - ph->wall_influence_q8)
    {
        wall_dx -= 1;
    }

    if (ph->pos_y_q8 < min_y + ph->wall_influence_q8)
    {
        wall_dy += 1;
    }
    else if (ph->pos_y_q8 > max_y - ph->wall_influence_q8)
    {
        wall_dy -= 1;
    }

    if (wall_dx != 0 || wall_dy != 0)
    {
        ph->has_target = 0;
        des_h = divoom_fish_tank_atan2_angle(wall_dy, wall_dx);
        diff = divoom_fish_tank_normalize_angle((int32_t)des_h - ph->heading_ang);
        abs_diff = (int16_t)divoom_fish_tank_iabs32(diff);
    }

    desired_speed = ph->v_max_q8;
    if (ph->cruise_pct == 0)
    {
        ph->cruise_pct = 98;
    }
    desired_speed = desired_speed * ph->cruise_pct / 100;

    if (dist < ph->arrive_radius_q8)
    {
        desired_speed = ph->v_min_q8 + (int32_t)(((int64_t)(ph->v_max_q8 - ph->v_min_q8) * dist) /
            ph->arrive_radius_q8);
    }

    if (abs_diff > DIVOOM_FISH_TANK_ANGLE_ABS_BACK)
    {
        desired_speed = 0;
    }
    else if (abs_diff > DIVOOM_FISH_TANK_ANGLE_ABS_SLOW_1)
    {
        desired_speed /= 2;
    }
    else if (abs_diff > DIVOOM_FISH_TANK_ANGLE_ABS_SLOW_2)
    {
        desired_speed = desired_speed * 3 / 4;
    }

    if (ph->dart_remain_ms > 0)
    {
        if (abs_diff < DIVOOM_FISH_TANK_ANGLE_DART_OK)
        {
            desired_speed = ph->v_max_q8;
        }

        if (ph->dart_remain_ms > dt_ms)
        {
            ph->dart_remain_ms -= dt_ms;
        }
        else
        {
            ph->dart_remain_ms = 0;
        }
    }
    else if (abs_diff < DIVOOM_FISH_TANK_ANGLE_DART_START)
    {
        dart_threshold = (uint32_t)(((uint64_t)ph->dart_chance_q15 * dt_ms) / 1000u);
        if ((divoom_fish_tank_rand_u32(ph) % DIVOOM_FISH_TANK_PROB_ONE) <= dart_threshold)
        {
            ph->dart_remain_ms = ph->params.dart_ms;
            desired_speed = ph->v_max_q8;
        }
    }

    if (diff > max_turn)
    {
        ph->heading_ang = divoom_fish_tank_normalize_angle((int32_t)ph->heading_ang + max_turn);
    }
    else if (diff < -max_turn)
    {
        ph->heading_ang = divoom_fish_tank_normalize_angle((int32_t)ph->heading_ang - max_turn);
    }
    else
    {
        ph->heading_ang = des_h;
    }

    cur_speed = ph->speed_q8;
    max_delta = (int32_t)(((int64_t)ph->accel_q8_s2 * dt_ms) / 1000);
    if (desired_speed > cur_speed + max_delta)
    {
        desired_speed = cur_speed + max_delta;
    }
    else if (desired_speed < cur_speed - max_delta)
    {
        desired_speed = cur_speed - max_delta;
    }

    if (desired_speed < 0)
    {
        desired_speed = 0;
    }

    if (desired_speed > 0 && desired_speed < ph->v_min_q8 && abs_diff < DIVOOM_FISH_TANK_ANGLE_MIN_SPEED)
    {
        desired_speed = ph->v_min_q8;
    }

    ph->speed_q8 = desired_speed;
    divoom_fish_tank_sincos_q14(ph->heading_ang, &sin_q14, &cos_q14);
    speed_delta = (int32_t)(((int64_t)ph->speed_q8 * dt_ms) / 1000);
    ph->pos_x_q8 += (int32_t)(((int64_t)cos_q14 * speed_delta) >> DIVOOM_FISH_TANK_TRIG_Q14);
    ph->pos_y_q8 += (int32_t)(((int64_t)sin_q14 * speed_delta) >> DIVOOM_FISH_TANK_TRIG_Q14);
    divoom_fish_tank_clamp_pos(ph);
    divoom_fish_tank_sync_vel(ph);
    divoom_fish_tank_update_facing(ph);
    divoom_fish_tank_update_bubbles(ph, dt_ms);
    turn_visual_target = ((int32_t)diff * 288) / DIVOOM_FISH_TANK_ANGLE_FULL;
    if (turn_visual_target > 70)
    {
        turn_visual_target = 70;
    }
    if (turn_visual_target < -70)
    {
        turn_visual_target = -70;
    }
    ph->turn_visual_deg10 += (int16_t)((turn_visual_target - ph->turn_visual_deg10) * 18 / 100);
    divoom_fish_tank_update_anim(ph, dt_ms);
    divoom_fish_tank_apply_fish_visual(ph);
}

static void divoom_fish_tank_steering_tick(lv_timer_t *timer)
{
    divoom_fish_tank_t *ph;

    ph = (divoom_fish_tank_t *)lv_timer_get_user_data(timer);
    if (!ph)
    {
        return;
    }

    divoom_fish_tank_steering_step(ph, DIVOOM_FISH_TANK_TICK_MS);
    divoom_fish_tank_second_step(ph, DIVOOM_FISH_TANK_TICK_MS);
    divoom_fish_tank_update_decor(ph, DIVOOM_FISH_TANK_TICK_MS);
}

static void divoom_fish_tank_init_root_style(divoom_fish_tank_t *ph)
{
    if (!ph || ph->styles_inited)
    {
        return;
    }

    lv_style_init(&ph->root_style);
    lv_style_set_bg_opa(&ph->root_style, LV_OPA_TRANSP);
    lv_style_set_border_width(&ph->root_style, 0);
    lv_style_set_pad_all(&ph->root_style, 0);
    lv_style_set_radius(&ph->root_style, 0);
    ph->styles_inited = 1;
}

static void divoom_fish_tank_update_area_from_obj(divoom_fish_tank_t *ph, lv_obj_t *obj)
{
    if (!ph || !obj)
    {
        return;
    }

    ph->area_w = lv_obj_get_content_width(obj);
    ph->area_h = lv_obj_get_content_height(obj);
}

static int divoom_fish_tank_ensure_inner(divoom_fish_tank_t *ph)
{
    lv_obj_t *root;

    if (!ph)
    {
        return -1;
    }

    root = (lv_obj_t *)ph;
    divoom_fish_tank_update_area_from_obj(ph, root);

    if (ph->area_w < ph->params.fish_w + ph->params.margin * 2 ||
        ph->area_h < ph->params.fish_h + ph->params.margin * 2)
    {
        return -1;
    }

    divoom_fish_tank_init_root_style(ph);
    if (!ph->root_style_added)
    {
        lv_obj_add_style(root, &ph->root_style, 0);
        ph->root_style_added = 1;
    }

    lv_obj_clear_flag(root, LV_OBJ_FLAG_CLICKABLE);

    if (divoom_fish_tank_ensure_bubbles(ph, root) != 0)
    {
        return -1;
    }
    if (divoom_fish_tank_ensure_decor(ph, root) != 0)
    {
        return -1;
    }

    if (ph->fish_img)
    {
        divoom_fish_tank_layout_decor(ph);
        divoom_fish_tank_clamp_pos(ph);
        divoom_fish_tank_second_clamp_pos(ph);
        divoom_fish_tank_apply_fish_visual(ph);
        divoom_fish_tank_second_apply_visual(ph);
        return 0;
    }

    ph->fish_img = lv_img_create(root);
    if (!ph->fish_img)
    {
        return -1;
    }

    ph->fish_img2 = lv_img_create(root);
    if (!ph->fish_img2)
    {
        return -1;
    }

    lv_obj_clear_flag(ph->fish_img, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(ph->fish_img, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_size(ph->fish_img, ph->params.fish_w, ph->params.fish_h);
    lv_img_set_size_mode(ph->fish_img, LV_IMG_SIZE_MODE_REAL);
    lv_img_set_pivot(ph->fish_img, ph->params.fish_w / 2, ph->params.fish_h / 2);
    lv_img_set_antialias(ph->fish_img, true);
    lv_obj_set_style_transform_pivot_x(ph->fish_img, ph->params.fish_w / 2, 0);
    lv_obj_set_style_transform_pivot_y(ph->fish_img, ph->params.fish_h / 2, 0);
    lv_obj_set_style_img_opa(ph->fish_img, LV_OPA_COVER, 0);
    lv_obj_set_style_opa(ph->fish_img, LV_OPA_COVER, 0);

    lv_obj_clear_flag(ph->fish_img2, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(ph->fish_img2, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_size(ph->fish_img2, ph->params.fish_w, ph->params.fish_h);
    lv_img_set_size_mode(ph->fish_img2, LV_IMG_SIZE_MODE_REAL);
    lv_img_set_pivot(ph->fish_img2, ph->params.fish_w / 2, ph->params.fish_h / 2);
    lv_img_set_antialias(ph->fish_img2, true);
    lv_obj_set_style_transform_pivot_x(ph->fish_img2, ph->params.fish_w / 2, 0);
    lv_obj_set_style_transform_pivot_y(ph->fish_img2, ph->params.fish_h / 2, 0);
    lv_obj_set_style_img_opa(ph->fish_img2, LV_OPA_COVER, 0);
    lv_obj_set_style_opa(ph->fish_img2, LV_OPA_COVER, 0);

    ph->fish_fallback = lv_obj_create(root);
    if (!ph->fish_fallback)
    {
        return -1;
    }
    lv_obj_remove_style_all(ph->fish_fallback);
    lv_obj_set_size(ph->fish_fallback, DIVOOM_FISH_TANK_FALLBACK_W, DIVOOM_FISH_TANK_FALLBACK_H);
    lv_obj_set_style_bg_opa(ph->fish_fallback, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ph->fish_fallback, 0, 0);
    lv_obj_set_style_pad_all(ph->fish_fallback, 0, 0);
    lv_obj_set_style_transform_pivot_x(ph->fish_fallback, DIVOOM_FISH_TANK_FALLBACK_W / 2, 0);
    lv_obj_set_style_transform_pivot_y(ph->fish_fallback, DIVOOM_FISH_TANK_FALLBACK_H / 2, 0);

    ph->fish_fallback_tail = lv_obj_create(ph->fish_fallback);
    ph->fish_fallback_body = lv_obj_create(ph->fish_fallback);
    ph->fish_fallback_fin = lv_obj_create(ph->fish_fallback);
    ph->fish_fallback_eye = lv_obj_create(ph->fish_fallback);
    if (!ph->fish_fallback_tail || !ph->fish_fallback_body ||
        !ph->fish_fallback_fin || !ph->fish_fallback_eye)
    {
        return -1;
    }

    lv_obj_remove_style_all(ph->fish_fallback_tail);
    lv_obj_set_size(ph->fish_fallback_tail, 12, 12);
    lv_obj_set_style_radius(ph->fish_fallback_tail, 2, 0);
    lv_obj_set_style_bg_color(ph->fish_fallback_tail, lv_color_hex(0x1685D9), 0);
    lv_obj_set_style_bg_opa(ph->fish_fallback_tail, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(ph->fish_fallback_tail, 1, 0);
    lv_obj_set_style_border_color(ph->fish_fallback_tail, lv_color_hex(0xEAFBFF), 0);
    lv_obj_set_style_transform_pivot_x(ph->fish_fallback_tail, 6, 0);
    lv_obj_set_style_transform_pivot_y(ph->fish_fallback_tail, 6, 0);

    lv_obj_remove_style_all(ph->fish_fallback_body);
    lv_obj_set_size(ph->fish_fallback_body, 28, 16);
    lv_obj_set_style_radius(ph->fish_fallback_body, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(ph->fish_fallback_body, lv_color_hex(0x35C8FF), 0);
    lv_obj_set_style_bg_opa(ph->fish_fallback_body, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(ph->fish_fallback_body, 2, 0);
    lv_obj_set_style_border_color(ph->fish_fallback_body, lv_color_hex(0xEAFBFF), 0);

    lv_obj_remove_style_all(ph->fish_fallback_fin);
    lv_obj_set_size(ph->fish_fallback_fin, 8, 5);
    lv_obj_set_style_radius(ph->fish_fallback_fin, 3, 0);
    lv_obj_set_style_bg_color(ph->fish_fallback_fin, lv_color_hex(0x8DE7FF), 0);
    lv_obj_set_style_bg_opa(ph->fish_fallback_fin, LV_OPA_COVER, 0);

    lv_obj_remove_style_all(ph->fish_fallback_eye);
    lv_obj_set_size(ph->fish_fallback_eye, 4, 4);
    lv_obj_set_style_radius(ph->fish_fallback_eye, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(ph->fish_fallback_eye, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(ph->fish_fallback_eye, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(ph->fish_fallback_eye, 1, 0);
    lv_obj_set_style_border_color(ph->fish_fallback_eye, lv_color_hex(0x062B4C), 0);

    lv_obj_clear_flag(ph->fish_fallback, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(ph->fish_fallback, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(ph->fish_fallback);
    lv_obj_move_foreground(ph->fish_img);
    lv_obj_move_foreground(ph->fish_img2);

    ph->facing_lr = 1;
    ph->sprite_lr_applied = 0xFF;
    ph->sprite_action_applied = 0xFF;
    ph->sprite_frame_applied = 0xFF;
    ph->anim_action = DIVOOM_FISH_TANK_ANIM_NORMAL;
    ph->anim_frame = 0;
    ph->anim_elapsed_ms = 0;
    ph->action_remain_ms = divoom_fish_tank_rand_range_u32(ph,
        DIVOOM_FISH_TANK_ACTION_MIN_MS, DIVOOM_FISH_TANK_ACTION_MAX_MS);
    divoom_fish_tank_params_sync_fixed(ph);
    ph->pos_x_q8 = divoom_fish_tank_q8_from_i32(ph->area_w - ph->params.fish_w) / 2;
    ph->pos_y_q8 = divoom_fish_tank_q8_from_i32(ph->area_h - ph->params.fish_h) / 2;
    ph->heading_ang = 0;
    ph->speed_q8 = ph->v_min_q8;
    ph->disp_pitch_deg10 = 0;
    ph->turn_visual_deg10 = 0;
    ph->cruise_pct = (uint8)divoom_fish_tank_rand_range_i32(ph, 92, 104);
    ph->cruise2_pct = (uint8)divoom_fish_tank_rand_range_i32(ph, 82, 96);
    ph->cruise_remain_ms = divoom_fish_tank_rand_range_u32(ph, 2200u, 5200u);
    ph->cruise2_remain_ms = divoom_fish_tank_rand_range_u32(ph, 2600u, 5800u);
    divoom_fish_tank_reset_bubbles(ph);
    divoom_fish_tank_sync_vel(ph);
    divoom_fish_tank_pick_target(ph);
    divoom_fish_tank_apply_fish_visual(ph);
    divoom_fish_tank_second_init(ph);

    if (!ph->steer_timer)
    {
        ph->steer_timer = lv_timer_create(divoom_fish_tank_steering_tick,
            DIVOOM_FISH_TANK_TICK_MS, ph);
    }

    ph->inner_ready = 1;
    divoom_print("[fish_tank] ready area=%dx%d sprite=%s/%s\n",
        (int)ph->area_w, (int)ph->area_h, ph->img_r, ph->img_l);
    return 0;
}

static void divoom_fish_tank_event_cb(lv_event_t *e)
{
    lv_obj_t *obj;
    divoom_fish_tank_t *ph;

    obj = lv_event_get_target(e);
    ph = divoom_fish_tank_get_obj(obj);
    if (!ph)
    {
        return;
    }

    if (lv_event_get_code(e) == LV_EVENT_SIZE_CHANGED)
    {
        divoom_fish_tank_update_area_from_obj(ph, obj);
        divoom_fish_tank_ensure_inner(ph);
        divoom_fish_tank_clamp_pos(ph);
        divoom_fish_tank_second_clamp_pos(ph);
        divoom_fish_tank_apply_fish_visual(ph);
        divoom_fish_tank_second_apply_visual(ph);
    }
}

static void divoom_fish_tank_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj)
{
    divoom_fish_tank_t *ph;

    (void)class_p;
    ph = divoom_fish_tank_get_obj(obj);
    if (!ph)
    {
        return;
    }

    divoom_fish_tank_params_init_default(&ph->params);
    divoom_fish_tank_params_sync_fixed(ph);
    strncpy(ph->img_r, DIVOOM_FISH_TANK_IMG_R_DEFAULT, sizeof(ph->img_r) - 1);
    ph->img_r[sizeof(ph->img_r) - 1] = '\0';
    strncpy(ph->img_l, DIVOOM_FISH_TANK_IMG_L_DEFAULT, sizeof(ph->img_l) - 1);
    ph->img_l[sizeof(ph->img_l) - 1] = '\0';
    ph->rand_seed = (uint32)lv_tick_get() ^ 0xF1A00001u ^ (uint32)(uintptr_t)obj;
    ph->active = 1;
    lv_obj_add_event_cb(obj, divoom_fish_tank_event_cb, LV_EVENT_SIZE_CHANGED, NULL);
}

static void divoom_fish_tank_destructor(const lv_obj_class_t *class_p, lv_obj_t *obj)
{
    divoom_fish_tank_t *ph;

    (void)class_p;
    ph = divoom_fish_tank_get_obj(obj);
    if (!ph)
    {
        return;
    }

    if (ph->steer_timer)
    {
        lv_timer_del(ph->steer_timer);
        ph->steer_timer = NULL;
    }

    if (ph->styles_inited)
    {
        lv_style_reset(&ph->root_style);
        ph->styles_inited = 0;
    }
}

lv_obj_t *divoom_fish_tank_create(lv_obj_t *parent)
{
    lv_obj_t *obj;

    obj = lv_obj_class_create_obj(DIVOOM_FISH_TANK_CLASS, parent);
    if (!obj)
    {
        return NULL;
    }

    lv_obj_class_init_obj(obj);
    return obj;
}

void divoom_fish_tank_set_seed(lv_obj_t *obj, uint32_t seed)
{
    divoom_fish_tank_t *ph;

    ph = divoom_fish_tank_get_obj(obj);
    if (!ph)
    {
        return;
    }

    ph->rand_seed = seed;
}

void divoom_fish_tank_set_params(lv_obj_t *obj, const divoom_fish_tank_params_t *p)
{
    divoom_fish_tank_t *ph;

    ph = divoom_fish_tank_get_obj(obj);
    if (!ph || !p)
    {
        return;
    }

    ph->params = *p;
    divoom_fish_tank_params_sync_fixed(ph);
    if (ph->speed_q8 > ph->v_max_q8)
    {
        ph->speed_q8 = ph->v_max_q8;
    }
    if (ph->speed2_q8 > ph->v_max_q8)
    {
        ph->speed2_q8 = ph->v_max_q8;
    }
    if (ph->speed_q8 < 0)
    {
        ph->speed_q8 = 0;
    }
    if (ph->speed2_q8 < 0)
    {
        ph->speed2_q8 = 0;
    }
    divoom_fish_tank_sync_vel(ph);
    divoom_fish_tank_second_sync_vel(ph);
    if (ph->fish_img)
    {
        lv_obj_set_size(ph->fish_img, ph->params.fish_w, ph->params.fish_h);
        lv_img_set_pivot(ph->fish_img, ph->params.fish_w / 2, ph->params.fish_h / 2);
        lv_obj_set_style_transform_pivot_x(ph->fish_img, ph->params.fish_w / 2, 0);
        lv_obj_set_style_transform_pivot_y(ph->fish_img, ph->params.fish_h / 2, 0);
    }

    if (ph->fish_img2)
    {
        lv_obj_set_size(ph->fish_img2, ph->params.fish_w, ph->params.fish_h);
        lv_img_set_pivot(ph->fish_img2, ph->params.fish_w / 2, ph->params.fish_h / 2);
        lv_obj_set_style_transform_pivot_x(ph->fish_img2, ph->params.fish_w / 2, 0);
        lv_obj_set_style_transform_pivot_y(ph->fish_img2, ph->params.fish_h / 2, 0);
    }

    if (ph->inner_ready)
    {
        divoom_fish_tank_clamp_pos(ph);
        divoom_fish_tank_second_clamp_pos(ph);
    }
}

void divoom_fish_tank_set_sprite(lv_obj_t *obj, const char *img_right, const char *img_left)
{
    divoom_fish_tank_t *ph;

    ph = divoom_fish_tank_get_obj(obj);
    if (!ph)
    {
        return;
    }

    if (img_right && img_right[0] != '\0')
    {
        strncpy(ph->img_r, img_right, sizeof(ph->img_r) - 1);
        ph->img_r[sizeof(ph->img_r) - 1] = '\0';
    }

    if (img_left && img_left[0] != '\0')
    {
        strncpy(ph->img_l, img_left, sizeof(ph->img_l) - 1);
        ph->img_l[sizeof(ph->img_l) - 1] = '\0';
    }

    if (ph->fish_img)
    {
        ph->sprite_lr_applied = 0xFF;
        ph->sprite_action_applied = 0xFF;
        ph->sprite_frame_applied = 0xFF;
        ph->sprite2_lr_applied = 0xFF;
        ph->sprite2_action_applied = 0xFF;
        ph->sprite2_frame_applied = 0xFF;
        ph->anim_action = DIVOOM_FISH_TANK_ANIM_NORMAL;
        ph->anim_frame = 0;
        ph->anim2_action = DIVOOM_FISH_TANK_ANIM_WIDE;
        if (!divoom_fish_tank_anim_action_available(ph->anim2_action))
        {
            ph->anim2_action = DIVOOM_FISH_TANK_ANIM_NORMAL;
        }
        ph->anim2_frame = 0;
        ph->anim_elapsed_ms = 0;
        ph->anim2_elapsed_ms = 0;
        divoom_fish_tank_apply_fish_visual(ph);
        divoom_fish_tank_second_apply_visual(ph);
    }
}

void divoom_fish_tank_set_active(lv_obj_t *obj, uint8 active)
{
    divoom_fish_tank_t *ph;

    ph = divoom_fish_tank_get_obj(obj);
    if (!ph)
    {
        return;
    }

    ph->active = active ? 1 : 0;
}

#include "divoom_astro_core_spectrum.h"

#include <stdlib.h>
#include <string.h>

#define ASTRO_CORE_DEFAULT_INNER_R 118
#define ASTRO_CORE_DEFAULT_OUTER_R 205
#define ASTRO_CORE_DEFAULT_RAYS    48
#define ASTRO_CORE_TRIGO_MAX       32767
#define ASTRO_CORE_BASE_RAYS       96
#define ASTRO_CORE_RING_SEGMENTS   22
#define ASTRO_CORE_NODE_COUNT      8

typedef struct _astro_core_spectrum_ray_t
{
    uint8_t shown;
} astro_core_spectrum_ray_t;

typedef struct _astro_core_spectrum_ctrl_t
{
    lv_obj_t *root;
    astro_core_spectrum_ray_t rays[DIVOOM_ASTRO_CORE_SPECTRUM_MAX_RAYS];
    void *buf;
    size_t buf_size;
    int size;
    int center;
    int inner_radius;
    int outer_radius;
    int ray_count;
    uint16_t refresh_ms;
    uint8_t eq_level;
    lv_color_t color_blue;
    lv_color_t color_pink;
} astro_core_spectrum_ctrl_t;

static int astro_core_clamp_int(int v, int lo, int hi)
{
    if (v < lo)
        return lo;
    if (v > hi)
        return hi;
    return v;
}

static uint8_t astro_core_clamp_u8(int v)
{
    if (v < 0)
        return 0;
    if (v > 255)
        return 255;
    return (uint8_t)v;
}

static int astro_core_angle_norm(int angle)
{
    angle %= 360;
    if (angle < 0)
        angle += 360;
    return angle;
}

static lv_coord_t astro_core_point_x(int cx, int radius, int angle)
{
    return (lv_coord_t)(cx + (radius * lv_trigo_cos(astro_core_angle_norm(angle))) / ASTRO_CORE_TRIGO_MAX);
}

static lv_coord_t astro_core_point_y(int cy, int radius, int angle)
{
    return (lv_coord_t)(cy + (radius * lv_trigo_sin(astro_core_angle_norm(angle))) / ASTRO_CORE_TRIGO_MAX);
}

static astro_core_spectrum_ctrl_t *astro_core_get(lv_obj_t *obj)
{
    if (!obj)
        return NULL;
    return (astro_core_spectrum_ctrl_t *)lv_obj_get_user_data(obj);
}

static lv_color_t astro_core_mix_color(lv_color_t a, lv_color_t b, int mix)
{
    mix = astro_core_clamp_int(mix, 0, 255);
    return lv_color_mix(b, a, (uint8_t)mix);
}

static lv_color_t astro_core_color_at(astro_core_spectrum_ctrl_t *sp, int idx, int count)
{
    int phase;

    if (!sp || count <= 1)
        return lv_color_hex(0xC044FF);

    phase = (idx * 255) / (count - 1);
    if (phase > 127)
        phase = 255 - phase;
    phase *= 2;
    return astro_core_mix_color(sp->color_blue, sp->color_pink, phase);
}

static void astro_core_style_plain(lv_obj_t *obj)
{
    if (!obj)
        return;
    lv_obj_remove_style_all(obj);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

static void astro_core_update_one_ray(astro_core_spectrum_ctrl_t *sp, int idx, uint8_t amp, uint8_t eq_level)
{
    astro_core_spectrum_ray_t *ray;
    int target;
    int contrast;

    if (!sp || idx < 0 || idx >= sp->ray_count)
        return;

    ray = &sp->rays[idx];

    if (amp < 64)
        contrast = (int)amp / 2;
    else
        contrast = 64 + (((int)amp - 64) * 3) / 2;
    target = contrast + ((int)eq_level / 16);
    target = astro_core_clamp_int(target, 0, 255);
    ray->shown = astro_core_clamp_u8(((int)ray->shown * 2 + target) / 3);
}

static void astro_core_draw_line(lv_layer_t *layer, lv_point_t p1, lv_point_t p2,
    lv_color_t color, int width, lv_opa_t opa)
{
    lv_draw_line_dsc_t line_dsc;

    lv_draw_line_dsc_init(&line_dsc);
    line_dsc.p1 = p1;
    line_dsc.p2 = p2;
    line_dsc.color = color;
    line_dsc.width = (lv_coord_t)width;
    line_dsc.opa = opa;
    line_dsc.round_start = 1;
    line_dsc.round_end = 1;
    lv_draw_line(layer, &line_dsc);
}

static void astro_core_draw_radial_line(lv_layer_t *layer, astro_core_spectrum_ctrl_t *sp,
    int angle, int r1, int r2, lv_color_t color, int width, lv_opa_t opa)
{
    lv_point_t p1;
    lv_point_t p2;

    p1.x = astro_core_point_x(sp->center, r1, angle);
    p1.y = astro_core_point_y(sp->center, r1, angle);
    p2.x = astro_core_point_x(sp->center, r2, angle);
    p2.y = astro_core_point_y(sp->center, r2, angle);
    astro_core_draw_line(layer, p1, p2, color, width, opa);
}

static void astro_core_draw_tangent_line(lv_layer_t *layer, astro_core_spectrum_ctrl_t *sp,
    int angle, int radius, int half_angle, lv_color_t color, int width, lv_opa_t opa)
{
    lv_point_t p1;
    lv_point_t p2;

    p1.x = astro_core_point_x(sp->center, radius, angle - half_angle);
    p1.y = astro_core_point_y(sp->center, radius, angle - half_angle);
    p2.x = astro_core_point_x(sp->center, radius, angle + half_angle);
    p2.y = astro_core_point_y(sp->center, radius, angle + half_angle);
    astro_core_draw_line(layer, p1, p2, color, width, opa);
}

static void astro_core_draw_broken_ring(astro_core_spectrum_ctrl_t *sp, lv_layer_t *layer)
{
    int i;
    int ring_r = sp->inner_radius - 22;
    int node_r = ring_r + 1;
    int pulse = (int)sp->eq_level / 38;

    if (ring_r < 40)
        return;

    for (i = 0; i < ASTRO_CORE_RING_SEGMENTS; i++) {
        int angle = i * 360 / ASTRO_CORE_RING_SEGMENTS;
        lv_color_t color = astro_core_color_at(sp, i, ASTRO_CORE_RING_SEGMENTS);
        int glow_opa = 18 + ((i & 1) ? 8 : 0);
        int core_opa = 70 + ((i % 5) == 0 ? 52 : 14);

        astro_core_draw_tangent_line(layer, sp, angle, ring_r + 1, 4, color, 5, (lv_opa_t)glow_opa);
        astro_core_draw_tangent_line(layer, sp, angle, ring_r, 3, color, 2, (lv_opa_t)core_opa);
    }

    for (i = 0; i < ASTRO_CORE_NODE_COUNT; i++) {
        int angle = i * 360 / ASTRO_CORE_NODE_COUNT;
        lv_color_t color = astro_core_color_at(sp, i, ASTRO_CORE_NODE_COUNT);
        lv_point_t p1;
        lv_point_t p2;

        p1.x = astro_core_point_x(sp->center, node_r - 2 - pulse, angle);
        p1.y = astro_core_point_y(sp->center, node_r - 2 - pulse, angle);
        p2.x = astro_core_point_x(sp->center, node_r + 2 + pulse, angle);
        p2.y = astro_core_point_y(sp->center, node_r + 2 + pulse, angle);
        astro_core_draw_line(layer, p1, p2, color, 3 + pulse, (lv_opa_t)150);
        astro_core_draw_line(layer, p1, p2, lv_color_hex(0xFFFFFF), 1, (lv_opa_t)140);
    }
}

static void astro_core_apply_layout(astro_core_spectrum_ctrl_t *sp)
{
    if (!sp || !sp->root)
        return;

    sp->outer_radius = astro_core_clamp_int(sp->outer_radius, sp->inner_radius + 12, 230);
    sp->inner_radius = astro_core_clamp_int(sp->inner_radius, 8, sp->outer_radius - 8);
    sp->size = sp->outer_radius * 2 + 8;
    sp->center = sp->size / 2;

    if (sp->buf) {
        free(sp->buf);
        sp->buf = NULL;
        sp->buf_size = 0;
    }
    sp->buf_size = (size_t)LV_CANVAS_BUF_SIZE_TRUE_COLOR_ALPHA(sp->size, sp->size);
    sp->buf = malloc(sp->buf_size);
    if (!sp->buf)
        return;

    lv_obj_set_size(sp->root, sp->size, sp->size);
    lv_canvas_set_buffer(sp->root, sp->buf, sp->size, sp->size, LV_COLOR_FORMAT_ARGB8888);
}

static void astro_core_render(astro_core_spectrum_ctrl_t *sp)
{
    lv_layer_t layer;
    int i;

    if (!sp || !sp->root || !sp->buf)
        return;

    lv_canvas_fill_bg(sp->root, lv_color_black(), LV_OPA_TRANSP);
    lv_canvas_init_layer(sp->root, &layer);

    for (i = 0; i < ASTRO_CORE_BASE_RAYS; i++) {
        int angle = i * 360 / ASTRO_CORE_BASE_RAYS;
        int ray_idx = (i * sp->ray_count) / ASTRO_CORE_BASE_RAYS;
        uint8_t shown = sp->rays[ray_idx].shown;
        int jitter = (i * 7) % 13;
        int inner = sp->inner_radius + 7 + (i & 1);
        int outer = inner + 10 + jitter + ((int)shown * 38) / 255;
        lv_opa_t opa = (lv_opa_t)astro_core_clamp_int(22 + ((int)shown * 70) / 255 + ((i % 3) * 4), 22, 100);
        lv_color_t color = astro_core_color_at(sp, i, ASTRO_CORE_BASE_RAYS);

        astro_core_draw_radial_line(&layer, sp, angle, inner, outer, color,
            shown > 185 ? 2 : 1, opa);
    }

    astro_core_draw_broken_ring(sp, &layer);

    for (i = 0; i < sp->ray_count; i++) {
        int len_range = sp->outer_radius - (sp->inner_radius + 36);
        int len;
        int inner;
        int outer;
        uint8_t shown = sp->rays[i].shown;
        int angle = i * 360 / sp->ray_count;
        int base_len;
        lv_color_t color = astro_core_color_at(sp, i, sp->ray_count);
        lv_opa_t glow_opa;
        lv_opa_t core_opa;

        if (len_range < 8)
            len_range = 8;
        inner = sp->inner_radius + 19 + ((i & 1) ? 2 : 0);
        base_len = 4 + ((i * 5) % 5);
        len = base_len + (len_range * (int)shown) / 250;
        outer = astro_core_clamp_int(inner + len, inner + 5, sp->outer_radius - 12);
        glow_opa = (lv_opa_t)astro_core_clamp_int(16 + ((int)shown * 92) / 255, 16, 108);
        core_opa = (lv_opa_t)astro_core_clamp_int(46 + ((int)shown * 160) / 255, 46, 206);

        astro_core_draw_radial_line(&layer, sp, angle, inner, outer, color,
            shown > 175 ? 4 : 3, glow_opa);
        astro_core_draw_radial_line(&layer, sp, angle, inner + 2, outer - 1, color,
            shown > 175 ? 2 : 1, core_opa);
    }
    lv_canvas_finish_layer(sp->root, &layer);
}

static void astro_core_event_cb(lv_event_t *e)
{
    lv_obj_t *obj;
    astro_core_spectrum_ctrl_t *sp;
    lv_event_code_t code;

    if (!e)
        return;
    code = lv_event_get_code(e);
    obj = lv_event_get_target(e);
    sp = astro_core_get(obj);
    if (!sp)
        return;
    if (code != LV_EVENT_DELETE)
        return;
    if (sp->buf)
        free(sp->buf);
    free(sp);
    lv_obj_set_user_data(obj, NULL);
}

lv_obj_t *divoom_astro_core_spectrum_create(lv_obj_t *parent,
    const divoom_astro_core_spectrum_cfg_t *cfg)
{
    lv_obj_t *root;
    astro_core_spectrum_ctrl_t *sp;
    int center_x;
    int center_y;
    int i;

    if (!parent)
        return NULL;

    root = lv_canvas_create(parent);
    if (!root)
        return NULL;
    astro_core_style_plain(root);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_opa(root, LV_OPA_TRANSP, 0);

    sp = (astro_core_spectrum_ctrl_t *)calloc(1, sizeof(*sp));
    if (!sp) {
        lv_obj_del(root);
        return NULL;
    }

    sp->root = root;
    sp->inner_radius = cfg ? cfg->inner_radius : ASTRO_CORE_DEFAULT_INNER_R;
    sp->outer_radius = cfg ? cfg->outer_radius : ASTRO_CORE_DEFAULT_OUTER_R;
    sp->ray_count = cfg ? cfg->ray_count : ASTRO_CORE_DEFAULT_RAYS;
    sp->ray_count = astro_core_clamp_int(sp->ray_count, 8, DIVOOM_ASTRO_CORE_SPECTRUM_MAX_RAYS);
    sp->refresh_ms = cfg && cfg->refresh_ms ? cfg->refresh_ms : 100;
    sp->color_blue = cfg ? cfg->color_blue : lv_color_hex(0x115DFF);
    sp->color_pink = cfg ? cfg->color_pink : lv_color_hex(0xF13BFF);

    lv_obj_set_user_data(root, sp);
    lv_obj_add_event(root, astro_core_event_cb, LV_EVENT_DELETE, NULL);
    for (i = 0; i < sp->ray_count; i++)
        sp->rays[i].shown = (uint8_t)(50 + (i % 4) * 8);

    astro_core_apply_layout(sp);
    astro_core_render(sp);

    center_x = cfg ? cfg->center_x : -1;
    center_y = cfg ? cfg->center_y : -1;
    if (center_x >= 0 && center_y >= 0) {
        lv_obj_set_pos(root, center_x - sp->center, center_y - sp->center);
    }
    else {
        lv_obj_center(root);
    }

    return root;
}

void divoom_astro_core_spectrum_set_bands(lv_obj_t *obj,
    const uint8_t bands[DIVOOM_ASTRO_CORE_SPECTRUM_BANDS],
    uint8_t eq_level)
{
    astro_core_spectrum_ctrl_t *sp = astro_core_get(obj);
    int i;

    if (!sp || !bands)
        return;

    sp->eq_level = eq_level;
    for (i = 0; i < sp->ray_count; i++) {
        int band_idx = (i * DIVOOM_ASTRO_CORE_SPECTRUM_BANDS) / sp->ray_count;
        if (band_idx >= DIVOOM_ASTRO_CORE_SPECTRUM_BANDS)
            band_idx = DIVOOM_ASTRO_CORE_SPECTRUM_BANDS - 1;
        astro_core_update_one_ray(sp, i, bands[band_idx], eq_level);
    }
    astro_core_render(sp);
}

void divoom_astro_core_spectrum_set_radius(lv_obj_t *obj, int inner_radius, int outer_radius)
{
    astro_core_spectrum_ctrl_t *sp = astro_core_get(obj);

    if (!sp)
        return;
    sp->inner_radius = inner_radius;
    sp->outer_radius = outer_radius;
    astro_core_apply_layout(sp);
    astro_core_render(sp);
}

void divoom_astro_core_spectrum_set_theme(lv_obj_t *obj, lv_color_t color_blue, lv_color_t color_pink)
{
    astro_core_spectrum_ctrl_t *sp = astro_core_get(obj);

    if (!sp)
        return;
    sp->color_blue = color_blue;
    sp->color_pink = color_pink;
    astro_core_render(sp);
}

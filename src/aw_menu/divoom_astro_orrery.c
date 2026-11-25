#include "divoom_astro_orrery.h"

#include "divoom.h"
#include "lvgl.h"
#include "middle/include/divoom_lvgl9_compat.h"

#define DIVOOM_ASTRO_ORRERY_CLASS       &divoom_astro_orrery_class
#define DIVOOM_ORRERY_TICK_MS           1000u
#define DIVOOM_ORRERY_DAY_SEC           86400u
#define DIVOOM_ORRERY_MOON_SEC          2551443u
#define DIVOOM_ORRERY_SUN_BREATH_SEC    8u

typedef struct
{
    lv_obj_t obj;
    lv_timer_t *timer;
    uint8 active;
    uint8 time_override_en;
    uint32 time_override_utc;
} divoom_astro_orrery_t;

static void divoom_astro_orrery_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj);
static void divoom_astro_orrery_destructor(const lv_obj_class_t *class_p, lv_obj_t *obj);
static void divoom_astro_orrery_event_cb(lv_event_t *e);
static void divoom_astro_orrery_tick_cb(lv_timer_t *timer);

const lv_obj_class_t divoom_astro_orrery_class = {
    .constructor_cb = divoom_astro_orrery_constructor,
    .destructor_cb = divoom_astro_orrery_destructor,
    .width_def = 480,
    .height_def = 480,
    .instance_size = sizeof(divoom_astro_orrery_t),
    .base_class = &lv_obj_class,
};

static divoom_astro_orrery_t *divoom_astro_orrery_get_obj(lv_obj_t *obj)
{
    if (!obj || !lv_obj_check_type(obj, DIVOOM_ASTRO_ORRERY_CLASS))
    {
        return NULL;
    }

    return (divoom_astro_orrery_t *)obj;
}

static int32 divoom_astro_orrery_clamp_i32(int32 v, int32 min_v, int32 max_v)
{
    if (v < min_v)
    {
        return min_v;
    }
    if (v > max_v)
    {
        return max_v;
    }

    return v;
}

static uint8 divoom_astro_orrery_clamp_u8(int32 v)
{
    return (uint8)divoom_astro_orrery_clamp_i32(v, 0, 255);
}

static int16 divoom_astro_orrery_wrap_deg(int32 angle)
{
    angle %= 360;
    if (angle < 0)
    {
        angle += 360;
    }

    return (int16)angle;
}

static uint32 divoom_astro_orrery_get_time(divoom_astro_orrery_t *ph)
{
    if (ph->time_override_en)
    {
        return ph->time_override_utc;
    }

    return divoom_system_get_utc_time();
}

static lv_coord_t divoom_astro_orrery_orbit_x(lv_coord_t cx, int32 radius, int32 angle)
{
    return (lv_coord_t)(cx + ((lv_trigo_sin(divoom_astro_orrery_wrap_deg(angle)) * radius) >> LV_TRIGO_SHIFT));
}

static lv_coord_t divoom_astro_orrery_orbit_y(lv_coord_t cy, int32 radius, int32 angle)
{
    return (lv_coord_t)(cy - ((lv_trigo_sin(divoom_astro_orrery_wrap_deg(angle + 90)) * radius) >> LV_TRIGO_SHIFT));
}

static void divoom_astro_orrery_draw_rect(lv_layer_t *layer, const lv_area_t *area, uint32 color, lv_opa_t opa,
    lv_coord_t radius)
{
    lv_draw_rect_dsc_t dsc;

    if (opa == 0)
    {
        return;
    }

    lv_draw_rect_dsc_init(&dsc);
    dsc.bg_color = lv_color_hex(color);
    dsc.bg_opa = opa;
    dsc.radius = radius;
    lv_draw_rect(layer, &dsc, area);
}

static void divoom_astro_orrery_draw_circle(lv_layer_t *layer, lv_coord_t cx, lv_coord_t cy, lv_coord_t radius,
    uint32 color, lv_opa_t opa)
{
    lv_area_t area;

    if (radius <= 0)
    {
        return;
    }

    area.x1 = (lv_coord_t)(cx - radius);
    area.y1 = (lv_coord_t)(cy - radius);
    area.x2 = (lv_coord_t)(cx + radius);
    area.y2 = (lv_coord_t)(cy + radius);
    divoom_astro_orrery_draw_rect(layer, &area, color, opa, LV_RADIUS_CIRCLE);
}

static void divoom_astro_orrery_draw_line(lv_layer_t *layer, lv_coord_t x1, lv_coord_t y1, lv_coord_t x2,
    lv_coord_t y2, uint32 color, lv_opa_t opa, lv_coord_t width)
{
    lv_draw_line_dsc_t dsc;

    if (opa == 0 || width <= 0)
    {
        return;
    }

    lv_draw_line_dsc_init(&dsc);
    dsc.p1.x = x1;
    dsc.p1.y = y1;
    dsc.p2.x = x2;
    dsc.p2.y = y2;
    dsc.color = lv_color_hex(color);
    dsc.opa = opa;
    dsc.width = width;
    lv_draw_line(layer, &dsc);
}

static void divoom_astro_orrery_draw_arc(lv_layer_t *layer, lv_coord_t cx, lv_coord_t cy, uint16 radius,
    uint32 color, lv_opa_t opa, lv_coord_t width)
{
    lv_draw_arc_dsc_t dsc;

    if (opa == 0 || radius == 0 || width <= 0)
    {
        return;
    }

    lv_draw_arc_dsc_init(&dsc);
    dsc.center.x = cx;
    dsc.center.y = cy;
    dsc.radius = radius;
    dsc.start_angle = 0;
    dsc.end_angle = 360;
    dsc.width = width;
    dsc.color = lv_color_hex(color);
    dsc.opa = opa;
    dsc.rounded = 1;
    lv_draw_arc(layer, &dsc);
}

static void divoom_astro_orrery_draw_ring(lv_layer_t *layer, lv_coord_t cx, lv_coord_t cy, lv_coord_t radius,
    uint32 color, lv_opa_t opa, lv_coord_t width)
{
    lv_area_t area;
    lv_draw_rect_dsc_t dsc;

    if (radius <= 0 || opa == 0 || width <= 0)
    {
        return;
    }

    area.x1 = (lv_coord_t)(cx - radius);
    area.y1 = (lv_coord_t)(cy - radius);
    area.x2 = (lv_coord_t)(cx + radius);
    area.y2 = (lv_coord_t)(cy + radius);

    lv_draw_rect_dsc_init(&dsc);
    dsc.bg_opa = LV_OPA_TRANSP;
    dsc.radius = LV_RADIUS_CIRCLE;
    dsc.border_color = lv_color_hex(color);
    dsc.border_opa = opa;
    dsc.border_width = width;
    lv_draw_rect(layer, &dsc, &area);
}

static uint16 divoom_astro_orrery_angle_from_period(uint32 utc_sec, uint32 period_sec)
{
    uint32 pos;

    if (period_sec == 0)
    {
        return 0;
    }

    pos = utc_sec % period_sec;
    return (uint16)((pos * 360u) / period_sec);
}

static lv_coord_t divoom_astro_orrery_scale_len(int32 side, int32 design_len)
{
    return (lv_coord_t)((side * design_len) / 480);
}

static void divoom_astro_orrery_draw_space(lv_layer_t *layer, const lv_area_t *coords, lv_coord_t cx,
    lv_coord_t cy, lv_coord_t side)
{
    static const int16 star_pos[][3] = {
        { 72, 92, 2 }, { 116, 338, 1 }, { 188, 76, 1 }, { 304, 82, 1 }, { 390, 126, 2 },
        { 430, 282, 1 }, { 72, 256, 1 }, { 154, 410, 1 }, { 356, 392, 1 }, { 228, 438, 1 },
    };
    lv_area_t bg;
    uint8 i;
    lv_coord_t x;
    lv_coord_t y;
    lv_coord_t r;

    bg = *coords;
    divoom_astro_orrery_draw_rect(layer, &bg, 0x02060Bu, LV_OPA_COVER, 0);

    /* 背景只保留星空和暗色盘面，不出现任何文字信息。 */
    divoom_astro_orrery_draw_circle(layer, cx, cy, divoom_astro_orrery_scale_len(side, 238), 0x071525u, 138);
    divoom_astro_orrery_draw_circle(layer, cx, cy, divoom_astro_orrery_scale_len(side, 214), 0x020810u, 158);
    divoom_astro_orrery_draw_circle(layer, cx, cy, divoom_astro_orrery_scale_len(side, 166), 0x07111Bu, 86);

    for (i = 0; i < (uint8)(sizeof(star_pos) / sizeof(star_pos[0])); i++)
    {
        x = (lv_coord_t)(coords->x1 + divoom_astro_orrery_scale_len(side, star_pos[i][0]));
        y = (lv_coord_t)(coords->y1 + divoom_astro_orrery_scale_len(side, star_pos[i][1]));
        r = divoom_astro_orrery_scale_len(side, star_pos[i][2]);
        divoom_astro_orrery_draw_circle(layer, x, y, r, 0xE7C07Cu, 150);
    }
}

static void divoom_astro_orrery_draw_dial_ticks(lv_layer_t *layer, lv_coord_t cx, lv_coord_t cy, lv_coord_t side)
{
    uint16 i;
    int32 angle;
    lv_coord_t outer_r;
    lv_coord_t inner_r;
    lv_coord_t x1;
    lv_coord_t y1;
    lv_coord_t x2;
    lv_coord_t y2;
    lv_opa_t opa;
    lv_coord_t width;

    outer_r = divoom_astro_orrery_scale_len(side, 203);
    for (i = 0; i < 72; i++)
    {
        angle = (int32)i * 5;
        inner_r = (i % 6 == 0) ? divoom_astro_orrery_scale_len(side, 188)
            : divoom_astro_orrery_scale_len(side, 194);
        opa = (i % 6 == 0) ? 140 : 72;
        width = (i % 6 == 0) ? 2 : 1;
        x1 = divoom_astro_orrery_orbit_x(cx, outer_r, angle);
        y1 = divoom_astro_orrery_orbit_y(cy, outer_r, angle);
        x2 = divoom_astro_orrery_orbit_x(cx, inner_r, angle);
        y2 = divoom_astro_orrery_orbit_y(cy, inner_r, angle);
        divoom_astro_orrery_draw_line(layer, x1, y1, x2, y2, 0xD69A42u, opa, width);
    }
}

static void divoom_astro_orrery_draw_sun(lv_layer_t *layer, lv_coord_t cx, lv_coord_t cy, lv_coord_t r,
    uint32 utc_sec)
{
    int32 breath_angle;
    int32 wave;
    int32 glow_add;
    lv_coord_t glow_r;

    /* 太阳的 8 秒呼吸光效只改变透明度和外圈半径，不影响天体角度。 */
    breath_angle = (int32)((utc_sec % DIVOOM_ORRERY_SUN_BREATH_SEC) * 360u / DIVOOM_ORRERY_SUN_BREATH_SEC);
    wave = lv_trigo_sin(divoom_astro_orrery_wrap_deg(breath_angle));
    glow_add = 4 + ((wave + (1 << LV_TRIGO_SHIFT)) * 7) / (2 << LV_TRIGO_SHIFT);

    /*
     * 光晕必须保持同心、低对比度渐变；硬边 ring 会让太阳像被脏边包住，
     * 因此这里只用多层半透明圆从外到内平滑过渡。
     */
    glow_r = (lv_coord_t)(r + glow_add);
    divoom_astro_orrery_draw_circle(layer, cx, cy, (lv_coord_t)(glow_r + (r * 11) / 10), 0xB85E08u, 12);
    divoom_astro_orrery_draw_circle(layer, cx, cy, (lv_coord_t)(glow_r + (r * 9) / 10), 0xD9760Bu, 18);
    divoom_astro_orrery_draw_circle(layer, cx, cy, (lv_coord_t)(glow_r + (r * 7) / 10), 0xEE8F12u, 26);
    divoom_astro_orrery_draw_circle(layer, cx, cy, (lv_coord_t)(glow_r + (r * 5) / 10), 0xFFAD27u, 38);
    divoom_astro_orrery_draw_circle(layer, cx, cy, (lv_coord_t)(glow_r + (r * 3) / 10), 0xFFD060u, 58);
    divoom_astro_orrery_draw_circle(layer, cx, cy, (lv_coord_t)(glow_r + (r * 1) / 10), 0xFFE08Au, 82);
    divoom_astro_orrery_draw_circle(layer, cx, cy, r, 0xF39A18u, LV_OPA_COVER);
    divoom_astro_orrery_draw_circle(layer, cx, cy, (lv_coord_t)((r * 5) / 7), 0xFFD36Au, 220);
    divoom_astro_orrery_draw_circle(layer, (lv_coord_t)(cx - r / 4), (lv_coord_t)(cy - r / 4),
        (lv_coord_t)(r / 3), 0xFFF3B0u, 128);
    divoom_astro_orrery_draw_circle(layer, (lv_coord_t)(cx + r / 5), (lv_coord_t)(cy + r / 6),
        (lv_coord_t)(r / 5), 0xC85A08u, 50);
    divoom_astro_orrery_draw_line(layer, (lv_coord_t)(cx - r / 2), (lv_coord_t)(cy + r / 5),
        (lv_coord_t)(cx + r / 2), (lv_coord_t)(cy - r / 4), 0xFFE19Au, 52, 2);
    divoom_astro_orrery_draw_line(layer, (lv_coord_t)(cx - r / 3), (lv_coord_t)(cy - r / 2),
        (lv_coord_t)(cx + r / 3), (lv_coord_t)(cy + r / 3), 0xFFF0B8u, 42, 1);
}

static void divoom_astro_orrery_draw_earth(lv_layer_t *layer, lv_coord_t cx, lv_coord_t cy, lv_coord_t r,
    uint16 earth_angle)
{
    lv_coord_t shade_x;

    lv_coord_t shade_y;

    divoom_astro_orrery_draw_circle(layer, cx, cy, (lv_coord_t)(r + 7), 0x88D8FFu, 36);
    divoom_astro_orrery_draw_circle(layer, cx, cy, (lv_coord_t)(r + 2), 0xD7F2FFu, 78);
    divoom_astro_orrery_draw_circle(layer, cx, cy, r, 0x175D95u, LV_OPA_COVER);
    divoom_astro_orrery_draw_circle(layer, (lv_coord_t)(cx - r / 5), (lv_coord_t)(cy - r / 5),
        (lv_coord_t)((r * 3) / 4), 0x2D9AD9u, 110);
    divoom_astro_orrery_draw_circle(layer, (lv_coord_t)(cx - r / 4), (lv_coord_t)(cy - r / 5),
        (lv_coord_t)(r / 2), 0x58B681u, 194);
    divoom_astro_orrery_draw_circle(layer, (lv_coord_t)(cx + r / 5), (lv_coord_t)(cy + r / 5),
        (lv_coord_t)(r / 3), 0xB59B51u, 168);
    divoom_astro_orrery_draw_circle(layer, (lv_coord_t)(cx - r / 7), (lv_coord_t)(cy + r / 4),
        (lv_coord_t)(r / 4), 0xD6B95Cu, 104);
    divoom_astro_orrery_draw_line(layer, (lv_coord_t)(cx - r / 2), (lv_coord_t)(cy + r / 3),
        (lv_coord_t)(cx + r / 2), (lv_coord_t)(cy - r / 4), 0x8DD5FFu, 70, 2);

    shade_x = (lv_coord_t)(cx + ((lv_trigo_sin(divoom_astro_orrery_wrap_deg(earth_angle)) * r) >> LV_TRIGO_SHIFT));
    shade_y = (lv_coord_t)(cy - ((lv_trigo_sin(divoom_astro_orrery_wrap_deg(earth_angle + 90)) * r)
        >> LV_TRIGO_SHIFT));
    divoom_astro_orrery_draw_circle(layer, shade_x, shade_y, (lv_coord_t)(r + 4), 0x020611u, 132);
    divoom_astro_orrery_draw_ring(layer, cx, cy, r, 0x87DBFFu, 80, 1);
    divoom_astro_orrery_draw_circle(layer, (lv_coord_t)(cx - r / 3), (lv_coord_t)(cy - r / 3),
        (lv_coord_t)(r / 4), 0xFFFFFFu, 48);
}

static void divoom_astro_orrery_draw_moon(lv_layer_t *layer, lv_coord_t cx, lv_coord_t cy, lv_coord_t r,
    uint16 moon_angle)
{
    lv_coord_t shade_x;

    divoom_astro_orrery_draw_circle(layer, cx, cy, (lv_coord_t)(r + 2), 0xF1E7D8u, 64);
    divoom_astro_orrery_draw_circle(layer, cx, cy, r, 0xB8B2A9u, LV_OPA_COVER);
    divoom_astro_orrery_draw_circle(layer, (lv_coord_t)(cx - r / 4), (lv_coord_t)(cy - r / 5),
        (lv_coord_t)(r / 4), 0xE6DFD2u, 72);
    divoom_astro_orrery_draw_circle(layer, (lv_coord_t)(cx + r / 4), (lv_coord_t)(cy + r / 4),
        (lv_coord_t)(r / 5), 0x5D5852u, 80);
    divoom_astro_orrery_draw_circle(layer, (lv_coord_t)(cx - r / 7), (lv_coord_t)(cy + r / 5),
        (lv_coord_t)(r / 6), 0x6A645Du, 62);

    shade_x = (lv_coord_t)(cx + ((lv_trigo_sin(divoom_astro_orrery_wrap_deg(moon_angle + 180)) * r)
        >> LV_TRIGO_SHIFT));
    divoom_astro_orrery_draw_circle(layer, shade_x, cy, (lv_coord_t)(r + 1), 0x040404u, 130);
}

static void divoom_astro_orrery_draw_scene(divoom_astro_orrery_t *ph, lv_layer_t *layer, const lv_area_t *coords)
{
    lv_coord_t side;
    lv_coord_t cx;
    lv_coord_t cy;
    lv_coord_t sun_r;
    lv_coord_t earth_r;
    lv_coord_t moon_r;
    lv_coord_t earth_orbit_r;
    lv_coord_t moon_orbit_r;
    lv_coord_t earth_x;
    lv_coord_t earth_y;
    lv_coord_t moon_x;
    lv_coord_t moon_y;
    uint32 utc_sec;
    uint16 earth_angle;
    uint16 moon_angle;
    lv_area_t bg;

    side = LV_MIN(lv_area_get_width(coords), lv_area_get_height(coords));
    cx = (lv_coord_t)((coords->x1 + coords->x2) / 2);
    cy = (lv_coord_t)((coords->y1 + coords->y2) / 2);
    utc_sec = divoom_astro_orrery_get_time(ph);
    earth_angle = divoom_astro_orrery_angle_from_period(utc_sec, DIVOOM_ORRERY_DAY_SEC);
    moon_angle = divoom_astro_orrery_angle_from_period(utc_sec, DIVOOM_ORRERY_MOON_SEC);

    /* 按参考表盘主视觉比例放大：太阳系占据屏幕主体，避免成为角落小装饰。 */
    sun_r = divoom_astro_orrery_scale_len(side, 52);
    earth_r = divoom_astro_orrery_scale_len(side, 42);
    moon_r = divoom_astro_orrery_scale_len(side, 15);
    earth_orbit_r = divoom_astro_orrery_scale_len(side, 142);
    moon_orbit_r = divoom_astro_orrery_scale_len(side, 55);

    earth_x = divoom_astro_orrery_orbit_x(cx, earth_orbit_r, earth_angle);
    earth_y = divoom_astro_orrery_orbit_y(cy, earth_orbit_r, earth_angle);
    moon_x = divoom_astro_orrery_orbit_x(earth_x, moon_orbit_r, moon_angle);
    moon_y = divoom_astro_orrery_orbit_y(earth_y, moon_orbit_r, moon_angle);

    bg = *coords;
    divoom_astro_orrery_draw_space(layer, &bg, cx, cy, side);
    divoom_astro_orrery_draw_ring(layer, cx, cy, divoom_astro_orrery_scale_len(side, 218), 0xC98B38u, 68, 2);
    divoom_astro_orrery_draw_ring(layer, cx, cy, divoom_astro_orrery_scale_len(side, 206), 0x4D3821u, 84, 1);
    divoom_astro_orrery_draw_dial_ticks(layer, cx, cy, side);
    divoom_astro_orrery_draw_arc(layer, cx, cy, (uint16)earth_orbit_r, 0xC98B38u, 86, 1);
    divoom_astro_orrery_draw_arc(layer, cx, cy, (uint16)divoom_astro_orrery_scale_len(side, 96), 0x745226u, 44, 1);
    divoom_astro_orrery_draw_arc(layer, cx, cy, (uint16)divoom_astro_orrery_scale_len(side, 192), 0x4C3822u, 36, 1);
    divoom_astro_orrery_draw_arc(layer, earth_x, earth_y, (uint16)moon_orbit_r, 0xB68A4Au, 76, 1);

    divoom_astro_orrery_draw_line(layer, cx, cy, earth_x, earth_y, 0xA8722Eu, 52, 1);
    divoom_astro_orrery_draw_line(layer, earth_x, earth_y, moon_x, moon_y, 0xAFA28Eu, 48, 1);

    /* 按 y 值粗略排序，让绕行天体在上/下半圈时有自然遮挡关系。 */
    if (earth_y < cy)
    {
        divoom_astro_orrery_draw_earth(layer, earth_x, earth_y, earth_r, earth_angle);
        divoom_astro_orrery_draw_moon(layer, moon_x, moon_y, moon_r, moon_angle);
        divoom_astro_orrery_draw_sun(layer, cx, cy, sun_r, utc_sec);
    }
    else
    {
        divoom_astro_orrery_draw_sun(layer, cx, cy, sun_r, utc_sec);
        divoom_astro_orrery_draw_earth(layer, earth_x, earth_y, earth_r, earth_angle);
        divoom_astro_orrery_draw_moon(layer, moon_x, moon_y, moon_r, moon_angle);
    }
}

static void divoom_astro_orrery_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    divoom_astro_orrery_t *ph = divoom_astro_orrery_get_obj(obj);

    if (!ph)
    {
        return;
    }

    if (code == LV_EVENT_DRAW_MAIN)
    {
        lv_layer_t *layer = lv_event_get_layer(e);
        lv_area_t coords;

        lv_obj_get_coords(obj, &coords);
        divoom_astro_orrery_draw_scene(ph, layer, &coords);
    }
    else if (code == LV_EVENT_SIZE_CHANGED)
    {
        lv_obj_invalidate(obj);
    }
}

static void divoom_astro_orrery_tick_cb(lv_timer_t *timer)
{
    lv_obj_t *obj = (lv_obj_t *)lv_timer_get_user_data(timer);
    divoom_astro_orrery_t *ph = divoom_astro_orrery_get_obj(obj);

    if (!ph || !ph->active)
    {
        return;
    }

    lv_obj_invalidate(obj);
}

static void divoom_astro_orrery_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj)
{
    divoom_astro_orrery_t *ph;

    (void)class_p;

    ph = divoom_astro_orrery_get_obj(obj);
    if (!ph)
    {
        return;
    }

    ph->active = 1;
    ph->time_override_en = 0;
    ph->time_override_utc = 0;

    lv_obj_remove_style_all(obj);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event(obj, divoom_astro_orrery_event_cb, LV_EVENT_DRAW_MAIN, NULL);
    lv_obj_add_event(obj, divoom_astro_orrery_event_cb, LV_EVENT_SIZE_CHANGED, NULL);
    ph->timer = lv_timer_create(divoom_astro_orrery_tick_cb, DIVOOM_ORRERY_TICK_MS, obj);
}

static void divoom_astro_orrery_destructor(const lv_obj_class_t *class_p, lv_obj_t *obj)
{
    divoom_astro_orrery_t *ph;

    (void)class_p;

    ph = divoom_astro_orrery_get_obj(obj);
    if (!ph)
    {
        return;
    }

    if (ph->timer)
    {
        lv_timer_del(ph->timer);
        ph->timer = NULL;
    }
}

lv_obj_t *divoom_astro_orrery_create(lv_obj_t *parent)
{
    lv_obj_t *obj = lv_obj_class_create_obj(DIVOOM_ASTRO_ORRERY_CLASS, parent);

    if (!obj)
    {
        return NULL;
    }

    lv_obj_class_init_obj(obj);
    return obj;
}

void divoom_astro_orrery_set_active(lv_obj_t *obj, uint8 active)
{
    divoom_astro_orrery_t *ph = divoom_astro_orrery_get_obj(obj);

    if (!ph)
    {
        return;
    }

    ph->active = active ? 1 : 0;
    lv_obj_invalidate(obj);
}

void divoom_astro_orrery_set_time_override(lv_obj_t *obj, uint8 enable, uint32 utc_sec)
{
    divoom_astro_orrery_t *ph = divoom_astro_orrery_get_obj(obj);

    if (!ph)
    {
        return;
    }

    ph->time_override_en = enable ? 1 : 0;
    ph->time_override_utc = utc_sec;
    lv_obj_invalidate(obj);
}

#include "divoom_fire_breath.h"

#include "divoom.h"
#include "lvgl.h"

#include <string.h>

#define DIVOOM_FIRE_BREATH_CLASS              &divoom_fire_breath_class
#define DIVOOM_FIRE_BREATH_FRAME_W            220
#define DIVOOM_FIRE_BREATH_FRAME_H            196
#define DIVOOM_FIRE_CANVAS_W                  160
#define DIVOOM_FIRE_CANVAS_H                  180
#define DIVOOM_FIRE_TICK_MS                   110u
#define DIVOOM_FIRE_BREATH_MS                 3600u

typedef struct
{
    int16 x;
    int16 base_y;
    int16 width;
    int16 height;
    int16 sway;
    int16 tip_shift;
    uint32 color;
    uint8 opa;
    uint16 phase_offset;
} divoom_fire_lobe_t;

typedef struct
{
    lv_obj_t obj;
    lv_obj_t *canvas;
    lv_timer_t *timer;
    uint8 *canvas_buf;
    uint8 *soft_buf;
    uint32 canvas_buf_size;
    uint32 phase_ms;
    uint32 last_tick;
    uint32 rand_seed;
    uint8 active;
    lv_coord_t canvas_base_x;
    lv_coord_t canvas_base_y;
} divoom_fire_breath_t;

static void divoom_fire_breath_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj);
static void divoom_fire_breath_destructor(const lv_obj_class_t *class_p, lv_obj_t *obj);
static void divoom_fire_breath_tick_cb(lv_timer_t *timer);

const lv_obj_class_t divoom_fire_breath_class = {
    .constructor_cb = divoom_fire_breath_constructor,
    .destructor_cb = divoom_fire_breath_destructor,
    .width_def = DIVOOM_FIRE_BREATH_FRAME_W,
    .height_def = DIVOOM_FIRE_BREATH_FRAME_H,
    .instance_size = sizeof(divoom_fire_breath_t),
    .base_class = &lv_obj_class,
};

static const divoom_fire_lobe_t s_fire_lobes[] = {
    { 80, 172, 82, 154, 5, -3, 0x7D1706, 190, 0u },
    { 80, 172, 42, 118, 8, -28, 0xC93108, 210, 730u },
    { 80, 172, 44, 128, 8, 30, 0xEB5B0A, 218, 1240u },
    { 80, 172, 48, 138, 6, 8, 0xFF7A16, 232, 1860u },
    { 80, 172, 28, 96, 4, -5, 0xFFE778, 244, 2480u },
    { 78, 172, 16, 142, 11, -20, 0xF05A0B, 118, 310u },
    { 84, 172, 15, 128, 10, 22, 0xFF8A1C, 126, 1510u },
    { 73, 172, 11, 100, 13, -32, 0xB92606, 92, 930u },
    { 90, 172, 11, 108, 13, 34, 0xD74308, 98, 2210u },
};

static divoom_fire_breath_t *divoom_fire_breath_get_obj(lv_obj_t *obj)
{
    if (!obj || !lv_obj_check_type(obj, DIVOOM_FIRE_BREATH_CLASS))
    {
        return NULL;
    }

    return (divoom_fire_breath_t *)obj;
}

static int32 divoom_fire_abs_i32(int32 value)
{
    return value < 0 ? -value : value;
}

static int32 divoom_fire_clamp_i32(int32 value, int32 min, int32 max)
{
    if (value < min)
    {
        return min;
    }
    if (value > max)
    {
        return max;
    }

    return value;
}

static uint32 divoom_fire_triangle_norm(uint32 phase_ms, uint32 period_ms)
{
    uint32 phase;
    uint32 half;

    phase = phase_ms % period_ms;
    half = period_ms / 2u;
    if (phase > half)
    {
        phase = period_ms - phase;
    }

    return (phase * 255u) / half;
}

static uint32 divoom_fire_smooth_norm(uint32 norm)
{
    if (norm > 255u)
    {
        norm = 255u;
    }

    return (norm * norm * (765u - 2u * norm)) / (255u * 255u);
}

static uint32 divoom_fire_wave(uint32 phase_ms, uint32 period_ms, uint32 offset_ms)
{
    return divoom_fire_smooth_norm(divoom_fire_triangle_norm(phase_ms + offset_ms, period_ms));
}

static int32 divoom_fire_noise_i32(uint32 x, uint32 y, uint32 seed)
{
    uint32 n;

    n = x * 374761393u + y * 668265263u + seed * 362437u;
    n = (n ^ (n >> 13)) * 1274126177u;
    n ^= n >> 16;
    return (int32)(n & 0xFFu) - 128;
}

static uint8 divoom_fire_color_r(uint32 color)
{
    return (uint8)((color >> 16) & 0xFFu);
}

static uint8 divoom_fire_color_g(uint32 color)
{
    return (uint8)((color >> 8) & 0xFFu);
}

static uint8 divoom_fire_color_b(uint32 color)
{
    return (uint8)(color & 0xFFu);
}

static void divoom_fire_mix_hot(uint32 base_color, uint32 hot_norm, uint8 *r, uint8 *g, uint8 *b)
{
    uint32 inv;

    if (hot_norm > 255u)
    {
        hot_norm = 255u;
    }

    inv = 255u - hot_norm;
    *r = (uint8)((divoom_fire_color_r(base_color) * inv + 255u * hot_norm) / 255u);
    *g = (uint8)((divoom_fire_color_g(base_color) * inv + 228u * hot_norm) / 255u);
    *b = (uint8)((divoom_fire_color_b(base_color) * inv + 150u * hot_norm) / 255u);
}

static void divoom_fire_clear_canvas(divoom_fire_breath_t *ph)
{
    if (!ph || !ph->canvas_buf)
    {
        return;
    }

    memset(ph->canvas_buf, 0, ph->canvas_buf_size);
}

static void divoom_fire_put_px(divoom_fire_breath_t *ph,
    int32 x,
    int32 y,
    uint8 r,
    uint8 g,
    uint8 b,
    uint8 a)
{
    lv_color32_t *dst;
    uint32 dst_a;
    uint32 inv_a;
    uint32 out_a;

    if (!ph || !ph->canvas_buf || a == 0)
    {
        return;
    }
    if (x < 0 || x >= DIVOOM_FIRE_CANVAS_W || y < 0 || y >= DIVOOM_FIRE_CANVAS_H)
    {
        return;
    }

    dst = ((lv_color32_t *)ph->canvas_buf) + y * DIVOOM_FIRE_CANVAS_W + x;
    dst_a = dst->alpha;
    inv_a = 255u - a;
    out_a = a + (dst_a * inv_a) / 255u;
    if (out_a == 0)
    {
        dst->red = 0;
        dst->green = 0;
        dst->blue = 0;
        dst->alpha = 0;
        return;
    }

    dst->red = (uint8)(((uint32)r * a + (uint32)dst->red * dst_a * inv_a / 255u) / out_a);
    dst->green = (uint8)(((uint32)g * a + (uint32)dst->green * dst_a * inv_a / 255u) / out_a);
    dst->blue = (uint8)(((uint32)b * a + (uint32)dst->blue * dst_a * inv_a / 255u) / out_a);
    dst->alpha = (uint8)out_a;
}

static void divoom_fire_draw_lobe(divoom_fire_breath_t *ph,
    const divoom_fire_lobe_t *lobe,
    uint32 breath,
    uint32 phase_ms)
{
    int32 y;
    int32 height;
    int32 width;
    int32 base_y;
    uint32 local_wave;

    if (!ph || !ph->canvas || !lobe)
    {
        return;
    }

    local_wave = divoom_fire_wave(phase_ms, 2100u, lobe->phase_offset);
    height = lobe->height + (int32)((breath * lobe->height) / 255u) / 10;
    height += ((int32)local_wave - 128) / 16;
    width = lobe->width + ((int32)local_wave - 128) / 18;
    base_y = lobe->base_y;

    /*
      火舌是代码栅格化出的半透明 shape：每层有独立高度、宽度、alpha 和局部弯曲。
      这里不是 SVG，也不是 PNG 切帧；每次 timer 都重新生成 raster canvas。
    */
    for (y = base_y - height; y <= base_y; y++)
    {
        int32 x;
        int32 rel;
        int32 t;
        int32 shape;
        int32 half_w;
        int32 center_x;
        int32 bend;
        int32 edge_wave;
        int32 side_notch;
        int32 vertical_fade;

        if (y < 0 || y >= DIVOOM_FIRE_CANVAS_H)
        {
            continue;
        }

        rel = base_y - y;
        t = (rel * 255) / height;
        shape = ((255 - t) * (128 + t / 3)) / 255;
        half_w = (width * shape) / 230;
        if (half_w <= 0)
        {
            continue;
        }

        edge_wave = divoom_fire_noise_i32((uint32)y / 3u, lobe->phase_offset, phase_ms / 96u);
        side_notch = divoom_fire_noise_i32((uint32)y / 5u, lobe->phase_offset + 17u, phase_ms / 130u);
        bend = (((int32)local_wave - 128) * lobe->sway * t) / (128 * 255);
        bend += (lobe->tip_shift * t * t) / (255 * 255);
        bend += (edge_wave * t) / 920;
        center_x = lobe->x + bend;
        half_w += (side_notch * (255 - t)) / 4200;
        if (half_w < 1)
        {
            continue;
        }
        vertical_fade = t < 34 ? (t * 255) / 34 : 255;

        for (x = center_x - half_w; x <= center_x + half_w; x++)
        {
            int32 dist;
            int32 edge;
            int32 mask;
            int32 alpha;
            uint32 hot;
            uint8 r;
            uint8 g;
            uint8 b;

            if (x < 0 || x >= DIVOOM_FIRE_CANVAS_W)
            {
                continue;
            }

            dist = divoom_fire_abs_i32(x - center_x);
            edge = half_w - dist;
            if (edge <= 0)
            {
                continue;
            }

            mask = edge >= 6 ? 255 : (edge * 255) / 6;
            alpha = ((int32)lobe->opa * mask) / 255;
            alpha = (alpha * vertical_fade) / 255;
            alpha = (alpha * (196 + (int32)((breath * 59u) / 255u))) / 255;
            alpha = divoom_fire_clamp_i32(alpha + 18, 0, 250);
            if (alpha <= 0)
            {
                continue;
            }

            hot = (uint32)divoom_fire_clamp_i32(255 - dist * 255 / half_w, 0, 255);
            hot = (hot * (uint32)t) / 255u;
            divoom_fire_mix_hot(lobe->color, hot, &r, &g, &b);
            divoom_fire_put_px(ph, x, y, r, g, b, (uint8)alpha);
        }
    }
}

static void divoom_fire_draw_base_glow(divoom_fire_breath_t *ph, uint32 breath)
{
    int32 x;
    int32 y;
    int32 cx;
    int32 cy;
    int32 rx;
    int32 ry;

    if (!ph || !ph->canvas)
    {
        return;
    }

    cx = DIVOOM_FIRE_CANVAS_W / 2;
    cy = 166;
    rx = 58 + (int32)((breath * 5u) / 255u);
    ry = 16;
    for (y = cy - ry; y <= cy + ry; y++)
    {
        for (x = cx - rx; x <= cx + rx; x++)
        {
            int32 dx;
            int32 dy;
            int32 dist;
            int32 alpha;

            if (x < 0 || x >= DIVOOM_FIRE_CANVAS_W || y < 0 || y >= DIVOOM_FIRE_CANVAS_H)
            {
                continue;
            }

            dx = ((x - cx) * 255) / rx;
            dy = ((y - cy) * 255) / ry;
            dist = (dx * dx + dy * dy) / 255;
            if (dist >= 255)
            {
                continue;
            }

            alpha = ((255 - dist) * (74 + (int32)((breath * 36u) / 255u))) / 255;
            divoom_fire_put_px(ph, x, y, 0xFFu, 0x6Au, 0x10u, (uint8)alpha);
        }
    }
}

static void divoom_fire_draw_sparks(divoom_fire_breath_t *ph)
{
    int32 i;

    if (!ph || !ph->canvas)
    {
        return;
    }

    for (i = 0; i < 10; i++)
    {
        uint32 p;
        int32 x;
        int32 y;
        int32 alpha;

        p = (ph->phase_ms / 37u + (uint32)i * 53u + ph->rand_seed) % 900u;
        x = DIVOOM_FIRE_CANVAS_W / 2 - 42 + (int32)((p * 37u + (uint32)i * 29u) % 85u);
        y = 24 + (int32)((p + (uint32)i * 71u) % 110u);
        alpha = 110 - (int32)(p % 90u);
        if (alpha <= 0 || x < 0 || x >= DIVOOM_FIRE_CANVAS_W || y < 0 || y >= DIVOOM_FIRE_CANVAS_H)
        {
            continue;
        }

        divoom_fire_put_px(ph, x, y, 0xFFu, 0xD2u, 0x6Bu, (uint8)alpha);
    }
}

static void divoom_fire_soften_canvas(divoom_fire_breath_t *ph)
{
    lv_color32_t *src;
    lv_color32_t *dst;
    int32 x;
    int32 y;

    if (!ph || !ph->canvas_buf || !ph->soft_buf)
    {
        return;
    }

    src = (lv_color32_t *)ph->canvas_buf;
    dst = (lv_color32_t *)ph->soft_buf;
    memcpy(ph->soft_buf, ph->canvas_buf, ph->canvas_buf_size);

    for (y = 1; y < DIVOOM_FIRE_CANVAS_H - 1; y++)
    {
        for (x = 1; x < DIVOOM_FIRE_CANVAS_W - 1; x++)
        {
            lv_color32_t *p;
            lv_color32_t *u;
            lv_color32_t *d;
            lv_color32_t *l;
            lv_color32_t *r;
            uint32 sum_a;

            p = &src[y * DIVOOM_FIRE_CANVAS_W + x];
            u = &src[(y - 1) * DIVOOM_FIRE_CANVAS_W + x];
            d = &src[(y + 1) * DIVOOM_FIRE_CANVAS_W + x];
            l = &src[y * DIVOOM_FIRE_CANVAS_W + x - 1];
            r = &src[y * DIVOOM_FIRE_CANVAS_W + x + 1];
            sum_a = p->alpha * 4u + u->alpha * 2u + d->alpha * 2u + l->alpha + r->alpha;
            if (sum_a == 0)
            {
                continue;
            }

            dst[y * DIVOOM_FIRE_CANVAS_W + x].alpha = (uint8)(sum_a / 10u);
            dst[y * DIVOOM_FIRE_CANVAS_W + x].red =
                (uint8)((p->red * 4u + u->red * 2u + d->red * 2u + l->red + r->red) / 10u);
            dst[y * DIVOOM_FIRE_CANVAS_W + x].green =
                (uint8)((p->green * 4u + u->green * 2u + d->green * 2u + l->green + r->green) / 10u);
            dst[y * DIVOOM_FIRE_CANVAS_W + x].blue =
                (uint8)((p->blue * 4u + u->blue * 2u + d->blue * 2u + l->blue + r->blue) / 10u);
        }
    }

    memcpy(ph->canvas_buf, ph->soft_buf, ph->canvas_buf_size);
}

static void divoom_fire_breath_render(divoom_fire_breath_t *ph)
{
    uint32 breath;
    uint32 i;

    if (!ph || !ph->canvas)
    {
        return;
    }

    breath = divoom_fire_smooth_norm(divoom_fire_triangle_norm(ph->phase_ms, DIVOOM_FIRE_BREATH_MS));
    divoom_fire_clear_canvas(ph);
    divoom_fire_draw_base_glow(ph, breath);
    for (i = 0; i < sizeof(s_fire_lobes) / sizeof(s_fire_lobes[0]); i++)
    {
        divoom_fire_draw_lobe(ph, &s_fire_lobes[i], breath, ph->phase_ms);
    }
    divoom_fire_draw_sparks(ph);
    divoom_fire_soften_canvas(ph);
    lv_obj_invalidate(ph->canvas);
    lv_obj_set_pos(ph->canvas, ph->canvas_base_x, ph->canvas_base_y);
}

static void divoom_fire_breath_tick_cb(lv_timer_t *timer)
{
    lv_obj_t *obj;
    divoom_fire_breath_t *ph;
    uint32 dt_ms;

    obj = (lv_obj_t *)lv_timer_get_user_data(timer);
    ph = divoom_fire_breath_get_obj(obj);
    if (!ph || !ph->active)
    {
        return;
    }

    dt_ms = ph->last_tick ? lv_tick_elaps(ph->last_tick) : DIVOOM_FIRE_TICK_MS;
    ph->last_tick = lv_tick_get();
    ph->phase_ms += dt_ms;
    divoom_fire_breath_render(ph);
}

static void divoom_fire_breath_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj)
{
    divoom_fire_breath_t *ph;

    (void)class_p;

    ph = divoom_fire_breath_get_obj(obj);
    if (!ph)
    {
        return;
    }

    ph->active = 1;
    ph->last_tick = lv_tick_get();
    ph->rand_seed = ph->last_tick ? ph->last_tick : 1u;

    lv_obj_remove_style_all(obj);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_size(obj, DIVOOM_FIRE_BREATH_FRAME_W, DIVOOM_FIRE_BREATH_FRAME_H);

    ph->canvas_buf_size = (uint32)LV_CANVAS_BUF_SIZE_TRUE_COLOR_ALPHA(
        DIVOOM_FIRE_CANVAS_W, DIVOOM_FIRE_CANVAS_H);
    ph->canvas_buf = (uint8 *)divoom_system_malloc(ph->canvas_buf_size);
    ph->soft_buf = (uint8 *)divoom_system_malloc(ph->canvas_buf_size);
    if (!ph->canvas_buf || !ph->soft_buf)
    {
        return;
    }

    ph->canvas = lv_canvas_create(obj);
    if (!ph->canvas)
    {
        return;
    }

    lv_obj_clear_flag(ph->canvas, LV_OBJ_FLAG_SCROLLABLE);
    lv_canvas_set_buffer(ph->canvas, ph->canvas_buf, DIVOOM_FIRE_CANVAS_W,
        DIVOOM_FIRE_CANVAS_H, LV_COLOR_FORMAT_ARGB8888);
    ph->canvas_base_x = (DIVOOM_FIRE_BREATH_FRAME_W - DIVOOM_FIRE_CANVAS_W) / 2;
    ph->canvas_base_y = DIVOOM_FIRE_BREATH_FRAME_H - DIVOOM_FIRE_CANVAS_H;
    divoom_fire_breath_render(ph);

    ph->timer = lv_timer_create(divoom_fire_breath_tick_cb, DIVOOM_FIRE_TICK_MS, obj);
}

static void divoom_fire_breath_destructor(const lv_obj_class_t *class_p, lv_obj_t *obj)
{
    divoom_fire_breath_t *ph;

    (void)class_p;

    ph = divoom_fire_breath_get_obj(obj);
    if (!ph)
    {
        return;
    }

    if (ph->timer)
    {
        lv_timer_del(ph->timer);
        ph->timer = NULL;
    }
    if (ph->canvas_buf)
    {
        divoom_system_free(ph->canvas_buf);
        ph->canvas_buf = NULL;
    }
    if (ph->soft_buf)
    {
        divoom_system_free(ph->soft_buf);
        ph->soft_buf = NULL;
    }
}

lv_obj_t *divoom_fire_breath_create(lv_obj_t *parent)
{
    lv_obj_t *obj;

    obj = lv_obj_class_create_obj(DIVOOM_FIRE_BREATH_CLASS, parent);
    if (!obj)
    {
        return NULL;
    }

    lv_obj_class_init_obj(obj);
    return obj;
}

void divoom_fire_breath_set_active(lv_obj_t *obj, uint8 active)
{
    divoom_fire_breath_t *ph;

    ph = divoom_fire_breath_get_obj(obj);
    if (!ph)
    {
        return;
    }

    ph->active = active ? 1u : 0u;
    ph->last_tick = lv_tick_get();
    divoom_fire_breath_render(ph);
}

void divoom_fire_breath_set_seed(lv_obj_t *obj, uint32 seed)
{
    divoom_fire_breath_t *ph;

    ph = divoom_fire_breath_get_obj(obj);
    if (!ph)
    {
        return;
    }

    ph->rand_seed = seed ? seed : 1u;
    ph->phase_ms = seed % DIVOOM_FIRE_BREATH_MS;
    ph->last_tick = lv_tick_get();
    divoom_fire_breath_render(ph);
}

void divoom_fire_breath_set_layout(lv_obj_t *obj, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h)
{
    divoom_fire_breath_t *ph;

    ph = divoom_fire_breath_get_obj(obj);
    if (!ph)
    {
        return;
    }

    if (w <= 0)
    {
        w = DIVOOM_FIRE_BREATH_FRAME_W;
    }
    if (h <= 0)
    {
        h = DIVOOM_FIRE_BREATH_FRAME_H;
    }

    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, w, h);
    ph->canvas_base_x = (w - DIVOOM_FIRE_CANVAS_W) / 2;
    ph->canvas_base_y = h - DIVOOM_FIRE_CANVAS_H;
    divoom_fire_breath_render(ph);
}

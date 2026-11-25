#include "divoom_astro_cell_growth.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define CELL_GROWTH_DEFAULT_REFRESH_MS 66
#define CELL_GROWTH_MIN_SIZE 16
#define CELL_GROWTH_MAX_SIZE 640
#define CELL_GROWTH_MAX_CELLS 160
#define CELL_GROWTH_TRIGO_MAX 32767

typedef struct _astro_cell_growth_cell_t
{
    int x;
    int y;
    int radius;
    int target_radius;
    int age;
    int life;
    int hue;
    int parent;
    int angle;
    int depth;
    uint8_t active;
    uint8_t pulse;
    uint8_t hot;
} astro_cell_growth_cell_t;

typedef struct _astro_cell_growth_ctrl_t
{
    lv_obj_t *root;
    lv_timer_t *timer;
    astro_cell_growth_cell_t *cells;
    void *buf;
    size_t buf_size;
    int width;
    int height;
    int bloom_x;
    int bloom_y;
    uint16_t refresh_ms;
    uint16_t max_cells;
    uint16_t cell_count;
    uint32_t rng;
    uint32_t frame;
    uint8_t paused;
    lv_color_t bg_color;
    lv_color_t teal_color;
    lv_color_t green_color;
    lv_color_t yellow_color;
    lv_color_t white_color;
} astro_cell_growth_ctrl_t;

typedef struct _cell_growth_field_point_t
{
    int x;
    int y;
    int r;
    int ax;
    int ay;
    int warm;
    int hot;
} cell_growth_field_point_t;

static int cell_growth_clamp_int(int v, int lo, int hi)
{
    if (v < lo)
        return lo;
    if (v > hi)
        return hi;
    return v;
}

static uint8_t cell_growth_clamp_u8(int v)
{
    if (v < 0)
        return 0;
    if (v > 255)
        return 255;
    return (uint8_t)v;
}

static uint32_t cell_growth_next_rand(astro_cell_growth_ctrl_t *cg)
{
    if (!cg)
        return 1;
    cg->rng = cg->rng * 1664525u + 1013904223u;
    return cg->rng;
}

static int cell_growth_rand_range(astro_cell_growth_ctrl_t *cg, int min_v, int max_v)
{
    uint32_t r;

    if (max_v <= min_v)
        return min_v;
    r = cell_growth_next_rand(cg);
    return min_v + (int)(r % (uint32_t)(max_v - min_v + 1));
}

static int cell_growth_angle_norm(int angle)
{
    angle %= 360;
    if (angle < 0)
        angle += 360;
    return angle;
}

static int cell_growth_cos(int angle)
{
    return lv_trigo_cos(cell_growth_angle_norm(angle));
}

static int cell_growth_sin(int angle)
{
    return lv_trigo_sin(cell_growth_angle_norm(angle));
}

static astro_cell_growth_ctrl_t *cell_growth_get(lv_obj_t *obj)
{
    if (!obj)
        return NULL;
    return (astro_cell_growth_ctrl_t *)lv_obj_get_user_data(obj);
}

static lv_color_t cell_growth_mix(lv_color_t a, lv_color_t b, int mix)
{
    mix = cell_growth_clamp_int(mix, 0, 255);
    return lv_color_mix(b, a, (uint8_t)mix);
}

static lv_color_t cell_growth_cell_color(astro_cell_growth_ctrl_t *cg, int hue)
{
    if (!cg)
        return lv_color_hex(0x73F3DF);
    hue = cell_growth_clamp_int(hue, 0, 255);
    if (hue < 96)
        return cell_growth_mix(cg->teal_color, cg->green_color, (hue * 255) / 96);
    if (hue < 196)
        return cell_growth_mix(cg->green_color, cg->yellow_color, ((hue - 96) * 255) / 100);
    return cell_growth_mix(cg->yellow_color, cg->white_color, ((hue - 196) * 255) / 59);
}

static void cell_growth_style_plain(lv_obj_t *obj)
{
    if (!obj)
        return;
    lv_obj_remove_style_all(obj);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

static void cell_growth_draw_circle(lv_layer_t *layer, int x, int y, int radius,
    lv_color_t color, lv_opa_t bg_opa, int border_w, lv_opa_t border_opa)
{
    lv_draw_rect_dsc_t dsc;
    lv_area_t area;

    if (!layer || radius <= 0)
        return;

    area.x1 = (lv_coord_t)(x - radius);
    area.y1 = (lv_coord_t)(y - radius);
    area.x2 = (lv_coord_t)(x + radius);
    area.y2 = (lv_coord_t)(y + radius);

    lv_draw_rect_dsc_init(&dsc);
    dsc.radius = LV_RADIUS_CIRCLE;
    dsc.bg_color = color;
    dsc.bg_opa = bg_opa;
    dsc.border_color = color;
    dsc.border_width = (lv_coord_t)border_w;
    dsc.border_opa = border_opa;
    dsc.border_side = LV_BORDER_SIDE_FULL;
    lv_draw_rect(layer, &dsc, &area);
}

static void cell_growth_draw_line(lv_layer_t *layer, int x1, int y1, int x2, int y2,
    lv_color_t color, int width, lv_opa_t opa)
{
    lv_draw_line_dsc_t dsc;

    if (!layer || width <= 0)
        return;

    lv_draw_line_dsc_init(&dsc);
    dsc.p1.x = (lv_coord_t)x1;
    dsc.p1.y = (lv_coord_t)y1;
    dsc.p2.x = (lv_coord_t)x2;
    dsc.p2.y = (lv_coord_t)y2;
    dsc.color = color;
    dsc.width = (lv_coord_t)width;
    dsc.opa = opa;
    dsc.round_start = 1;
    dsc.round_end = 1;
    lv_draw_line(layer, &dsc);
}

static int cell_growth_distance2(const astro_cell_growth_cell_t *a,
    const astro_cell_growth_cell_t *b)
{
    int dx;
    int dy;

    if (!a || !b)
        return 0;
    dx = a->x - b->x;
    dy = a->y - b->y;
    return dx * dx + dy * dy;
}

static uint32_t cell_growth_hash(uint32_t x)
{
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    return x;
}

static int cell_growth_lerp_int(int a, int b, int t)
{
    t = cell_growth_clamp_int(t, 0, 255);
    return a + ((b - a) * t) / 255;
}

static int cell_growth_color_r(lv_color_t c)
{
    return c.red;
}

static int cell_growth_color_g(lv_color_t c)
{
    return c.green;
}

static int cell_growth_color_b(lv_color_t c)
{
    return c.blue;
}

static int cell_growth_soft_noise(int x, int y, uint32_t frame)
{
    uint32_t h;

    h = cell_growth_hash((uint32_t)(x * 73856093u) ^
        (uint32_t)(y * 19349663u) ^ (frame * 83492791u));
    return (int)(h & 255u) - 128;
}

static int cell_growth_isqrt(int v)
{
    int r;

    if (v <= 0)
        return 0;
    r = (int)sqrt((double)v);
    return r;
}

static void cell_growth_make_point(astro_cell_growth_ctrl_t *cg, int idx,
    cell_growth_field_point_t *pt)
{
    uint32_t h;
    int angle;
    int dist;
    int radial;
    int sx;
    int sy;
    int drift;
    int center_x;
    int center_y;

    if (!cg || !pt)
        return;

    h = cell_growth_hash(0x115B100u + (uint32_t)idx * 2654435761u);
    angle = cell_growth_angle_norm(idx * 137 + (int)(h % 39u) - 19 +
        (int)(cg->frame / 18u));
    radial = cell_growth_isqrt((idx + 3) * 9200);
    dist = 8 + (radial * cg->width) / 2550;
    sx = (dist * (118 + (int)((h >> 20) & 35u))) / 100;
    sy = (dist * (92 + (int)((h >> 25) & 39u))) / 100;
    drift = (int)((cg->frame + (h & 31u)) % 64u);
    if (drift > 32)
        drift = 64 - drift;

    center_x = (cg->width * 68) / 100;
    center_y = (cg->height * 51) / 100;
    pt->x = center_x + (sx * cell_growth_cos(angle)) / CELL_GROWTH_TRIGO_MAX;
    pt->y = center_y + (sy * cell_growth_sin(angle)) / CELL_GROWTH_TRIGO_MAX;
    pt->x += (dist * 10) / 100;
    pt->x += (int)((h >> 10) & 31u) - 15;
    pt->y += (int)((h >> 15) & 25u) - 12;
    pt->x += ((drift - 16) * (int)((h >> 13) & 5u)) / 18;
    pt->y += ((drift - 16) * (int)((h >> 16) & 5u)) / 20;
    pt->r = 7 + (int)((h >> 14) % 18u);
    if ((h & 31u) == 0u)
        pt->r += 8;
    pt->ax = 82 + (int)((h >> 21) & 47u);
    pt->ay = 86 + (int)((h >> 26) & 43u);
    pt->warm = cell_growth_clamp_int(230 - dist / 2 +
        (int)((h >> 18) & 45u) + ((pt->x - cg->width / 2) / 6), 18, 246);
    pt->hot = ((h & 63u) == 0u || pt->warm > 234) ? 1 : 0;
}

static void cell_growth_write_pixel_block(astro_cell_growth_ctrl_t *cg,
    int x, int y, int r, int g, int b)
{
    lv_color32_t *dst;
    int xx;
    int yy;

    if (!cg || !cg->buf)
        return;

    for (yy = y; yy < y + 1 && yy < cg->height; yy++) {
        if (yy < 0)
            continue;
        dst = (lv_color32_t *)cg->buf + yy * cg->width;
        for (xx = x; xx < x + 1 && xx < cg->width; xx++) {
            if (xx < 0)
                continue;
            dst[xx].red = cell_growth_clamp_u8(r);
            dst[xx].green = cell_growth_clamp_u8(g);
            dst[xx].blue = cell_growth_clamp_u8(b);
            dst[xx].alpha = 255;
        }
    }
}

static void cell_growth_render_field_pixels(astro_cell_growth_ctrl_t *cg)
{
    cell_growth_field_point_t pts[96];
    int point_count;
    int visible_count;
    int growth;
    int i;
    int x;
    int y;
    int bg_r;
    int bg_g;
    int bg_b;

    if (!cg || !cg->buf)
        return;

    point_count = 84;
    growth = cell_growth_clamp_int((int)cg->frame * 9 + 150, 150, cg->width + 240);
    visible_count = 0;
    for (i = 0; i < point_count; i++) {
        cell_growth_field_point_t pt;
        int dx;
        int dy;
        int dist2;

        cell_growth_make_point(cg, i, &pt);
        dx = pt.x - (cg->width * 68) / 100;
        dy = pt.y - (cg->height * 51) / 100;
        dist2 = dx * dx + dy * dy;
        if (dist2 > growth * growth)
            continue;
        pts[visible_count++] = pt;
    }

    bg_r = cell_growth_color_r(cg->bg_color);
    bg_g = cell_growth_color_g(cg->bg_color);
    bg_b = cell_growth_color_b(cg->bg_color);

    for (y = 0; y < cg->height; y++) {
        for (x = 0; x < cg->width; x++) {
            int mass = 0;
            int glow = 0;
            int membrane = 0;
            int fill = 0;
            int cavity = 0;
            int core = 0;
            int warm_sum = 0;
            int weight_sum = 0;
            int n;
            int haze;
            int vignette;
            int warm;
            int r;
            int g;
            int b;

            {
                int hx = ((x - (cg->width * 72) / 100) * 100) / 138;
                int hy = ((y - (cg->height * 54) / 100) * 100) / 112;
                int hd = cell_growth_isqrt(hx * hx + hy * hy);
                haze = cell_growth_clamp_int(156 - hd, 0, 156);
            }
            for (n = 0; n < visible_count; n++) {
                int dx;
                int dy;
                int ex;
                int ey;
                int d2;
                int d;
                int reach;
                int reach2;
                int band_w;
                int band;
                int inner_r;
                int w;
                int soft_r;

                dx = x - pts[n].x;
                dy = y - pts[n].y;
                ex = (dx * 100) / pts[n].ax;
                ey = (dy * 100) / pts[n].ay;
                d2 = ex * ex + ey * ey;
                reach = (pts[n].r * 29) / 10;
                reach2 = reach * reach;
                if (d2 > reach2)
                    continue;
                d = cell_growth_isqrt(d2);

                w = ((reach - d) * (reach - d) * 54) / (reach * reach);
                mass += w;
                glow += w;
                warm_sum += w * pts[n].warm;
                weight_sum += w;

                soft_r = (pts[n].r * 19) / 10;
                if (d < soft_r) {
                    int soft = ((soft_r - d) * 92) / (soft_r + 1);
                    fill += soft;
                    warm_sum += soft * pts[n].warm;
                    weight_sum += soft;
                }

                band_w = pts[n].r / 3 + 5;
                band = d > pts[n].r ? d - pts[n].r : pts[n].r - d;
                if (band < band_w) {
                    w = ((band_w - band) * 150) / band_w;
                    membrane += w;
                    warm_sum += w * pts[n].warm;
                    weight_sum += w;
                }

                inner_r = (pts[n].r * 46) / 100;
                if (d < inner_r && !pts[n].hot && pts[n].r > 10) {
                    cavity += ((inner_r - d) * 92) / (inner_r + 1);
                }
                if (pts[n].hot && d < pts[n].r / 2 + 3) {
                    core += ((pts[n].r / 2 + 3 - d) * 132) / (pts[n].r / 2 + 4);
                    warm_sum += 180 * pts[n].warm;
                    weight_sum += 180;
                }
            }

            vignette = (x < cg->width / 3) ? ((cg->width / 3 - x) / 5) : 0;
            glow += haze / 2;
            mass += haze / 3;
            glow -= vignette;

            if (mass < 14 && membrane < 12 && core < 8) {
                r = bg_r;
                g = bg_g;
                b = bg_b;
                if (haze > 0) {
                    g += haze / 11;
                    b += haze / 5;
                }
                cell_growth_write_pixel_block(cg, x, y, r, g, b);
                continue;
            }

            warm = weight_sum > 0 ? warm_sum / weight_sum :
                cell_growth_clamp_int(210 - cell_growth_isqrt(
                    (x - (cg->width * 68) / 100) * (x - (cg->width * 68) / 100) +
                    (y - (cg->height * 54) / 100) * (y - (cg->height * 54) / 100)) / 2,
                    64, 218);
            mass = cell_growth_clamp_int(mass, 0, 240);
            glow = cell_growth_clamp_int(glow, 0, 190);
            membrane = cell_growth_clamp_int(membrane, 0, 230);
            fill = cell_growth_clamp_int(fill, 0, 220);
            cavity = cell_growth_clamp_int(cavity, 0, 210);
            core = cell_growth_clamp_int(core, 0, 220);

            if (warm < 92) {
                int t = (warm * 255) / 92;
                r = cell_growth_lerp_int(3, 16, t);
                g = cell_growth_lerp_int(28, 210, t);
                b = cell_growth_lerp_int(34, 222, t);
            }
            else if (warm < 178) {
                int t = ((warm - 92) * 255) / 86;
                r = cell_growth_lerp_int(16, 170, t);
                g = cell_growth_lerp_int(210, 242, t);
                b = cell_growth_lerp_int(222, 74, t);
            }
            else {
                int t = ((warm - 178) * 255) / 77;
                r = cell_growth_lerp_int(170, 250, t);
                g = cell_growth_lerp_int(242, 255, t);
                b = cell_growth_lerp_int(74, 214, t);
            }

            r = bg_r + (r * (fill + membrane + mass / 2)) / 360;
            g = bg_g + (g * (fill + membrane + mass / 2)) / 310;
            b = bg_b + (b * (fill + membrane + mass / 2)) / 340;

            r += glow / 16;
            g += glow / 6;
            b += glow / 5;

            if (cavity > 0) {
                r = (r * (255 - cavity) + bg_r * cavity) / 255;
                g = (g * (255 - cavity) + bg_g * cavity) / 255;
                b = (b * (255 - cavity) + bg_b * cavity) / 255;
            }
            if (core > 0) {
                r += core / 2;
                g += core / 2;
                b += core / 4;
            }

            cell_growth_write_pixel_block(cg, x, y, r, g, b);
        }
    }

    lv_obj_invalidate(cg->root);
}

static void cell_growth_render_bloom_field(astro_cell_growth_ctrl_t *cg, lv_layer_t *layer)
{
    int i;
    int growth;

    if (!cg || !layer)
        return;

    growth = cell_growth_clamp_int((int)cg->frame * 2 + 28, 28, cg->width);
    for (i = 0; i < 92; i++) {
        uint32_t h = cell_growth_hash(0xB100115u + (uint32_t)i * 977u);
        int angle = (int)(h % 360u);
        int dist = 14 + (int)((h >> 9) % 250u);
        int spread_x = (dist * (112 + (int)((h >> 21) & 31u))) / 100;
        int spread_y = (dist * (76 + (int)((h >> 26) & 31u))) / 100;
        int x = cg->bloom_x + (spread_x * cell_growth_cos(angle)) / CELL_GROWTH_TRIGO_MAX;
        int y = cg->bloom_y + (spread_y * cell_growth_sin(angle)) / CELL_GROWTH_TRIGO_MAX;
        int r = 4 + (int)((h >> 15) % 24u);
        int warm;
        int fade;
        int pulse;
        lv_color_t color;

        x += (dist * 34) / 100;
        if ((h & 7u) == 0u)
            r += 12;
        if (dist > growth + (int)((h >> 4) % 48u))
            continue;

        pulse = (int)((cg->frame + (h & 63u)) % 64u);
        if (pulse > 32)
            pulse = 64 - pulse;
        fade = cell_growth_clamp_int(growth - dist + 60, 28, 150);
        warm = cell_growth_clamp_int(214 - dist / 2 + (int)((h >> 13) & 39u), 18, 222);
        color = cell_growth_cell_color(cg, warm);

        cell_growth_draw_circle(layer, x, y, r + 18, color,
            (lv_opa_t)cell_growth_clamp_int(fade / 10, 4, 15), 0, LV_OPA_TRANSP);
        cell_growth_draw_circle(layer, x, y, r + 8, color,
            (lv_opa_t)cell_growth_clamp_int(fade / 5, 9, 30), 0, LV_OPA_TRANSP);
        cell_growth_draw_circle(layer, x, y, r + pulse / 12, color,
            (lv_opa_t)cell_growth_clamp_int(fade / 3, 18, 62), 2,
            (lv_opa_t)cell_growth_clamp_int(fade, 46, 122));
        if (r > 11 && (h & 3u) != 0u) {
            cell_growth_draw_circle(layer, x, y, r - 5, cg->bg_color,
                (lv_opa_t)70, 0, LV_OPA_TRANSP);
        }
        if ((h & 15u) == 0u || warm > 188) {
            cell_growth_draw_circle(layer, x, y, r / 4 + 2,
                warm > 200 ? cg->white_color : color,
                (lv_opa_t)cell_growth_clamp_int(42 + fade / 3, 54, 102),
                0, LV_OPA_TRANSP);
        }
    }
}

static int cell_growth_find_parent(astro_cell_growth_ctrl_t *cg)
{
    int tries;
    int best;
    int best_score;

    if (!cg || cg->cell_count == 0)
        return -1;

    best = 0;
    best_score = -9999;
    for (tries = 0; tries < 18; tries++) {
        int idx = cell_growth_rand_range(cg, 0, (int)cg->cell_count - 1);
        astro_cell_growth_cell_t *cell = &cg->cells[idx];
        int score;

        if (!cell->active)
            continue;
        score = cell->depth * 3 + cell->radius + cell_growth_rand_range(cg, 0, 38);
        if (score > best_score) {
            best = idx;
            best_score = score;
        }
    }
    return best;
}

static void cell_growth_add_cell(astro_cell_growth_ctrl_t *cg, int parent_idx)
{
    astro_cell_growth_cell_t *cell;
    int angle;
    int distance;
    int jitter;

    if (!cg || !cg->cells || cg->cell_count >= cg->max_cells)
        return;

    cell = &cg->cells[cg->cell_count];
    memset(cell, 0, sizeof(*cell));
    cell->active = 1;
    cell->parent = parent_idx;
    cell->age = 0;
    cell->pulse = (uint8_t)cell_growth_rand_range(cg, 0, 255);
    cell->life = cell_growth_rand_range(cg, 520, 1260);
    cell->target_radius = cell_growth_rand_range(cg, 6, 24);
    cell->radius = cell_growth_rand_range(cg, 2, 8);

    if (parent_idx >= 0 && parent_idx < (int)cg->cell_count) {
        astro_cell_growth_cell_t *parent = &cg->cells[parent_idx];
        int outward = cell_growth_rand_range(cg, -18, 42);

        angle = parent->angle + cell_growth_rand_range(cg, -72, 72);
        if ((cg->cell_count % 7u) == 0u)
            angle += cell_growth_rand_range(cg, -135, 135);
        distance = parent->radius + cell->target_radius + cell_growth_rand_range(cg, -8, 12);
        cell->x = parent->x + (distance * cell_growth_cos(angle)) / CELL_GROWTH_TRIGO_MAX;
        cell->y = parent->y + (distance * cell_growth_sin(angle)) / CELL_GROWTH_TRIGO_MAX;
        cell->hue = cell_growth_clamp_int(parent->hue + outward, 0, 224);
        cell->angle = cell_growth_angle_norm(angle);
        cell->depth = parent->depth + 1;
    }
    else {
        angle = cell_growth_rand_range(cg, -70, 92);
        distance = cell_growth_rand_range(cg, 0, cg->width / 18);
        cell->x = cg->bloom_x + (distance * cell_growth_cos(angle)) / CELL_GROWTH_TRIGO_MAX;
        cell->y = cg->bloom_y + (distance * cell_growth_sin(angle)) / CELL_GROWTH_TRIGO_MAX;
        cell->hue = cell_growth_rand_range(cg, 118, 202);
        cell->angle = cell_growth_angle_norm(angle);
        cell->depth = 0;
    }

    if (cell->depth > 3 && cell_growth_rand_range(cg, 0, 100) < 56) {
        cell->target_radius = cell_growth_rand_range(cg, 4, 14);
        cell->hue = cell_growth_clamp_int(cell->hue - cell_growth_rand_range(cg, 32, 88), 0, 170);
    }
    if (cell_growth_rand_range(cg, 0, 100) < 14) {
        cell->target_radius = cell_growth_rand_range(cg, 24, 38);
        cell->hue = cell_growth_rand_range(cg, 5, 112);
    }
    cell->hot = (uint8_t)(cell_growth_rand_range(cg, 0, 100) < 9);

    jitter = cell_growth_rand_range(cg, -10, 10);
    cell->x = cell_growth_clamp_int(cell->x + jitter, -48, cg->width + 56);
    cell->y = cell_growth_clamp_int(cell->y + cell_growth_rand_range(cg, -10, 10), -48, cg->height + 56);
    cg->cell_count++;
}

static void cell_growth_seed(astro_cell_growth_ctrl_t *cg, uint32_t seed)
{
    int i;

    if (!cg || !cg->cells)
        return;

    cg->rng = seed ? seed : 0xB10A115u;
    cg->frame = 0;
    cg->cell_count = 0;
    memset(cg->cells, 0, sizeof(cg->cells[0]) * cg->max_cells);

    cg->bloom_x = (cg->width * 54) / 100;
    cg->bloom_y = (cg->height * 53) / 100;
    for (i = 0; i < 5 && i < (int)cg->max_cells; i++)
        cell_growth_add_cell(cg, -1);
}

static void cell_growth_step_state(astro_cell_growth_ctrl_t *cg)
{
    int i;
    int spawn_count;

    if (!cg || !cg->cells)
        return;

    cg->frame++;
    for (i = 0; i < (int)cg->cell_count; i++) {
        astro_cell_growth_cell_t *cell = &cg->cells[i];
        int pulse;
        int grow_rate;

        if (!cell->active)
            continue;
        cell->age++;
        if (cell->age > cell->life && cg->cell_count > cg->max_cells / 2u)
            cell->active = 0;

        pulse = (int)((cg->frame + cell->pulse) % 64u);
        if (pulse > 32)
            pulse = 64 - pulse;
        grow_rate = (cg->frame < 70u || cell->radius < 7) ? 2 : 1;
        if (cell->radius < cell->target_radius)
            cell->radius += grow_rate;
        else if (pulse == 0 && cell->radius > 4)
            cell->radius--;
    }

    spawn_count = (cg->frame < 70u) ? 3 : ((cg->frame < 180u) ? 3 : 1);
    if ((cg->frame % 2u) == 0u) {
        for (i = 0; i < spawn_count; i++) {
            int parent_idx = cell_growth_find_parent(cg);
            if (parent_idx >= 0)
                cell_growth_add_cell(cg, parent_idx);
        }
    }

    if (cg->cell_count >= cg->max_cells && (cg->frame % 180u) == 0u)
        cell_growth_seed(cg, cg->rng ^ (cg->frame * 2654435761u));
}

static void cell_growth_render_connections(astro_cell_growth_ctrl_t *cg, lv_layer_t *layer)
{
    int i;

    if (!cg || !layer)
        return;

    for (i = 0; i < (int)cg->cell_count; i++) {
        astro_cell_growth_cell_t *cell = &cg->cells[i];
        astro_cell_growth_cell_t *parent;
        lv_color_t color;
        int width;

        if (!cell->active || cell->parent < 0 || cell->parent >= (int)cg->cell_count)
            continue;
        parent = &cg->cells[cell->parent];
        if (!parent->active || cell_growth_distance2(cell, parent) > 112 * 112)
            continue;

        color = cell_growth_cell_color(cg, (cell->hue + parent->hue) / 2);
        width = cell_growth_clamp_int((cell->radius + parent->radius) / 2, 10, 40);
        cell_growth_draw_line(layer, parent->x, parent->y, cell->x, cell->y,
            color, width + 18, (lv_opa_t)14);
        cell_growth_draw_line(layer, parent->x, parent->y, cell->x, cell->y,
            color, width + 5, (lv_opa_t)24);
    }

    for (i = 0; i < (int)cg->cell_count; i++) {
        astro_cell_growth_cell_t *cell = &cg->cells[i];
        astro_cell_growth_cell_t *parent;
        int d2;
        lv_color_t color;
        int width;

        if (!cell->active || cell->parent < 0 || cell->parent >= (int)cg->cell_count)
            continue;
        parent = &cg->cells[cell->parent];
        if (!parent->active)
            continue;
        d2 = cell_growth_distance2(cell, parent);
        if (d2 > 92 * 92)
            continue;

        color = cell_growth_cell_color(cg, (cell->hue + parent->hue) / 2);
        width = cell_growth_clamp_int((cell->radius + parent->radius) / 5, 2, 10);
        cell_growth_draw_line(layer, parent->x, parent->y, cell->x, cell->y,
            color, width + 8, (lv_opa_t)30);
        cell_growth_draw_line(layer, parent->x, parent->y, cell->x, cell->y,
            color, width + 2, (lv_opa_t)70);
    }
}

static void cell_growth_render_cells(astro_cell_growth_ctrl_t *cg, lv_layer_t *layer)
{
    int i;

    if (!cg || !layer)
        return;

    for (i = 0; i < (int)cg->cell_count; i++) {
        astro_cell_growth_cell_t *cell = &cg->cells[i];
        lv_color_t color;
        int radius;
        int pulse;
        int ring_r;
        int bright;
        lv_opa_t membrane_opa;

        if (!cell->active)
            continue;

        pulse = (int)((cg->frame + cell->pulse) % 72u);
        if (pulse > 36)
            pulse = 72 - pulse;
        radius = cell_growth_clamp_int(cell->radius + pulse / 18, 2, 46);
        color = cell_growth_cell_color(cg, cell->hue);
        bright = cell_growth_clamp_int(cell->hue + (int)cell->hot * 70, 0, 255);
        membrane_opa = (lv_opa_t)cell_growth_clamp_int(74 + bright / 4, 78, 132);

        cell_growth_draw_circle(layer, cell->x, cell->y, radius + 22, color,
            (lv_opa_t)10, 0, LV_OPA_TRANSP);
        cell_growth_draw_circle(layer, cell->x, cell->y, radius + 11, color,
            (lv_opa_t)20, 0, LV_OPA_TRANSP);
        cell_growth_draw_circle(layer, cell->x, cell->y, radius + 2, color,
            (lv_opa_t)(cell->hue > 150 ? 52 : 34), 0, LV_OPA_TRANSP);

        ring_r = radius + 4 + (pulse / 8);
        cell_growth_draw_circle(layer, cell->x, cell->y, ring_r, color,
            LV_OPA_TRANSP, cell->hue > 182 ? 3 : 2, membrane_opa);

        if (!cell->hot && radius > 11) {
            cell_growth_draw_circle(layer, cell->x, cell->y,
                cell_growth_clamp_int(radius - 7, 3, radius - 2),
                cg->bg_color, (lv_opa_t)88, 0, LV_OPA_TRANSP);
        }

        if (cell->hot || (cell->hue > 190 && (i % 6) == 0)) {
            cell_growth_draw_circle(layer, cell->x, cell->y, radius / 2 + 4,
                cg->white_color, (lv_opa_t)70, 0, LV_OPA_TRANSP);
            cell_growth_draw_circle(layer, cell->x, cell->y, radius / 4 + 2,
                cg->white_color, (lv_opa_t)cell_growth_clamp_int(104 + pulse * 2, 104, 166),
                0, LV_OPA_TRANSP);
        }
        else {
            cell_growth_draw_circle(layer, cell->x, cell->y, radius / 4 + 1,
                color, (lv_opa_t)cell_growth_clamp_int(34 + cell->hue / 6, 34, 72),
                0, LV_OPA_TRANSP);
        }
    }
}

static void cell_growth_render(astro_cell_growth_ctrl_t *cg)
{
    lv_layer_t layer;
    int i;

    if (!cg || !cg->root || !cg->buf)
        return;

    cell_growth_render_field_pixels(cg);
    return;

    lv_canvas_fill_bg(cg->root, cg->bg_color, LV_OPA_COVER);
    lv_canvas_init_layer(cg->root, &layer);

    cell_growth_draw_circle(&layer, (cg->width * 70) / 100, (cg->height * 58) / 100,
        cg->width / 4, cg->green_color, (lv_opa_t)10, 0, LV_OPA_TRANSP);
    cell_growth_draw_circle(&layer, (cg->width * 82) / 100, (cg->height * 48) / 100,
        cg->width / 3, cg->teal_color, (lv_opa_t)8, 0, LV_OPA_TRANSP);

    for (i = 0; i < 46; i++) {
        int drift = (int)(cg->frame / 3u);
        int x = (i * 67 + drift) % (cg->width + 170) - 28;
        int y = (i * 41 + (int)(cg->frame / 5u)) % (cg->height + 120) - 48;
        int r = 5 + ((i * 11) % 30);
        lv_color_t color = (i & 1) ? cg->teal_color : cg->green_color;
        lv_opa_t opa = (lv_opa_t)(7 + (i % 4) * 4);

        if (x < cg->width / 4 && (i % 4) != 0)
            continue;
        cell_growth_draw_circle(&layer, x, y, r + 6, color, (lv_opa_t)(opa / 2),
            0, LV_OPA_TRANSP);
        cell_growth_draw_circle(&layer, x, y, r, cg->bg_color, (lv_opa_t)54,
            1, (lv_opa_t)(opa * 5));
        if ((i % 5) == 0) {
            cell_growth_draw_circle(&layer, x, y, r / 3 + 1, color,
                (lv_opa_t)(opa * 4), 0, LV_OPA_TRANSP);
        }
    }

    cell_growth_render_bloom_field(cg, &layer);
    cell_growth_render_connections(cg, &layer);
    cell_growth_render_cells(cg, &layer);
    lv_canvas_finish_layer(cg->root, &layer);
}

static void cell_growth_timer_cb(lv_timer_t *timer)
{
    astro_cell_growth_ctrl_t *cg;

    if (!timer)
        return;
    cg = (astro_cell_growth_ctrl_t *)lv_timer_get_user_data(timer);
    if (!cg || cg->paused)
        return;
    cell_growth_step_state(cg);
    cell_growth_render(cg);
}

static void cell_growth_free(astro_cell_growth_ctrl_t *cg)
{
    if (!cg)
        return;
    if (cg->timer) {
        lv_timer_del(cg->timer);
        cg->timer = NULL;
    }
    if (cg->buf) {
        free(cg->buf);
        cg->buf = NULL;
        cg->buf_size = 0;
    }
    if (cg->cells) {
        free(cg->cells);
        cg->cells = NULL;
    }
    free(cg);
}

static void cell_growth_event_cb(lv_event_t *e)
{
    lv_obj_t *obj;
    astro_cell_growth_ctrl_t *cg;

    if (!e || lv_event_get_code(e) != LV_EVENT_DELETE)
        return;
    obj = lv_event_get_target(e);
    cg = cell_growth_get(obj);
    if (!cg)
        return;
    lv_obj_set_user_data(obj, NULL);
    cell_growth_free(cg);
}

lv_obj_t *divoom_astro_cell_growth_create(lv_obj_t *parent,
    const divoom_astro_cell_growth_cfg_t *cfg)
{
    lv_obj_t *root;
    astro_cell_growth_ctrl_t *cg;
    uint32_t seed;

    if (!parent)
        return NULL;

    root = lv_canvas_create(parent);
    if (!root)
        return NULL;
    cell_growth_style_plain(root);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_opa(root, LV_OPA_TRANSP, 0);

    cg = (astro_cell_growth_ctrl_t *)calloc(1, sizeof(*cg));
    if (!cg) {
        lv_obj_del(root);
        return NULL;
    }

    cg->root = root;
    cg->width = cfg && cfg->width ? cfg->width : DIVOOM_ASTRO_CELL_GROWTH_DEFAULT_W;
    cg->height = cfg && cfg->height ? cfg->height : DIVOOM_ASTRO_CELL_GROWTH_DEFAULT_H;
    cg->width = cell_growth_clamp_int(cg->width, CELL_GROWTH_MIN_SIZE, CELL_GROWTH_MAX_SIZE);
    cg->height = cell_growth_clamp_int(cg->height, CELL_GROWTH_MIN_SIZE, CELL_GROWTH_MAX_SIZE);
    cg->refresh_ms = cfg && cfg->refresh_ms ? cfg->refresh_ms : CELL_GROWTH_DEFAULT_REFRESH_MS;
    cg->max_cells = cfg && cfg->max_cells ? cfg->max_cells : DIVOOM_ASTRO_CELL_GROWTH_DEFAULT_MAX_CELLS;
    cg->max_cells = (uint16_t)cell_growth_clamp_int(cg->max_cells, 8, CELL_GROWTH_MAX_CELLS);
    cg->bg_color = cfg ? cfg->bg_color : lv_color_hex(0x02070B);
    cg->teal_color = cfg ? cfg->teal_color : lv_color_hex(0x14D9E6);
    cg->green_color = cfg ? cfg->green_color : lv_color_hex(0x48E277);
    cg->yellow_color = cfg ? cfg->yellow_color : lv_color_hex(0xD7F43C);
    cg->white_color = cfg ? cfg->white_color : lv_color_hex(0xF6FFDB);

    cg->buf_size = (size_t)LV_CANVAS_BUF_SIZE_TRUE_COLOR_ALPHA(cg->width, cg->height);
    cg->buf = malloc(cg->buf_size);
    cg->cells = (astro_cell_growth_cell_t *)calloc(cg->max_cells, sizeof(cg->cells[0]));
    if (!cg->buf || !cg->cells) {
        cell_growth_free(cg);
        lv_obj_del(root);
        return NULL;
    }

    lv_obj_set_user_data(root, cg);
    lv_obj_add_event(root, cell_growth_event_cb, LV_EVENT_DELETE, NULL);
    lv_obj_set_size(root, cg->width, cg->height);
    lv_canvas_set_buffer(root, cg->buf, cg->width, cg->height, LV_COLOR_FORMAT_ARGB8888);

    seed = cfg && cfg->seed ? cfg->seed : 0x115106u;
    cell_growth_seed(cg, seed);
    cell_growth_render(cg);

    cg->timer = lv_timer_create(cell_growth_timer_cb, cg->refresh_ms, cg);
    if (!cg->timer) {
        lv_obj_del(root);
        return NULL;
    }

    return root;
}

void divoom_astro_cell_growth_reset(lv_obj_t *obj, uint32_t seed)
{
    astro_cell_growth_ctrl_t *cg = cell_growth_get(obj);

    if (!cg)
        return;
    cell_growth_seed(cg, seed);
    cell_growth_render(cg);
}

void divoom_astro_cell_growth_set_palette(lv_obj_t *obj,
    lv_color_t bg,
    lv_color_t teal,
    lv_color_t green,
    lv_color_t yellow,
    lv_color_t white)
{
    astro_cell_growth_ctrl_t *cg = cell_growth_get(obj);

    if (!cg)
        return;
    cg->bg_color = bg;
    cg->teal_color = teal;
    cg->green_color = green;
    cg->yellow_color = yellow;
    cg->white_color = white;
    cell_growth_render(cg);
}

void divoom_astro_cell_growth_set_paused(lv_obj_t *obj, int paused)
{
    astro_cell_growth_ctrl_t *cg = cell_growth_get(obj);

    if (!cg)
        return;
    cg->paused = paused ? 1 : 0;
}

void divoom_astro_cell_growth_step(lv_obj_t *obj)
{
    astro_cell_growth_ctrl_t *cg = cell_growth_get(obj);

    if (!cg)
        return;
    cell_growth_step_state(cg);
    cell_growth_render(cg);
}

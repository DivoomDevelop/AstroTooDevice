#include "divoom.h"
#include "lvgl.h"
#include "divoom_light_app.h"
#include "divoom_menu_control.h"
#include "divoom_lvgl_mixer.h"

#define DIVOOM_LVGL_MIXER_GRID_SIZE       13
#define DIVOOM_LVGL_MIXER_CELL_COUNT      (DIVOOM_LVGL_MIXER_GRID_SIZE * DIVOOM_LVGL_MIXER_GRID_SIZE)
#define DIVOOM_LVGL_MIXER_CELL_SIZE       34
#define DIVOOM_LVGL_MIXER_CELL_STEP       36
#define DIVOOM_LVGL_MIXER_GRID_ORIGIN     7
#define DIVOOM_LVGL_MIXER_FRAME_TIME_MS   100
#define DIVOOM_LVGL_MIXER_PERIMETER_COUNT 48
#define DIVOOM_LVGL_MIXER_CENTER_PHASES   9
#define DIVOOM_LVGL_MIXER_UI_DIR          "132/"
#define DIVOOM_LVGL_MIXER_IDLE_IMAGE      DIVOOM_LVGL_MIXER_UI_DIR "mixer_cell_idle.png"

typedef enum
{
    DIVOOM_LVGL_MIXER_EFFECT_WAVE,
    DIVOOM_LVGL_MIXER_EFFECT_ROTATE,
    DIVOOM_LVGL_MIXER_EFFECT_EDGE_6,
    DIVOOM_LVGL_MIXER_EFFECT_ROWS,
    DIVOOM_LVGL_MIXER_EFFECT_EDGE_10,
    DIVOOM_LVGL_MIXER_EFFECT_DIAGONAL,
    DIVOOM_LVGL_MIXER_EFFECT_CENTER,
    DIVOOM_LVGL_MIXER_EFFECT_DUAL,
    DIVOOM_LVGL_MIXER_EFFECT_DUAL_OPPOSED,
}DIVOOM_LVGL_MIXER_EFFECT;

typedef struct _divoom_lvgl_mixer_handle_
{
    lv_obj_t *screen;
    lv_obj_t *cell[DIVOOM_LVGL_MIXER_CELL_COUNT];
    lv_obj_t *active_cell[DIVOOM_LVGL_MIXER_CELL_COUNT];
    lv_timer_t *timer;
    lv_style_t screen_style;
    divoom_light_app_mixer_state state;
    uint32 frame;
    uint8 loaded_color;
}divoom_lvgl_mixer_handle, *Pdivoom_lvgl_mixer_handle;

static const char *divoom_lvgl_mixer_active_image[DIVOOM_LIGHT_APP_MIXER_COLOR_COUNT] =
{
    DIVOOM_LVGL_MIXER_UI_DIR "mixer_cell_red.png",
    DIVOOM_LVGL_MIXER_UI_DIR "mixer_cell_purple.png",
    DIVOOM_LVGL_MIXER_UI_DIR "mixer_cell_green.png",
    DIVOOM_LVGL_MIXER_UI_DIR "mixer_cell_orange.png",
    DIVOOM_LVGL_MIXER_UI_DIR "mixer_cell_cyan.png",
    DIVOOM_LVGL_MIXER_UI_DIR "mixer_cell_blue.png"
};

static void divoom_lvgl_mixer_set_point(uint8 *opacity, int16 x, int16 y, uint8 value)
{
    uint16 index;

    if(x < 0 || x >= DIVOOM_LVGL_MIXER_GRID_SIZE ||
        y < 0 || y >= DIVOOM_LVGL_MIXER_GRID_SIZE)
    {
        return;
    }
    index = (uint16)(y * DIVOOM_LVGL_MIXER_GRID_SIZE + x);
    if(opacity[index] < value)
    {
        opacity[index] = value;
    }
}

static int16 divoom_lvgl_mixer_abs(int16 value)
{
    if(value < 0)
    {
        return (int16)-value;
    }
    return value;
}

static void divoom_lvgl_mixer_draw_line(uint8 *opacity, int16 x0, int16 y0,
    int16 x1, int16 y1, uint8 value)
{
    int16 dx = divoom_lvgl_mixer_abs((int16)(x1 - x0));
    int16 sx = x0 < x1 ? 1 : -1;
    int16 dy = (int16)-divoom_lvgl_mixer_abs((int16)(y1 - y0));
    int16 sy = y0 < y1 ? 1 : -1;
    int16 error = (int16)(dx + dy);

    while(1)
    {
        int16 error2;

        divoom_lvgl_mixer_set_point(opacity, x0, y0, value);
        if(x0 == x1 && y0 == y1)
        {
            break;
        }
        error2 = (int16)(error * 2);
        if(error2 >= dy)
        {
            error = (int16)(error + dy);
            x0 = (int16)(x0 + sx);
        }
        if(error2 <= dx)
        {
            error = (int16)(error + dx);
            y0 = (int16)(y0 + sy);
        }
    }
}

static void divoom_lvgl_mixer_perimeter_point(int16 position, int16 *x, int16 *y)
{
    position %= DIVOOM_LVGL_MIXER_PERIMETER_COUNT;
    if(position < 0)
    {
        position = (int16)(position + DIVOOM_LVGL_MIXER_PERIMETER_COUNT);
    }

    if(position < 13)
    {
        *x = position;
        *y = 0;
    }
    else if(position < 25)
    {
        *x = 12;
        *y = (int16)(position - 12);
    }
    else if(position < 37)
    {
        *x = (int16)(36 - position);
        *y = 12;
    }
    else
    {
        *x = 0;
        *y = (int16)(48 - position);
    }
}

static void divoom_lvgl_mixer_render_wave(Pdivoom_lvgl_mixer_handle handle, uint8 *opacity)
{
    static const uint8 wave_y[6] = {0, 1, 2, 3, 2, 1};
    int16 x;

    for(x = 0; x < DIVOOM_LVGL_MIXER_GRID_SIZE; x ++)
    {
        int16 y = wave_y[(x + handle->frame) % 6];

        while(y < DIVOOM_LVGL_MIXER_GRID_SIZE)
        {
            divoom_lvgl_mixer_set_point(opacity, x, y, 255);
            y = (int16)(y + 6);
        }
    }
}

static void divoom_lvgl_mixer_render_rotate(Pdivoom_lvgl_mixer_handle handle, uint8 *opacity)
{
    static const int8 end_point[12][2] =
    {
        {0, -6}, {2, -6}, {3, -5}, {4, -4}, {5, -3}, {6, -2},
        {6, 0}, {6, 2}, {5, 3}, {4, 4}, {3, 5}, {2, 6}
    };
    uint8 direction = (uint8)(handle->frame % 12);
    int16 dx = end_point[direction][0];
    int16 dy = end_point[direction][1];

    divoom_lvgl_mixer_draw_line(opacity, (int16)(6 - dx), (int16)(6 - dy),
        (int16)(6 + dx), (int16)(6 + dy), 255);
}

static void divoom_lvgl_mixer_render_edge(Pdivoom_lvgl_mixer_handle handle,
    uint8 *opacity, uint8 tail_count, uint8 speed)
{
    static const uint8 tail_opacity[5] = {255, 204, 153, 102, 64};
    int16 head = (int16)((handle->frame * speed) % DIVOOM_LVGL_MIXER_PERIMETER_COUNT);
    uint8 group;

    for(group = 0; group < 2; group ++)
    {
        uint8 tail;
        int16 group_head = (int16)(head + group * (DIVOOM_LVGL_MIXER_PERIMETER_COUNT / 2));

        for(tail = 0; tail < tail_count; tail ++)
        {
            int16 x;
            int16 y;

            divoom_lvgl_mixer_perimeter_point((int16)(group_head - tail), &x, &y);
            divoom_lvgl_mixer_set_point(opacity, x, y, tail_opacity[tail]);
        }
    }
}

static void divoom_lvgl_mixer_render_rows(Pdivoom_lvgl_mixer_handle handle, uint8 *opacity)
{
    static const uint8 row_opacity[4] = {255, 191, 128, 64};
    int16 head = (int16)(12 - (handle->frame % 17));
    uint8 tail;

    for(tail = 0; tail < 4; tail ++)
    {
        int16 row = (int16)(head + tail);
        int16 x;

        for(x = 0; x < DIVOOM_LVGL_MIXER_GRID_SIZE; x ++)
        {
            divoom_lvgl_mixer_set_point(opacity, x, row, row_opacity[tail]);
        }
    }
}

static void divoom_lvgl_mixer_render_diagonal(Pdivoom_lvgl_mixer_handle handle, uint8 *opacity)
{
    static const uint8 diagonal_opacity[4] = {230, 179, 128, 77};
    int16 offset = (int16)((handle->frame % 21) - 8);
    int16 y;

    for(y = 0; y < DIVOOM_LVGL_MIXER_GRID_SIZE; y ++)
    {
        uint8 group;

        for(group = 0; group < 4; group ++)
        {
            int16 x = (int16)(y + offset + group * 4);

            divoom_lvgl_mixer_set_point(opacity, x, y, diagonal_opacity[group]);
        }
    }
}

static void divoom_lvgl_mixer_render_center(Pdivoom_lvgl_mixer_handle handle, uint8 *opacity)
{
    uint8 phase = (uint8)((handle->frame / 2) % DIVOOM_LVGL_MIXER_CENTER_PHASES);
    int16 x;
    int16 y;

    if(phase > 7)
    {
        /* FlowToo 在全部格子移出后保留一帧空场，下一帧从中心重新开始。 */
        return;
    }

    for(y = 0; y < DIVOOM_LVGL_MIXER_GRID_SIZE; y ++)
    {
        for(x = 0; x < DIVOOM_LVGL_MIXER_GRID_SIZE; x ++)
        {
            int16 dx = divoom_lvgl_mixer_abs((int16)(x - 6));
            int16 dy = divoom_lvgl_mixer_abs((int16)(y - 6));
            int16 radius = dx > dy ? dx : dy;

            if(radius == phase)
            {
                divoom_lvgl_mixer_set_point(opacity, x, y, 255);
            }
            else if(phase > 0 && radius == (int16)(phase - 1))
            {
                divoom_lvgl_mixer_set_point(opacity, x, y, 102);
            }
        }
    }
}

static void divoom_lvgl_mixer_render_dual(Pdivoom_lvgl_mixer_handle handle, uint8 *opacity)
{
    static const uint8 trail_opacity[3] = {255, 153, 77};
    int16 head = (int16)((handle->frame % 19) - 3);
    uint8 trail;

    /* Mode 7 保留原效果：四个角的对角线灯点同时向中心移动并带两级尾迹。 */
    for(trail = 0; trail < 3; trail ++)
    {
        int16 position = (int16)(head - trail);

        divoom_lvgl_mixer_set_point(opacity, position, position, trail_opacity[trail]);
        divoom_lvgl_mixer_set_point(opacity, (int16)(12 - position), position,
            trail_opacity[trail]);
        divoom_lvgl_mixer_set_point(opacity, position, (int16)(12 - position),
            trail_opacity[trail]);
        divoom_lvgl_mixer_set_point(opacity, (int16)(12 - position),
            (int16)(12 - position), trail_opacity[trail]);
    }
}

static void divoom_lvgl_mixer_render_dual_opposed(Pdivoom_lvgl_mixer_handle handle,
    uint8 *opacity)
{
    static const uint8 frame_count[6] = {4, 8, 12, 12, 8, 4};
    static const uint8 column[6][12] =
    {
        {0, 1, 11, 10},
        {0, 1, 2, 3, 11, 10, 9, 8},
        {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11},
        {2, 3, 4, 5, 6, 7, 4, 5, 6, 7, 8, 9},
        {4, 5, 6, 7, 4, 5, 6, 7},
        {6, 7, 4, 5}
    };
    static const uint8 row[6][12] =
    {
        {0, 0, 3, 3},
        {0, 0, 1, 1, 2, 2, 3, 3},
        {0, 0, 1, 1, 2, 2, 1, 1, 2, 2, 3, 3},
        {1, 1, 2, 2, 3, 3, 0, 0, 1, 1, 2, 2},
        {2, 2, 3, 3, 0, 0, 1, 1},
        {3, 3, 0, 0}
    };
    static const uint8 opacity_code[6][12] =
    {
        {1, 1, 1, 1},
        {2, 2, 1, 1, 1, 1, 2, 2},
        {3, 3, 2, 2, 1, 1, 1, 1, 2, 2, 3, 3},
        {3, 3, 2, 2, 1, 1, 1, 1, 2, 2, 3, 3},
        {3, 3, 2, 2, 2, 2, 3, 3},
        {3, 3, 3, 3}
    };
    static const uint8 opacity_value[4] = {0, 255, 128, 64};
    uint8 phase = (uint8)(handle->frame % 6);
    uint8 index;

    /* 复用 FlowToo 六帧表，将 12×4 坐标等比映射到 13×13：左右两组汇聚并穿过中心。 */
    for(index = 0; index < frame_count[phase]; index ++)
    {
        int16 x = (int16)((column[phase][index] * 12 + 5) / 11);
        int16 y = (int16)(row[phase][index] * 4);

        divoom_lvgl_mixer_set_point(opacity, x, y,
            opacity_value[opacity_code[phase][index]]);
    }
}

static void divoom_lvgl_mixer_apply_color(Pdivoom_lvgl_mixer_handle handle)
{
    uint8 color = handle->state.color % DIVOOM_LIGHT_APP_MIXER_COLOR_COUNT;
    uint16 index;

    if(handle->loaded_color == color)
    {
        return;
    }

    /* FlowToo 激活格 PNG 自带深色外沿、高光和中央圆角内圈，仅在颜色变化时换图。 */
    for(index = 0; index < DIVOOM_LVGL_MIXER_CELL_COUNT; index ++)
    {
        divoom_menu_control_lv_img_set_src(handle->active_cell[index],
            divoom_lvgl_mixer_active_image[color]);
    }
    handle->loaded_color = color;
}

static void divoom_lvgl_mixer_apply_cells(Pdivoom_lvgl_mixer_handle handle, uint8 *opacity)
{
    uint16 index;

    divoom_lvgl_mixer_apply_color(handle);
    for(index = 0; index < DIVOOM_LVGL_MIXER_CELL_COUNT; index ++)
    {
        if(opacity[index] == 0)
        {
            lv_obj_add_flag(handle->active_cell[index], LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_set_style_img_opa(handle->active_cell[index], opacity[index], 0);
            lv_obj_clear_flag(handle->active_cell[index], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void divoom_lvgl_mixer_render(Pdivoom_lvgl_mixer_handle handle)
{
    uint8 opacity[DIVOOM_LVGL_MIXER_CELL_COUNT] = {0};

    if(handle->state.running)
    {
        switch(handle->state.effect % DIVOOM_LIGHT_APP_MIXER_EFFECT_COUNT)
        {
            case DIVOOM_LVGL_MIXER_EFFECT_WAVE:
            {
                divoom_lvgl_mixer_render_wave(handle, opacity);
                break;
            }
            case DIVOOM_LVGL_MIXER_EFFECT_ROTATE:
            {
                divoom_lvgl_mixer_render_rotate(handle, opacity);
                break;
            }
            case DIVOOM_LVGL_MIXER_EFFECT_EDGE_6:
            {
                divoom_lvgl_mixer_render_edge(handle, opacity, 3, 2);
                break;
            }
            case DIVOOM_LVGL_MIXER_EFFECT_ROWS:
            {
                divoom_lvgl_mixer_render_rows(handle, opacity);
                break;
            }
            case DIVOOM_LVGL_MIXER_EFFECT_EDGE_10:
            {
                divoom_lvgl_mixer_render_edge(handle, opacity, 5, 1);
                break;
            }
            case DIVOOM_LVGL_MIXER_EFFECT_DIAGONAL:
            {
                divoom_lvgl_mixer_render_diagonal(handle, opacity);
                break;
            }
            case DIVOOM_LVGL_MIXER_EFFECT_CENTER:
            {
                divoom_lvgl_mixer_render_center(handle, opacity);
                break;
            }
            case DIVOOM_LVGL_MIXER_EFFECT_DUAL:
            {
                divoom_lvgl_mixer_render_dual(handle, opacity);
                break;
            }
            case DIVOOM_LVGL_MIXER_EFFECT_DUAL_OPPOSED:
            default:
            {
                divoom_lvgl_mixer_render_dual_opposed(handle, opacity);
                break;
            }
        }
    }
    divoom_lvgl_mixer_apply_cells(handle, opacity);
}

static void divoom_lvgl_mixer_timer_cb(lv_timer_t *timer)
{
    Pdivoom_lvgl_mixer_handle handle = (Pdivoom_lvgl_mixer_handle)lv_timer_get_user_data(timer);
    divoom_light_app_mixer_state state;

    if(handle == NULL)
    {
        return;
    }
    divoom_light_app_get_mixer_state(&state);
    if(state.generation != handle->state.generation ||
        state.effect != handle->state.effect || state.color != handle->state.color ||
        state.running != handle->state.running)
    {
        handle->state = state;
        handle->frame = 0;
    }

    divoom_lvgl_mixer_render(handle);
    if(handle->state.running)
    {
        handle->frame ++;
    }
}

static void divoom_lvgl_mixer_init_styles(Pdivoom_lvgl_mixer_handle handle)
{
    lv_style_init(&handle->screen_style);
    lv_style_set_bg_color(&handle->screen_style, lv_color_hex(0x000000));
    lv_style_set_bg_opa(&handle->screen_style, LV_OPA_COVER);
    lv_style_set_border_width(&handle->screen_style, 0);
    lv_style_set_pad_all(&handle->screen_style, 0);
    lv_style_set_radius(&handle->screen_style, 0);
}

void *divoom_lvgl_mixer_init(void *user_data, int menu_id)
{
    Pdivoom_lvgl_mixer_handle handle;
    uint16 index;

    (void)user_data;
    (void)menu_id;
    handle = (Pdivoom_lvgl_mixer_handle)divoom_system_malloc(sizeof(divoom_lvgl_mixer_handle));
    if(handle == NULL)
    {
        return NULL;
    }

    divoom_lvgl_mixer_init_styles(handle);
    handle->loaded_color = 0xff;
    handle->screen = lv_obj_create(NULL);
    lv_obj_add_style(handle->screen, &handle->screen_style, 0);
    lv_obj_set_size(handle->screen, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    lv_obj_clear_flag(handle->screen, LV_OBJ_FLAG_SCROLLABLE);

    for(index = 0; index < DIVOOM_LVGL_MIXER_CELL_COUNT; index ++)
    {
        uint8 row = (uint8)(index / DIVOOM_LVGL_MIXER_GRID_SIZE);
        uint8 column = (uint8)(index % DIVOOM_LVGL_MIXER_GRID_SIZE);

        handle->cell[index] = lv_img_create(handle->screen);
        lv_obj_set_size(handle->cell[index], DIVOOM_LVGL_MIXER_CELL_SIZE,
            DIVOOM_LVGL_MIXER_CELL_SIZE);
        lv_obj_set_pos(handle->cell[index],
            DIVOOM_LVGL_MIXER_GRID_ORIGIN + column * DIVOOM_LVGL_MIXER_CELL_STEP,
            DIVOOM_LVGL_MIXER_GRID_ORIGIN + row * DIVOOM_LVGL_MIXER_CELL_STEP);
        divoom_menu_control_lv_img_set_src(handle->cell[index],
            DIVOOM_LVGL_MIXER_IDLE_IMAGE);
        /* MasterGo Fx07 在黑底上以 40% 显示，保留内圈但避免暗格抢过激活图案。 */
        lv_obj_set_style_img_opa(handle->cell[index], LV_OPA_40, 0);
        lv_obj_clear_flag(handle->cell[index], LV_OBJ_FLAG_SCROLLABLE);

        handle->active_cell[index] = lv_img_create(handle->screen);
        lv_obj_set_size(handle->active_cell[index], DIVOOM_LVGL_MIXER_CELL_SIZE,
            DIVOOM_LVGL_MIXER_CELL_SIZE);
        lv_obj_set_pos(handle->active_cell[index],
            DIVOOM_LVGL_MIXER_GRID_ORIGIN + column * DIVOOM_LVGL_MIXER_CELL_STEP,
            DIVOOM_LVGL_MIXER_GRID_ORIGIN + row * DIVOOM_LVGL_MIXER_CELL_STEP);
        lv_obj_clear_flag(handle->active_cell[index], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(handle->active_cell[index], LV_OBJ_FLAG_HIDDEN);
    }

    handle->timer = lv_timer_create(divoom_lvgl_mixer_timer_cb,
        DIVOOM_LVGL_MIXER_FRAME_TIME_MS, handle);
    if(handle->timer != NULL)
    {
        lv_timer_resume(handle->timer);
    }
    return handle;
}

void divoom_lvgl_mixer_exit(void *handle_data)
{
    Pdivoom_lvgl_mixer_handle handle = (Pdivoom_lvgl_mixer_handle)handle_data;

    if(handle == NULL)
    {
        return;
    }
    if(handle->timer != NULL)
    {
        lv_timer_del(handle->timer);
        handle->timer = NULL;
    }
    if(handle->screen != NULL)
    {
        lv_obj_del(handle->screen);
        handle->screen = NULL;
    }
    lv_style_reset(&handle->screen_style);
    divoom_system_free(handle);
}

void divoom_lvgl_mixer_disp(void *handle_data, void *active_obj)
{
    Pdivoom_lvgl_mixer_handle handle = (Pdivoom_lvgl_mixer_handle)handle_data;

    (void)active_obj;
    if(handle == NULL || handle->screen == NULL)
    {
        return;
    }
    lv_scr_load(handle->screen);
    if(handle->timer != NULL)
    {
        lv_timer_resume(handle->timer);
    }
}

void divoom_lvgl_mixer_pause(void *handle_data)
{
    Pdivoom_lvgl_mixer_handle handle = (Pdivoom_lvgl_mixer_handle)handle_data;

    if(handle != NULL && handle->timer != NULL)
    {
        lv_timer_pause(handle->timer);
    }
}

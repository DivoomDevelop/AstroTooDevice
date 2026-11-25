#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_lvgl_top_bright_volume_video.h"
#include "divoom_lvgl_top_control.h"

#if defined(DIVOOM_PRODUCT_ASTRO_TOO)

#define TBV_VIDEO_OSD_W              316
#define TBV_VIDEO_OSD_H              40
#define TBV_VIDEO_OSD_Y              32
#define TBV_VIDEO_OSD_X              ((DIVOOM_SCREEN_WIDTH - TBV_VIDEO_OSD_W) / 2)
#define TBV_VIDEO_OSD_RADIUS         12
#define TBV_VIDEO_OSD_BORDER         2
#define TBV_VIDEO_OSD_INNER_W        (TBV_VIDEO_OSD_W - TBV_VIDEO_OSD_BORDER * 2)
#define TBV_VIDEO_OSD_INNER_H        (TBV_VIDEO_OSD_H - TBV_VIDEO_OSD_BORDER * 2)
#define TBV_VIDEO_OSD_ICON           24
#define TBV_VIDEO_OSD_ICON_X         8
#define TBV_VIDEO_OSD_DISMISS_TICKS  11

#if !defined(DIVOOM_SIMULATOR) || !(DIVOOM_SIMULATOR)
extern void *sunxifb_alloc(size_t size, char *label);
extern void sunxifb_free(void **data, char *label);
extern void sunxifb_flush_cache(void *data, size_t size);
extern void *sunxifb_mem_get_phyaddr(void *data);
#endif

typedef struct
{
    lv_obj_t *main_obj;
    lv_obj_t *fill_obj;
    lv_obj_t *volume_icon;
    lv_obj_t *brightness_icon;
    lv_timer_t *exit_timer;
    lv_style_t bar_style;
    lv_style_t fill_style;
    uint8 styles_initialized;
    uint8 osd_enabled;
    uint8 osd_front_index;
    uint8 brightness_mode;
    int fill_percent;
    int exit_time;
    void *osd_buf[2];
    uint32 osd_buf_size;
} divoom_tbv_video_ctrl;

static divoom_tbv_video_ctrl g_tbv_video;

static lv_obj_t *divoom_tbv_video_create_solid(lv_obj_t *parent,
    int x, int y, int w, int h, int radius)
{
    lv_obj_t *obj;

    obj = lv_obj_create(parent);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, w, h);
    lv_obj_set_style_bg_color(obj, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_radius(obj, radius, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    return obj;
}

static lv_obj_t *divoom_tbv_video_create_icon_root(lv_obj_t *parent)
{
    lv_obj_t *obj;

    obj = lv_obj_create(parent);
    lv_obj_set_pos(obj, TBV_VIDEO_OSD_ICON_X,
        (TBV_VIDEO_OSD_INNER_H - TBV_VIDEO_OSD_ICON) / 2);
    lv_obj_set_size(obj, TBV_VIDEO_OSD_ICON, TBV_VIDEO_OSD_ICON);
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    return obj;
}

static void divoom_tbv_video_create_icons(divoom_tbv_video_ctrl *ctrl)
{
    ctrl->volume_icon = divoom_tbv_video_create_icon_root(ctrl->main_obj);
    divoom_tbv_video_create_solid(ctrl->volume_icon, 2, 9, 5, 7, 1);
    divoom_tbv_video_create_solid(ctrl->volume_icon, 6, 6, 6, 13, 2);
    divoom_tbv_video_create_solid(ctrl->volume_icon, 15, 8, 2, 9, 1);
    divoom_tbv_video_create_solid(ctrl->volume_icon, 20, 5, 2, 15, 1);

    ctrl->brightness_icon = divoom_tbv_video_create_icon_root(ctrl->main_obj);
    divoom_tbv_video_create_solid(ctrl->brightness_icon, 7, 7, 10, 10, 5);
    divoom_tbv_video_create_solid(ctrl->brightness_icon, 11, 1, 2, 4, 1);
    divoom_tbv_video_create_solid(ctrl->brightness_icon, 11, 19, 2, 4, 1);
    divoom_tbv_video_create_solid(ctrl->brightness_icon, 1, 11, 4, 2, 1);
    divoom_tbv_video_create_solid(ctrl->brightness_icon, 19, 11, 4, 2, 1);
    divoom_tbv_video_create_solid(ctrl->brightness_icon, 4, 4, 3, 3, 1);
    divoom_tbv_video_create_solid(ctrl->brightness_icon, 17, 4, 3, 3, 1);
    divoom_tbv_video_create_solid(ctrl->brightness_icon, 4, 17, 3, 3, 1);
    divoom_tbv_video_create_solid(ctrl->brightness_icon, 17, 17, 3, 3, 1);
}

static void divoom_tbv_video_release_buffers(divoom_tbv_video_ctrl *ctrl)
{
    int i;

    if (ctrl->osd_enabled)
    {
        divoom_menu_control_disable_video_ui_overlay();
        ctrl->osd_enabled = 0;
    }
#if !defined(DIVOOM_SIMULATOR) || !(DIVOOM_SIMULATOR)
    for (i = 0; i < 2; i++)
    {
        if (ctrl->osd_buf[i])
        {
            sunxifb_free(&ctrl->osd_buf[i],
                i == 0 ? "tbv_video_osd0" : "tbv_video_osd1");
        }
    }
#else
    (void)i;
#endif
    ctrl->osd_buf_size = 0;
    ctrl->osd_front_index = 0;
}

static void divoom_tbv_video_delete(void)
{
    divoom_tbv_video_ctrl *ctrl = &g_tbv_video;

    if (ctrl->exit_timer)
    {
        lv_timer_del(ctrl->exit_timer);
        ctrl->exit_timer = NULL;
    }
    divoom_tbv_video_release_buffers(ctrl);
    if (ctrl->main_obj)
    {
        lv_obj_del(ctrl->main_obj);
        ctrl->main_obj = NULL;
    }
    if (ctrl->styles_initialized)
    {
        lv_style_reset(&ctrl->bar_style);
        lv_style_reset(&ctrl->fill_style);
    }
    memset(ctrl, 0, sizeof(*ctrl));
}

static void divoom_tbv_video_exit_timer_cb(lv_timer_t *timer)
{
    divoom_tbv_video_ctrl *ctrl = &g_tbv_video;

    (void)timer;
    ctrl->exit_time++;
    if (ctrl->exit_time >= TBV_VIDEO_OSD_DISMISS_TICKS)
    {
        divoom_tbv_video_delete();
    }
}

static void divoom_tbv_video_create_ui(divoom_tbv_video_ctrl *ctrl)
{
    lv_style_init(&ctrl->bar_style);
    lv_style_set_bg_color(&ctrl->bar_style, lv_color_hex(0x161616));
    lv_style_set_bg_opa(&ctrl->bar_style, (lv_opa_t)(LV_OPA_COVER * 70 / 100));
    lv_style_set_border_width(&ctrl->bar_style, TBV_VIDEO_OSD_BORDER);
    lv_style_set_border_color(&ctrl->bar_style, lv_color_hex(0x333333));
    lv_style_set_radius(&ctrl->bar_style, TBV_VIDEO_OSD_RADIUS);
    lv_style_set_pad_all(&ctrl->bar_style, 0);
    lv_style_set_clip_corner(&ctrl->bar_style, true);
    lv_style_set_shadow_width(&ctrl->bar_style, 0);
    lv_style_set_outline_width(&ctrl->bar_style, 0);

    lv_style_init(&ctrl->fill_style);
    lv_style_set_bg_color(&ctrl->fill_style, lv_color_hex(0x666666));
    lv_style_set_bg_opa(&ctrl->fill_style, LV_OPA_COVER);
    lv_style_set_border_width(&ctrl->fill_style, 0);
    lv_style_set_radius(&ctrl->fill_style, 0);
    lv_style_set_pad_all(&ctrl->fill_style, 0);
    ctrl->styles_initialized = 1;

    ctrl->main_obj = lv_obj_create(lv_layer_top());
    lv_obj_add_style(ctrl->main_obj, &ctrl->bar_style, 0);
    lv_obj_set_pos(ctrl->main_obj, TBV_VIDEO_OSD_X, TBV_VIDEO_OSD_Y);
    lv_obj_set_size(ctrl->main_obj, TBV_VIDEO_OSD_W, TBV_VIDEO_OSD_H);
    lv_obj_clear_flag(ctrl->main_obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(ctrl->main_obj, LV_OBJ_FLAG_CLICKABLE);

    ctrl->fill_obj = lv_obj_create(ctrl->main_obj);
    lv_obj_add_style(ctrl->fill_obj, &ctrl->fill_style, 0);
    lv_obj_set_pos(ctrl->fill_obj, 0, 0);
    lv_obj_set_height(ctrl->fill_obj, TBV_VIDEO_OSD_INNER_H);
    lv_obj_clear_flag(ctrl->fill_obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(ctrl->fill_obj, LV_OBJ_FLAG_CLICKABLE);

    divoom_tbv_video_create_icons(ctrl);
    ctrl->exit_timer = lv_timer_create(divoom_tbv_video_exit_timer_cb, 200, NULL);
}

static void divoom_tbv_video_update_ui(divoom_tbv_video_ctrl *ctrl)
{
    int fill_w;

    if (ctrl->fill_percent <= 0)
    {
        fill_w = 0;
        lv_obj_add_flag(ctrl->fill_obj, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        fill_w = (ctrl->fill_percent * TBV_VIDEO_OSD_INNER_W + 50) / 100;
        if (fill_w > TBV_VIDEO_OSD_INNER_W)
        {
            fill_w = TBV_VIDEO_OSD_INNER_W;
        }
        lv_obj_clear_flag(ctrl->fill_obj, LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_set_width(ctrl->fill_obj, fill_w);
    if (ctrl->brightness_mode)
    {
        lv_obj_add_flag(ctrl->volume_icon, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ctrl->brightness_icon, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        lv_obj_clear_flag(ctrl->volume_icon, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ctrl->brightness_icon, LV_OBJ_FLAG_HIDDEN);
    }
}

static void divoom_tbv_video_update_osd(divoom_tbv_video_ctrl *ctrl)
{
#if !defined(DIVOOM_SIMULATOR) || !(DIVOOM_SIMULATOR)
    lv_draw_buf_t draw_buf;
    lv_area_t coords;
    void *back_buf;
    void *phy_addr;
    uint32 need_size;
    uint32 image_w;
    uint32 image_h;
    uint32 y;
    uint8 back_index;
    int i;
    int ret;

    lv_obj_update_layout(ctrl->main_obj);
    image_w = (uint32)lv_obj_get_width(ctrl->main_obj);
    image_h = (uint32)lv_obj_get_height(ctrl->main_obj);
    need_size = LV_DRAW_BUF_SIZE(image_w, image_h, LV_COLOR_FORMAT_ARGB8888);
    if (!ctrl->osd_buf[0] || !ctrl->osd_buf[1] || ctrl->osd_buf_size < need_size)
    {
        divoom_tbv_video_release_buffers(ctrl);
        for (i = 0; i < 2; i++)
        {
            ctrl->osd_buf[i] = sunxifb_alloc(need_size,
                i == 0 ? "tbv_video_osd0" : "tbv_video_osd1");
            if (!ctrl->osd_buf[i])
            {
                divoom_print("sleep bright/volume TOP OSD buffer:%d alloc fail:%u\n", i, need_size);
                divoom_tbv_video_release_buffers(ctrl);
                return;
            }
        }
        ctrl->osd_buf_size = need_size;
    }

    back_index = ctrl->osd_enabled ? (uint8)(ctrl->osd_front_index ^ 1U) : 0;
    back_buf = ctrl->osd_buf[back_index];
    memset(back_buf, 0, ctrl->osd_buf_size);
    ret = lv_draw_buf_init(&draw_buf, image_w, image_h,
        LV_COLOR_FORMAT_ARGB8888, LV_STRIDE_AUTO,
        back_buf, ctrl->osd_buf_size);
    if (ret != LV_RES_OK)
    {
        divoom_print("sleep bright/volume TOP OSD draw buffer fail:%d\n", ret);
        return;
    }
    ret = lv_snapshot_take_to_draw_buf(ctrl->main_obj,
        LV_COLOR_FORMAT_ARGB8888, &draw_buf);
    if (ret != LV_RES_OK)
    {
        divoom_print("sleep bright/volume TOP OSD snapshot fail:%d\n", ret);
        return;
    }

    image_w = draw_buf.header.w;
    image_h = draw_buf.header.h;
    for (y = 0; y < image_h; y++)
    {
        lv_color32_t *src = (lv_color32_t *)((uint8_t *)draw_buf.data +
            y * draw_buf.header.stride);
        uint16_t *dst = (uint16_t *)draw_buf.data + y * image_w;
        uint32 x;

        for (x = 0; x < image_w; x++)
        {
            lv_color32_t pixel = src[x];
            dst[x] = (pixel.alpha >= 0x80U ? 0x8000U : 0U) |
                ((uint16_t)(pixel.red >> 3) << 10) |
                ((uint16_t)(pixel.green >> 3) << 5) |
                (uint16_t)(pixel.blue >> 3);
        }
    }

    lv_obj_get_coords(ctrl->main_obj, &coords);
    phy_addr = sunxifb_mem_get_phyaddr(draw_buf.data);
    if (!phy_addr)
    {
        divoom_print("sleep bright/volume TOP OSD physical address fail\n");
        return;
    }
    sunxifb_flush_cache(draw_buf.data,
        image_w * image_h * (uint32)sizeof(uint16_t));
    ret = divoom_menu_control_set_video_ui_overlay_argb1555(phy_addr,
        coords.x1, coords.y1, image_w, image_h);
    if (ret != 0)
    {
        divoom_print("sleep bright/volume TOP OSD layer update fail:%d\n", ret);
        return;
    }
    ctrl->osd_front_index = back_index;
    ctrl->osd_enabled = 1;
    divoom_print("sleep bright/volume TOP OSD updated:%ux%u mode:%u value:%d index:%u\n",
        image_w, image_h, ctrl->brightness_mode, ctrl->fill_percent, back_index);
#else
    (void)ctrl;
#endif
}

void divoom_lvgl_top_bright_volume_video_show(int brightness_mode, int fill_percent)
{
    divoom_tbv_video_ctrl *ctrl = &g_tbv_video;

    if (fill_percent < 0)
    {
        fill_percent = 0;
    }
    else if (fill_percent > 100)
    {
        fill_percent = 100;
    }
    if (!ctrl->main_obj)
    {
        divoom_tbv_video_create_ui(ctrl);
    }
    ctrl->brightness_mode = brightness_mode ? 1 : 0;
    ctrl->fill_percent = fill_percent;
    ctrl->exit_time = 0;
    divoom_tbv_video_update_ui(ctrl);
    divoom_tbv_video_update_osd(ctrl);
    if (ctrl->main_obj)
    {
        divoom_lvgl_layer_top_raise(ctrl->main_obj);
    }
}

void divoom_lvgl_top_bright_volume_video_exit(void)
{
    divoom_tbv_video_delete();
}

#else

void divoom_lvgl_top_bright_volume_video_show(int brightness_mode, int fill_percent)
{
    (void)brightness_mode;
    (void)fill_percent;
}

void divoom_lvgl_top_bright_volume_video_exit(void)
{
}

#endif

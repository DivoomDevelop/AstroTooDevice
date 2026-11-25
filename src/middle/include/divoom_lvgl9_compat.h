/**
 * @file divoom_lvgl9_compat.h
 * LVGL9 兼容头（单一文件，仿真与 F101 真机共用，可双向同步）。
 *
 * 仿真(LvglPlatform fork) 与 真机(upstream littlevgl-9) 是两个不同的 LVGL9 fork，
 * 源码层差异通过仿真宏 DIVOOM_LVGL9_IS_SIM 在编译期分支：
 *   1. 图像头初始化：仿真 lv_img_header_t.always_zero  /  真机 lv_image_header_t.magic
 *   2. 按钮类符号：  仿真原生 lv_btn_class             /  真机仅 lv_button_class（需别名）
 *   3. 图像缓存失效：仿真原生 lv_img_cache_invalidate_src / 真机 lv_image_cache_drop（需别名）
 *   4. 绘图图像 API：仿真仍为 lv_draw_img*             /  真机为 lv_draw_image*（需别名）
 *
 * 仿真判定：sim_lvgl9.props 注入 DIVOOM_LVGL9_SIM；并以 _WIN32 兜底
 * （MSVC 必定义，真机 riscv-gcc 交叉编译必不定义），即使个别 TU 漏挂 props 也不会误判。
 * 勿定义 lv_event_send 等与 LVGL 内部符号同名的宏（会破坏 lv_obj_event.c 等核心源码）。
 */
#ifndef DIVOOM_LVGL9_COMPAT_H
#define DIVOOM_LVGL9_COMPAT_H

#include "lvgl.h"

#if defined(DIVOOM_LVGL9_SIM) || defined(_WIN32)
#define DIVOOM_LVGL9_IS_SIM 1
#else
#define DIVOOM_LVGL9_IS_SIM 0
#endif

#if !DIVOOM_LVGL9_IS_SIM
#include "src/core/lv_obj_class_private.h"
#include "src/core/lv_obj_private.h"
#include "src/draw/lv_image_decoder_private.h"
#include "src/misc/lv_timer_private.h"
#endif

#if !DIVOOM_LVGL9_IS_SIM
/* lv_conf.h defaults FREETYPE=0; AstroToo app TUs need compat freetype shims. */
#undef LV_USE_FREETYPE
#define LV_USE_FREETYPE 1
#endif

#ifndef DIVOOM_LV_IMG_HEADER_INIT
#if DIVOOM_LVGL9_IS_SIM
/* LvglPlatform 仿真 fork 仍使用 lv_img_header_t.always_zero（非 upstream magic 字段） */
#define DIVOOM_LV_IMG_HEADER_INIT(hdr) do { if (hdr) { (hdr)->always_zero = 0; } } while(0)
#else
/* upstream lv_image_header_t 使用 magic，无 always_zero */
#define DIVOOM_LV_IMG_HEADER_INIT(hdr) do { if (hdr) { (hdr)->magic = LV_IMAGE_HEADER_MAGIC; } } while(0)
#endif
#endif

/* 三角形绘制描述符在仿真 fork 使用 bg_color/bg_opa，真机 upstream 使用 color/opa。 */
#ifndef DIVOOM_LV_TRIANGLE_DSC_SET_COLOR
#if DIVOOM_LVGL9_IS_SIM
#define DIVOOM_LV_TRIANGLE_DSC_SET_COLOR(dsc, c, o) do { (dsc)->bg_color = (c); (dsc)->bg_opa = (o); } while(0)
#else
#define DIVOOM_LV_TRIANGLE_DSC_SET_COLOR(dsc, c, o) do { (dsc)->color = (c); (dsc)->opa = (o); } while(0)
#endif
#endif

/* 真机三角形顶点为 lv_point_precise_t，逐字段赋值可兼容 lv_point_t 与 precise point。 */
#ifndef DIVOOM_LV_TRIANGLE_DSC_SET_POINT
#define DIVOOM_LV_TRIANGLE_DSC_SET_POINT(dsc, idx, px, py) do { \
    (dsc)->p[(idx)].x = (lv_coord_t)(px); \
    (dsc)->p[(idx)].y = (lv_coord_t)(py); \
} while(0)
#endif

/* 图像旋转字段：仿真 fork 为 angle，真机 upstream 为 rotation，单位均为 0.1 度。 */
#ifndef DIVOOM_LV_IMAGE_DSC_SET_ROTATION
#if DIVOOM_LVGL9_IS_SIM
#define DIVOOM_LV_IMAGE_DSC_SET_ROTATION(dsc, angle_deg10) do { \
    (dsc)->angle = (lv_coord_t)(angle_deg10); \
} while(0)
#else
#define DIVOOM_LV_IMAGE_DSC_SET_ROTATION(dsc, angle_deg10) do { \
    (dsc)->rotation = (int32_t)(angle_deg10); \
} while(0)
#endif
#endif

#if !DIVOOM_LVGL9_IS_SIM
/* 真机：upstream 仅有 lv_button_class / lv_image_cache_drop，需别名为旧名 */
#ifndef lv_btn_class
#define lv_btn_class lv_button_class
#endif
#ifndef lv_img_cache_invalidate_src
#define lv_img_cache_invalidate_src(src) lv_image_cache_drop(src)
#endif
#ifndef lv_draw_img_dsc_t
#define lv_draw_img_dsc_t lv_draw_image_dsc_t
#endif
#ifndef lv_draw_img_dsc_init
#define lv_draw_img_dsc_init lv_draw_image_dsc_init
#endif
#ifndef lv_draw_img
#define lv_draw_img lv_draw_image
#endif
#ifndef LV_RES_OK
#define LV_RES_OK         LV_RESULT_OK
#endif
#ifndef LV_RES_INV
#define LV_RES_INV        LV_RESULT_INVALID
#endif
#ifndef lv_img_header_t
#define lv_img_header_t lv_image_header_t
#endif
#endif

#ifndef LV_IMG_CF_TRUE_COLOR
/* 应用层图片缓存与真机保持 RGB565；带透明通道时使用 RGB565A8 平面布局。 */
#define LV_IMG_CF_TRUE_COLOR            LV_COLOR_FORMAT_RGB565
#endif
#ifndef LV_IMG_CF_TRUE_COLOR_ALPHA
#define LV_IMG_CF_TRUE_COLOR_ALPHA      LV_COLOR_FORMAT_RGB565A8
#endif

#ifndef DIVOOM_LVGL_DISPLAY_COLOR_FORMAT
#define DIVOOM_LVGL_DISPLAY_COLOR_FORMAT LV_COLOR_FORMAT_RGB565
#endif

#ifndef DIVOOM_LVGL_OVERLAY_COLOR_FORMAT
#define DIVOOM_LVGL_OVERLAY_COLOR_FORMAT LV_COLOR_FORMAT_RGB565A8
#endif
#ifndef LV_IMG_CF_RGB565
#define LV_IMG_CF_RGB565                LV_COLOR_FORMAT_RGB565
#endif
#ifndef LV_IMG_CF_RGB565A8
#define LV_IMG_CF_RGB565A8              LV_COLOR_FORMAT_RGB565A8
#endif

#ifndef LV_IMG_PX_SIZE_ALPHA_BYTE
#if DIVOOM_LVGL9_IS_SIM
#define LV_IMG_PX_SIZE_ALPHA_BYTE       _LV_COLOR_NATIVE_WITH_ALPHA_SIZE
#else
#define LV_IMG_PX_SIZE_ALPHA_BYTE       LV_COLOR_NATIVE_WITH_ALPHA_SIZE
#endif
#endif

#ifndef LV_CANVAS_BUF_SIZE_TRUE_COLOR_ALPHA
/* 旧 LVGL8/仿真接口：RGB565 系统的 true color alpha 缓冲为 RGB565 + A8 平面布局。 */
#define LV_CANVAS_BUF_SIZE_TRUE_COLOR_ALPHA(w, h) (LV_IMG_PX_SIZE_ALPHA_BYTE * (w) * (h))
#endif

#if DIVOOM_LVGL9_IS_SIM
#if LV_IMG_CACHE_DEF_SIZE > 0
#define DIVOOM_LVGL9_SIM_IMG_CACHE_DEFAULT_ENTRIES LV_IMG_CACHE_DEF_SIZE
#else
#define DIVOOM_LVGL9_SIM_IMG_CACHE_DEFAULT_ENTRIES 128
#endif
#define DIVOOM_LVGL9_SIM_IMG_CACHE_GAME_ENTRIES 4
#else
#ifndef LV_CACHE_DEF_SIZE
#define LV_CACHE_DEF_SIZE (8U * 1024U * 1024U)
#endif
void lv_img_cache_set_size(uint32_t size);
#endif

#ifndef lv_chart_set_series_value_by_id
#define lv_chart_set_series_value_by_id lv_chart_set_value_by_id
#endif

#ifndef LV_DISP_DEF_REFR_PERIOD
#define LV_DISP_DEF_REFR_PERIOD  LV_DEF_REFR_PERIOD
#endif

#ifndef lv_mem_free
#define lv_mem_free  lv_free
#endif

#ifndef lv_color_to32
#define lv_color_to32  lv_color_to_u32
#endif

void lv_canvas_draw_text(lv_obj_t * canvas, lv_coord_t x, lv_coord_t y, lv_coord_t max_w,
                         lv_draw_label_dsc_t * draw_dsc, const char * txt);
lv_opa_t lv_img_buf_get_px_alpha(const lv_img_dsc_t * dsc, lv_coord_t x, lv_coord_t y);
void lv_img_buf_set_px_alpha(const lv_img_dsc_t * dsc, lv_coord_t x, lv_coord_t y, lv_opa_t opa);

void divoom_lvgl9_set_screen_transp(int on);
int divoom_lvgl9_get_screen_transp(void);

static inline void divoom_lvgl9_image_cache_drop_all(void)
{
    lv_img_cache_invalidate_src(NULL);
}

static inline void divoom_lvgl9_image_cache_set_game_limit(uint32_t size_bytes)
{
#if DIVOOM_LVGL9_IS_SIM
    /*
     * 仿真 fork 仍沿用 LVGL8 语义：lv_img_cache_set_size 参数是条目数。
     * 真机 LVGL9 compat 映射为字节数，因此这里必须分支，避免把 256KB 当成条目数。
     */
    (void)size_bytes;
    lv_img_cache_set_size(DIVOOM_LVGL9_SIM_IMG_CACHE_GAME_ENTRIES);
#else
    lv_img_cache_set_size(size_bytes);
#endif
}

static inline void divoom_lvgl9_image_cache_restore_default(void)
{
#if DIVOOM_LVGL9_IS_SIM
    lv_img_cache_set_size(DIVOOM_LVGL9_SIM_IMG_CACHE_DEFAULT_ENTRIES);
#else
    lv_img_cache_set_size((uint32_t)LV_CACHE_DEF_SIZE);
#endif
}

void lv_disp_set_bg_opa(lv_disp_t * disp, lv_opa_t opa);
void lv_disp_set_bg_color(lv_disp_t * disp, lv_color_t color);

lv_coord_t divoom_lv_txt_get_width(const char * txt, uint32_t length, const lv_font_t * font,
                                   lv_coord_t letter_space, lv_text_flag_t flags);

#if LV_USE_FREETYPE && !defined(DIVOOM_LV_FT_INFO_T)
#define DIVOOM_LV_FT_INFO_T
typedef struct {
    const char * name;
    const void * mem;
    size_t mem_size;
    lv_font_t * font;
    uint16_t weight;
    uint16_t style;
} lv_ft_info_t;

#ifndef FT_FONT_STYLE_NORMAL
#ifndef LV_FREETYPE_FONT_STYLE_NORMAL
#define LV_FREETYPE_FONT_STYLE_NORMAL  0
#endif
#define FT_FONT_STYLE_NORMAL  LV_FREETYPE_FONT_STYLE_NORMAL
#endif
#ifndef FT_FONT_STYLE_ITALIC
#ifndef LV_FREETYPE_FONT_STYLE_ITALIC
#define LV_FREETYPE_FONT_STYLE_ITALIC  1
#endif
#define FT_FONT_STYLE_ITALIC  LV_FREETYPE_FONT_STYLE_ITALIC
#endif
#ifndef FT_FONT_STYLE_BOLD
#ifndef LV_FREETYPE_FONT_STYLE_BOLD
#define LV_FREETYPE_FONT_STYLE_BOLD  2
#endif
#define FT_FONT_STYLE_BOLD    LV_FREETYPE_FONT_STYLE_BOLD
#endif

/* 显式声明 freetype 工厂函数：本头被多个 TU 强制包含，且仓库存在两份 lvgl
 * 副本（LvglPlatform 与 third_party/lvgl-8），若某 TU 的 lvgl.h 未引入
 * lv_freetype.h，则 lv_freetype_font_create 会被隐式声明为返回 int，
 * 64 位 lv_font_t* 被截断并符号扩展（如 0xFFFFFFFFDF8401D0），随后
 * lv_font_get_line_height 解引用即崩溃。此处补声明确保返回类型正确。 */
#if DIVOOM_LVGL9_IS_SIM
extern lv_font_t * lv_freetype_font_create(const char * pathname, uint16_t size, uint16_t style);
extern void lv_freetype_font_del(lv_font_t * font);
#else
#include "libs/freetype/lv_freetype.h"
extern lv_font_t * lv_freetype_font_create(const char * pathname,
                                           lv_freetype_font_render_mode_t render_mode,
                                           uint32_t size, lv_freetype_font_style_t style);
#endif

static inline bool lv_ft_font_init(lv_ft_info_t * info)
{
    if(info == NULL || info->name == NULL) {
        return false;
    }
#if DIVOOM_LVGL9_IS_SIM
    info->font = lv_freetype_font_create(info->name, info->weight, info->style);
#else
    info->font = lv_freetype_font_create(info->name, LV_FREETYPE_FONT_RENDER_MODE_BITMAP,
                                         (uint32_t)info->weight, (lv_freetype_font_style_t)info->style);
#endif
    return info->font != NULL;
}

static inline void lv_ft_font_destroy(lv_font_t * font)
{
    if(font != NULL) {
#if DIVOOM_LVGL9_IS_SIM
        lv_freetype_font_del(font);
#else
        lv_freetype_font_delete(font);
#endif
    }
}
#endif /* DIVOOM_LV_FT_INFO_T */

#if !DIVOOM_LVGL9_IS_SIM
#ifndef LV_IMG_SIZE_MODE_REAL
#define LV_IMG_SIZE_MODE_REAL  0
#endif
#ifndef LV_IMG_SIZE_MODE_VIRTUAL
#define LV_IMG_SIZE_MODE_VIRTUAL  1
#endif
#ifndef lv_img_set_size_mode
#define lv_img_set_size_mode(img, mode)  ((void)(img), (void)(mode))
#endif
#ifndef LV_IMG_ZOOM_NONE
#define LV_IMG_ZOOM_NONE  LV_ZOOM_NONE
#endif
#ifndef lv_img_set_zoom
#define lv_img_set_zoom(img, zoom)  lv_image_set_scale((img), (uint32_t)(zoom))
#endif
/* 旧接口的 anim_speed 是像素/秒；真机 LVGL9 改为 anim_duration，但可接收 lv_anim_speed() 编码。 */
#define lv_obj_set_style_anim_speed(obj, val, sel)  lv_obj_set_style_anim_duration((obj), lv_anim_speed((uint32_t)(val)), (sel))
void lv_img_cache_set_size(uint32_t size);

typedef struct {
	lv_image_header_t header;
	const uint8_t *img_data;
	lv_image_decoder_dsc_t dsc;
} lv_img_decoder_dsc_t;

lv_result_t lv_img_decoder_open(lv_img_decoder_dsc_t *dsc, const char *src,
				lv_color_t color, int32_t frame_id);
lv_result_t lv_img_decoder_get_info(const char *src, lv_image_header_t *header);
void lv_img_decoder_close(lv_img_decoder_dsc_t *dsc);

lv_img_dsc_t *divoom_lvgl9_snapshot_take(lv_obj_t *obj, lv_color_format_t cf);
void divoom_lvgl9_snapshot_free(lv_img_dsc_t *dsc);
#define lv_snapshot_take(obj, cf)  divoom_lvgl9_snapshot_take((obj), (lv_color_format_t)(cf))
#define lv_snapshot_free(dsc)      divoom_lvgl9_snapshot_free((dsc))
#endif /* !DIVOOM_LVGL9_IS_SIM */

#endif /* DIVOOM_LVGL9_COMPAT_H */

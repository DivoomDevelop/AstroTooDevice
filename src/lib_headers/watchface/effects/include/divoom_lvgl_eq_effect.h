#ifndef _divoom_lvgl_eq_effect_h_
#define _divoom_lvgl_eq_effect_h_

#include <stdint.h>

/* JL 跟拍 15ms；麦克/light_music 100ms（与 add_divoom_queue 一致），运行时按音源切换 LVGL 定时器 */
#define DIVOOM_LVGL_EQ_EFFECT_JL_REFRESH_MS   15
#define DIVOOM_LVGL_EQ_EFFECT_MIC_REFRESH_MS  100
#define DIVOOM_LVGL_EQ_EFFECT_REFRESH_MS      DIVOOM_LVGL_EQ_EFFECT_JL_REFRESH_MS
/* JL UART 超时柱归零，不回退 F101 麦；全 0 仍走 JL */
#define DIVOOM_LVGL_EQ_EFFECT_JL_SILENCE_MS  200

#define DIVOOM_EQ_EFFECT_MAX_BAR_COUNT       108
/* 1642 轨道终端 EQ 画布 150×47 */
#define DIVOOM_LVGL_EQ_EFFECT_MAX_CANVAS_W   480
#define DIVOOM_LVGL_EQ_EFFECT_MAX_CANVAS_H   340
#define DIVOOM_LVGL_EQ_EFFECT_MAX_RGB_BYTES  (DIVOOM_LVGL_EQ_EFFECT_MAX_CANVAS_W * DIVOOM_LVGL_EQ_EFFECT_MAX_CANVAS_H * 3)
#define DIVOOM_LVGL_EQ_EFFECT_MAX_RGB565_BYTES  (DIVOOM_LVGL_EQ_EFFECT_MAX_CANVAS_W * DIVOOM_LVGL_EQ_EFFECT_MAX_CANVAS_H * 2)

/** 只画音柱时未绘制像素的 RGB 标记（stretch 后转 alpha=0） */
#define DIVOOM_EQ_EFFECT_BARS_ONLY_KEY_R  0x01
#define DIVOOM_EQ_EFFECT_BARS_ONLY_KEY_G  0x00
#define DIVOOM_EQ_EFFECT_BARS_ONLY_KEY_B  0x01

typedef struct _divoom_disp_clock_ctrl *Pdivoom_disp_clock_ctrl;
typedef struct _divoom_disp_clock_disp_item *Pdivoom_disp_clock_disp_item;

typedef struct {
    int canvas_w;       //画布宽
    int canvas_h;       //画布高
    int origin_x;       //原点x
    int origin_y;       //原点y
    int draw_w;         //绘制宽
    int draw_h;         //绘制高
    int bar_width;      //柱宽；line_wave=1 时为线条粗细(px)
    int bar_gap;        //柱间距
    int bar_count;      //柱数
    int max_bar_height; //柱高；center_mirror=1 时为半高 draw_h/2
    int bar_radius;     //半径
    /** >0 时分块律动（1642：6×2 像素块竖叠）；0=实心柱 */
    int bar_block_h;    //分块高度
    /** 分块竖叠时节与节之间的透明间距；0=节间无缝 */
    int bar_block_gap;  //分块间距
    /** 1=铺 bg_color 不透明底；0=只画音柱，其余像素透明 */
    int opaque_bg;      //是否铺背景颜色
    uint8 bg_color[3];  //背景颜色
    uint8 color[3];     //柱颜色（渐变起始色，柱底）
    uint8 color_end[3]; //柱渐变结束色（柱顶）；与 color 相同时为纯色
    /** 1=从垂直中心向上下对称分块律动（1890 波形单体） */
    int center_mirror;
    /** 1=绘制中心水平发光带，厚度随该列能量起伏 */
    int center_glow;
    /** >0 时分块画成弧形音波带：柱中心相对两端下沉的像素数 */
    int block_arc;
    /** 1=低频在中心、高频向两侧展开（对称频谱）；0=从左到右低→高 */
    int freq_from_center;
    /** 1=示波器单线条：连接各采样点 y 值，不填充柱/块 */
    int line_wave;
    /** >0：该柱有 EQ 数据时叠加的高度(px)；0=不叠加 */
    int min_bar_height;
} divoom_eq_effect_preset_t;

#define DIVOOM_LVGL_EQ_EFFECT_CLOCK_WAVEFORM_MONOLITH  1890

const divoom_eq_effect_preset_t *divoom_lvgl_eq_effect_preset_from_clock_id(int clock_id);

void *divoom_lvgl_eq_effect_bind_clock(Pdivoom_disp_clock_ctrl pclock, Pdivoom_disp_clock_disp_item pdisp_attr);

void divoom_lvgl_eq_effect_unbind_clock(void *bind);

void divoom_lvgl_eq_effect_get_disp_buf(void *user_data, uint8 *disp_buf, int *image_width);
void divoom_lvgl_eq_effect_get_disp_rgb565_buf(void *user_data, uint8 *disp_buf, int *image_width);
/** 原尺寸抗锯齿 EQ（线形及 2084/3200 圆角音柱），输出 LV_COLOR_FORMAT_RGB565A8。 */
void divoom_lvgl_eq_effect_get_disp_rgb565a8_buf(void *user_data, uint8 *disp_buf, int *image_width);

/** eq_effect bind 有效时返回画布尺寸，否则 -1 */
int divoom_lvgl_eq_effect_get_canvas_size(void *user_data, int *canvas_w, int *canvas_h);

/** 1890 等：画布与控件同尺寸时可直画 lvgl buffer，跳过离屏 buffer */
int divoom_lvgl_eq_effect_prefer_direct_draw(void *user_data, int area_w, int area_h);

/** 当前 EQ 是否使用 RGB565A8 原尺寸直绘（2426、2084、3200）。 */
int divoom_lvgl_eq_effect_use_alpha_direct_draw(void *user_data, int area_w, int area_h);

int divoom_lvgl_eq_effect_is_transparent_mode(void *user_data);

/** bars_only 模式下 stretch 后：标记色 → 透明，音柱像素 → 不透明 */
int divoom_lvgl_eq_effect_apply_bars_only_alpha(uint8 *rgba_buf, int px_count);
int divoom_lvgl_eq_effect_rgb565_is_bars_only_key(const uint8 *rgb565_px);

/** eq_effect 绑定有效时返回 JL/MIC 对应 LVGL 刷新周期(ms)，否则 -1 */
int divoom_lvgl_eq_effect_get_lvgl_refresh_ms(void *user_data);

/** 频谱显示用的 0～64 对比度曲线，保留强峰，压低弱频带。 */
int32 divoom_lvgl_eq_effect_contrast_level(int32 level);

/** 按当前 JL/麦克音源返回 LVGL 刷新周期（15 或 100），light_music 与 eq_effect 共用 */
int divoom_lvgl_eq_effect_get_jl_mic_refresh_ms(void);

#endif

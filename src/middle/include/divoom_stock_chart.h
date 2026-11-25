#ifndef __DIVOOM_STOCK_CHART_H__
#define __DIVOOM_STOCK_CHART_H__

#include "lvgl.h"
#include "divoom_lvgl9_compat.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DIVOOM_STOCK_CHART_GLOW_CLOCK_ID        1788       /* 财富殿堂美股组件 clock_id，使用发光曲线 */
#define DIVOOM_STOCK_CHART_GLOW_LINE_COLOR      0x40DE00   /* 发光折线色（设计稿 #40DE00） */
#define DIVOOM_STOCK_CHART_GLOW_FILL_COLOR      0x40DE00   /* 曲线下方渐变顶色 */

typedef enum {
    DIVOOM_STOCK_CHART_STYLE_CLASSIC = 0,   /* 经典 lv_chart 折线 */
    DIVOOM_STOCK_CHART_STYLE_GLOW    = 1,   /* canvas 渐变填充 + 多层发光折线 */
} divoom_stock_chart_style_t;

/* 时间段价格数据结构 */
typedef struct {
    float start_price;     /* 时间段开始价格 */
    float end_price;       /* 时间段结束价格 */
    uint32_t time_start;   /* 开始时间戳（可选） */
    uint32_t time_end;     /* 结束时间戳（可选） */
} divoom_stock_time_period_t;

/* 股票图表数据结构 */
typedef struct _divoom_stock_chart_t{
    uint8_t style;                 /* divoom_stock_chart_style_t */
    uint32_t line_color;           /* Glow 折线色 */
    uint32_t fill_color;           /* Glow 渐变顶色 */
    lv_coord_t canvas_w;           /* Glow 画布宽 */
    lv_coord_t canvas_h;           /* Glow 画布高 */
    uint8_t *canvas_buf;           /* Glow 画布缓冲 */
    lv_obj_t *canvas_obj;          /* Glow 画布对象 */
    lv_obj_t *chart_obj;           /* 图表对象 */
    lv_chart_series_t *price_series;  /* 价格曲线系列 */
    lv_chart_series_t *volume_series; /* 成交量曲线系列（可选） */
    float *price_data;             /* 价格数据数组 */
    float *volume_data;            /* 成交量数据数组（可选） */
    int data_count;                /* 数据点数量 */
    float min_price;               /* Y轴最小价格（包含边距） */
    float max_price;               /* Y轴最大价格（包含边距） */
    float actual_min_price;        /* 实际数据中的最低价格 */
    float actual_max_price;        /* 实际数据中的最高价格 */
    float current_price;           /* 当前价格 */
    float prev_price;              /* 前一个价格 */
    char stock_name[32];           /* 股票名称 */
    uint8_t time_period_mode;      /* 时间段模式：0=普通模式，1=时间段模式 */
    float last_period_end_price;   /* 上一个时间段的结束价格（用于连接） */
    /* 轴标签对象 */
    lv_obj_t *label_max_price;     /* Y轴最高价格标签 */
    lv_obj_t *label_min_price;     /* Y轴最低价格标签 */
    lv_obj_t *label_time_start;    /* X轴开始时间标签（保留用于兼容） */
    lv_obj_t *label_time_end;      /* X轴结束时间标签（保留用于兼容） */
    lv_obj_t **time_labels;        /* X轴时间点标签数组 */
    int time_label_count;          /* 当前时间标签数量 */
    uint32_t time_start;           /* 开始时间戳 */
    uint32_t time_end;             /* 结束时间戳 */
    uint32_t time_interval_minutes; /* 时间间隔（分钟），如30表示每30分钟显示一个时间点 */
} divoom_stock_chart_t, *Pdivoom_stock_chart_t;

/**
 * @brief 创建股票曲线图表
 * @param parent 父对象，如果为NULL则创建在屏幕上
 * @param x x坐标位置
 * @param y y坐标位置
 * @param width 图表宽度
 * @param height 图表高度
 * @param stock_name 股票名称
 * @param style DIVOOM_STOCK_CHART_STYLE_CLASSIC 或 DIVOOM_STOCK_CHART_STYLE_GLOW
 * @return 返回股票图表结构指针，失败返回NULL
 */
divoom_stock_chart_t* divoom_stock_chart_create(lv_obj_t *parent, lv_coord_t x, lv_coord_t y, 
                                                 lv_coord_t width, lv_coord_t height, 
                                                 const char *stock_name, uint8_t style);

/**
 * @brief 更新股票价格数据
 * @param chart 股票图表结构指针
 * @param prices 价格数组
 * @param count 数据点数量
 * @return 0成功，-1失败
 */
int divoom_stock_chart_update_prices(divoom_stock_chart_t *chart, float *prices, int *time_stamp, int count, float max_price, float min_price);

/**
 * @brief 销毁股票图表
 * @param chart 股票图表结构指针
 * @return 0成功，-1失败
 */
int divoom_stock_chart_destroy(divoom_stock_chart_t *chart);

/**
 * @brief 设置时间范围（用于X轴时间标签显示）
 * @param chart 股票图表结构指针
 * @param time_start 开始时间戳
 * @param time_end 结束时间戳
 * @return 0成功，-1失败
 */
int divoom_stock_chart_set_time_range(divoom_stock_chart_t *chart, uint32_t time_start, uint32_t time_end, float max_price, float min_price);


/**
 * @brief 更新轴标签显示（Y轴最高/最低价格，X轴时间点）
 * @param chart 股票图表结构指针
 * @return 0成功，-1失败
 */
int divoom_stock_chart_update_axis_labels(divoom_stock_chart_t *chart, float max_price, float min_price);

/**
 * @brief 设置曲线颜色
 * @param chart 股票图表结构指针
 * @param r 红色分量 (0-255)
 * @param g 绿色分量 (0-255)
 * @param b 蓝色分量 (0-255)
 * @return 0成功，-1失败
 */
int divoom_stock_chart_set_line_color(divoom_stock_chart_t *chart, uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief 设置曲线颜色（十六进制格式）
 * @param chart 股票图表结构指针
 * @param color 十六进制颜色值，如 0xFF0000 表示红色
 * @return 0成功，-1失败
 */
int divoom_stock_chart_set_line_color_hex(divoom_stock_chart_t *chart, uint32_t color);

/**
 * @brief 设置 Glow 模式折线与填充渐变色（仅 style=GLOW 有效）
 */
int divoom_stock_chart_set_glow_colors(divoom_stock_chart_t *chart, uint32_t line_color, uint32_t fill_color);

#ifdef __cplusplus
}
#endif

#endif /* __DIVOOM_STOCK_CHART_H__ */

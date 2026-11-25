#include "divoom_stock_chart.h"
#include "divoom_menu_control.h"
#include "divoom.h"
#include "string.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include <time.h>

#define STOCK_CHART_DEFAULT_POINTS DIVOOM_CLOCK_CONTROL_HISOTYR_DAYS
#define STOCK_CHART_MAX_POINTS 200
#define STOCK_CHART_MAX_TIME_LABELS 8  /* 最大时间标签数量（减少显示密度） */
#define STOCK_CHART_AUTO_RANGE_MARGIN 0.01f  /* 自动范围边距1%，让最高最低点尽量接近顶部底部 */
#define STOCK_CHART_MIN_VOLATILITY_MULTIPLIER 1.2f  /* 小波动时的最小放大倍数（降低放大） */
#define STOCK_CHART_MAX_VOLATILITY_MULTIPLIER 2.0f  /* 极小波动时的最大放大倍数（降低放大） */
#define STOCK_CHART_SMALL_VOLATILITY_THRESHOLD 0.005f  /* 小波动阈值0.5%，只有极小波动才放大 */
#define STOCK_CHART_RANGE_UPDATE_THRESHOLD 0.15f  /* 范围更新阈值15%，超出此值才更新范围 */
#define STOCK_CHART_ENABLE_AMPLIFICATION 1  /* 是否启用放大机制：0=禁用（显示真实振幅），1=启用 */
#define STOCK_CHART_MIN_RANGE_PERCENT 0.05f  /* 最小范围百分比5%，确保即使价格接近也有显示范围，避免变成直线 */
#define STOCK_AMPLIFY_NUM	 100
#define STOCK_CHART_GLOW_MARGIN       2
#define STOCK_CHART_GLOW_FILL_OPA_TOP 140
#define STOCK_CHART_GLOW_START_OFFSET 1
#define STOCK_CHART_GLOW_RANGE_SCALE  1.1f

static int divoom_stock_chart_is_glow(Pdivoom_stock_chart_t chart)
{
    return chart != NULL && chart->style == DIVOOM_STOCK_CHART_STYLE_GLOW;
}

static lv_coord_t stock_chart_interp_y_at_x(lv_coord_t x, const lv_coord_t *xs, const lv_coord_t *ys, int count)
{
    int i;

    if (count <= 0)
    {
        return 0;
    }
    if (count == 1 || x <= xs[0])
    {
        return ys[0];
    }
    if (x >= xs[count - 1])
    {
        return ys[count - 1];
    }

    for (i = 0; i < count - 1; i++)
    {
        if (x >= xs[i] && x <= xs[i + 1])
        {
            if (xs[i + 1] == xs[i])
            {
                return ys[i];
            }
            return ys[i] + (lv_coord_t)((ys[i + 1] - ys[i]) * (x - xs[i]) / (xs[i + 1] - xs[i]));
        }
    }

    return ys[count - 1];
}

static void stock_chart_canvas_set_px(Pdivoom_stock_chart_t chart, lv_coord_t x, lv_coord_t y, uint32_t rgb, lv_opa_t opa)
{
    uint16_t *px;
    uint8_t r, g, b;

    if (chart == NULL || chart->canvas_buf == NULL || opa == LV_OPA_TRANSP)
    {
        return;
    }
    if (x < 0 || x >= chart->canvas_w || y < 0 || y >= chart->canvas_h)
    {
        return;
    }

    r = (uint8_t)((((rgb >> 16) & 0xFF) * opa) / 255);
    g = (uint8_t)((((rgb >> 8)  & 0xFF) * opa) / 255);
    b = (uint8_t)(((rgb         & 0xFF) * opa) / 255);

    px = ((uint16_t *)chart->canvas_buf) + (uint32_t)y * (uint32_t)chart->canvas_w + (uint32_t)x;
    *px = (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

static void stock_chart_draw_line_seg(lv_layer_t *layer, lv_point_t p1, lv_point_t p2,
                                      uint32_t color, lv_coord_t width, lv_opa_t opa)
{
    lv_draw_line_dsc_t line_dsc;

    lv_draw_line_dsc_init(&line_dsc);
    line_dsc.p1.x = p1.x;
    line_dsc.p1.y = p1.y;
    line_dsc.p2.x = p2.x;
    line_dsc.p2.y = p2.y;
    line_dsc.color = lv_color_hex(color);
    line_dsc.width = width;
    line_dsc.opa = opa;
    line_dsc.round_start = 0;
    line_dsc.round_end = 0;
    lv_draw_line(layer, &line_dsc);
}

static void stock_chart_draw_glow_lines(lv_obj_t *canvas, const lv_point_t *points, int count, uint32_t color)
{
    static const lv_coord_t glow_widths[] = {3, 2, 1};
    static const lv_opa_t glow_opas[] = {LV_OPA_20, LV_OPA_40, LV_OPA_COVER};
    lv_layer_t layer;
    int layer_idx, i;

    if (canvas == NULL || points == NULL || count < 2)
    {
        return;
    }

    lv_canvas_init_layer(canvas, &layer);
    for (layer_idx = 0; layer_idx < 3; layer_idx++)
    {
        for (i = 0; i < count - 1; i++)
        {
            stock_chart_draw_line_seg(&layer, points[i], points[i + 1], color,
                                      glow_widths[layer_idx], glow_opas[layer_idx]);
        }
    }
    lv_canvas_finish_layer(canvas, &layer);
}

static int divoom_stock_chart_render_glow(Pdivoom_stock_chart_t chart, const float *prices, int count)
{
    lv_coord_t xs[STOCK_CHART_DEFAULT_POINTS];
    lv_coord_t ys[STOCK_CHART_DEFAULT_POINTS];
    lv_point_t points[STOCK_CHART_DEFAULT_POINTS];
    float min_val, max_val, range;
    lv_coord_t w, h, plot_h, x, y, top_y;
    lv_coord_t grad_top;
    int grad_den;
    int i;

    if (chart == NULL || chart->canvas_obj == NULL || prices == NULL || count <= 0)
    {
        return -1;
    }

    w = chart->canvas_w;
    h = chart->canvas_h;
    if (w <= 0 || h <= 0)
    {
        return -1;
    }

    if (count > STOCK_CHART_DEFAULT_POINTS)
    {
        count = STOCK_CHART_DEFAULT_POINTS;
    }

    min_val = prices[0];
    max_val = prices[0];
    for (i = 1; i < count; i++)
    {
        if (prices[i] < min_val)
        {
            min_val = prices[i];
        }
        if (prices[i] > max_val)
        {
            max_val = prices[i];
        }
    }

    range = max_val - min_val;
    if (range < 0.0001f)
    {
        range = max_val * 0.001f;
        if (range < 0.0001f)
        {
            range = 1.0f;
        }
        min_val = max_val - range * 0.5f;
        max_val = max_val + range * 0.5f;
        range = max_val - min_val;
    }

    plot_h = h - STOCK_CHART_GLOW_MARGIN * 2;
    if (plot_h < 1)
    {
        plot_h = h;
    }

    for (i = 0; i < count; i++)
    {
        if (count > 1)
        {
            xs[i] = (lv_coord_t)(i * (w - 1) / (count - 1));
        }
        else
        {
            xs[i] = w / 2;
        }
        ys[i] = STOCK_CHART_GLOW_MARGIN + (lv_coord_t)(((max_val - prices[i]) / range) * plot_h);
        if (ys[i] < STOCK_CHART_GLOW_MARGIN)
        {
            ys[i] = STOCK_CHART_GLOW_MARGIN;
        }
        if (ys[i] > h - 1 - STOCK_CHART_GLOW_MARGIN)
        {
            ys[i] = h - 1 - STOCK_CHART_GLOW_MARGIN;
        }
        points[i].x = xs[i];
        points[i].y = ys[i];
    }

    lv_canvas_fill_bg(chart->canvas_obj, lv_color_black(), LV_OPA_COVER);

    grad_top = ys[0];
    for (i = 1; i < count; i++)
    {
        if (ys[i] < grad_top)
        {
            grad_top = ys[i];
        }
    }
    grad_top = (lv_coord_t)(grad_top + STOCK_CHART_GLOW_START_OFFSET);
    if (grad_top < 0)
    {
        grad_top = 0;
    }
    if (grad_top > h - 1)
    {
        grad_top = h - 1;
    }
    grad_den = (int)((h - 1 - grad_top) * STOCK_CHART_GLOW_RANGE_SCALE);
    if (grad_den < 1)
    {
        grad_den = 1;
    }

    for (x = 0; x < w; x++)
    {
        top_y = stock_chart_interp_y_at_x(x, xs, ys, count);
        if (top_y >= h - 1)
        {
            continue;
        }

        for (y = top_y; y < h; y++)
        {
            float t = (float)(y - grad_top) / (float)grad_den;
            lv_opa_t opa;

            if (t < 0.0f)
            {
                t = 0.0f;
            }
            if (t > 1.0f)
            {
                t = 1.0f;
            }
            opa = (lv_opa_t)(STOCK_CHART_GLOW_FILL_OPA_TOP * (1.0f - t));

            stock_chart_canvas_set_px(chart, x, y, chart->fill_color, opa);
        }
    }

    stock_chart_draw_glow_lines(chart->canvas_obj, points, count, chart->line_color);
    lv_obj_invalidate(chart->canvas_obj);
    return 0;
}

static int divoom_stock_chart_update_prices_glow(Pdivoom_stock_chart_t chart, float *prices, int count)
{
    int valid_count = 0;
    float valid_prices[STOCK_CHART_DEFAULT_POINTS];
    int i;

    if (chart == NULL || prices == NULL || count <= 0)
    {
        return -1;
    }

    if (count > STOCK_CHART_DEFAULT_POINTS)
    {
        count = STOCK_CHART_DEFAULT_POINTS;
    }

    for (i = 0; i < count; i++)
    {
        if (prices[i] != 0.0f)
        {
            valid_prices[valid_count] = prices[i];
            valid_count++;
        }
    }

    if (valid_count < 3)
    {
        if (valid_count == 0)
        {
            divoom_debug("Glow chart: no valid data points\n");
        }
        else
        {
            divoom_debug("Glow chart: only 1 valid point, keep previous curve\n");
        }
        return -1;
    }

    for (i = 0; i < valid_count; i++)
    {
        chart->price_data[i] = valid_prices[i];
    }

    chart->data_count = valid_count;
    chart->actual_min_price = valid_prices[0];
    chart->actual_max_price = valid_prices[0];
    for (i = 1; i < valid_count; i++)
    {
        if (valid_prices[i] < chart->actual_min_price)
        {
            chart->actual_min_price = valid_prices[i];
        }
        if (valid_prices[i] > chart->actual_max_price)
        {
            chart->actual_max_price = valid_prices[i];
        }
    }

    chart->current_price = valid_prices[valid_count - 1];
    chart->prev_price = valid_count > 1 ? valid_prices[valid_count - 2] : valid_prices[valid_count - 1];

    return divoom_stock_chart_render_glow(chart, valid_prices, valid_count);
}

/**
 * @brief 计算价格数组的最小值和最大值
 */
static void calculate_price_range(float *prices, int count, float *min_price, float *max_price)
{
    if (prices == NULL || count <= 0) 
	{
        *min_price = 0;
        *max_price = 100;
        return;
    }

    *min_price = prices[0];
    *max_price = prices[0];

    for (int i = 1; i < count; i++) 
	{
        if (prices[i] < *min_price) 
		{
            *min_price = prices[i];
        }
        if (prices[i] > *max_price) 
		{
            *max_price = prices[i];
        }
    }

    /* 计算实际价格范围 */
    float range = *max_price - *min_price;
    
    /* 计算平均价格，用于判断波动大小 */
    float avg_price = (*min_price + *max_price) / 2.0f;
    if (avg_price < 0.01f) 
	{
        avg_price = (*max_price > 0.01f) ? *max_price : 100.0f;
    }
    
    /* 计算波动百分比 */
    float volatility_percent = (avg_price > 0.01f) ? (range / avg_price) : 0.0f;
    
    /* 直接使用实际最高和最低价格作为Y轴范围 */
    /* 最高点显示在图表顶部，最低点显示在图表底部 */
    /* 不需要任何额外的边距或放大 */
    
    /* 确保最小值不为负 */
    if (*min_price < 0) 
	{
        float offset = -*min_price;
        *max_price += offset;
        *min_price = 0;
    }
}


/**
 * @brief 格式化时间戳为字符串（HH:MM格式）
 * 支持两种时间戳格式：
 * 1. Unix时间戳（从1970年1月1日开始的秒数）
 * 2. 从当天0点开始的秒数（例如：9:30 = 9*3600 + 30*60 = 34200秒）
 */
static void format_time_string(uint32_t timestamp, char *buffer, int buffer_size)
{
    if (timestamp == 0) {
        snprintf(buffer, buffer_size, "--:--");
        return;
    }
    
    int hour = 0, minute = 0;
    
    /* 判断时间戳格式：
     * 如果时间戳小于86400（一天的秒数），认为是当天0点开始的秒数
     * 否则认为是Unix时间戳
     */
    if (timestamp < 86400) {
        /* 从当天0点开始的秒数格式 */
        hour = timestamp / 3600;
        minute = (timestamp % 3600) / 60;
    } else {
        /* Unix时间戳格式，尝试使用localtime转换 */
        time_t t = (time_t)timestamp;
        struct tm *timeinfo = localtime(&t);
        if (timeinfo != NULL) {
            hour = timeinfo->tm_hour;
            minute = timeinfo->tm_min;
        } else {
            /* 如果localtime失败，尝试直接计算（假设是Unix时间戳） */
            /* 计算从1970年1月1日0点开始的小时数 */
            uint32_t hours_since_epoch = timestamp / 3600;
            /* 计算当前小时（UTC+8，可以根据需要调整） */
            hour = (hours_since_epoch + 8) % 24;  /* +8是时区偏移，可根据需要调整 */
            minute = (timestamp % 3600) / 60;
        }
    }
    
    /* 确保小时和分钟在有效范围内 */
    if (hour < 0 || hour > 23) hour = 0;
    if (minute < 0 || minute > 59) minute = 0;
    
    snprintf(buffer, buffer_size, "%02d:%02d", hour, minute);
}

/**
 * @brief 格式化价格为字符串
 */
static void format_price_string(float price, char *buffer, int buffer_size)
{
    if (price < 0.01f) {
        snprintf(buffer, buffer_size, "0.00");
    } else if (price < 1.0f) {
        snprintf(buffer, buffer_size, "%.3f", price);
    } else if (price < 100.0f) {
        snprintf(buffer, buffer_size, "%.2f", price);
    } else {
        snprintf(buffer, buffer_size, "%.1f", price);
    }
}

/**
 * @brief 根据时间范围自动计算合适的时间间隔（秒）
 * @param time_span 时间范围（秒）
 * @param max_labels 最大标签数量
 * @return 计算出的时间间隔（秒）
 */
static uint32_t calculate_optimal_time_interval(uint32_t time_span, int max_labels)
{
    if (time_span == 0 || max_labels <= 1) {
        return 3600;  /* 默认1小时 */
    }
    
    /* 计算最小需要的间隔（秒） */
    uint32_t min_interval_seconds = time_span / max_labels;
    if (min_interval_seconds == 0) {
        min_interval_seconds = 1;
    }
    
    /* 定义常见的时间间隔（秒）：5分钟、10分钟、15分钟、30分钟、1小时、2小时、4小时、6小时、12小时、24小时 */
    uint32_t common_intervals[] = {
        300,      /* 5分钟 */
        600,      /* 10分钟 */
        900,      /* 15分钟 */
        1800,     /* 30分钟 */
        3600,     /* 1小时 */
        7200,     /* 2小时 */
        14400,    /* 4小时 */
        21600,    /* 6小时 */
        43200,    /* 12小时 */
        86400     /* 24小时 */
    };
    int interval_count = sizeof(common_intervals) / sizeof(common_intervals[0]);
    
    /* 找到大于等于最小间隔的第一个常见间隔 */
    for (int i = 0; i < interval_count; i++) {
        if (common_intervals[i] >= min_interval_seconds) {
            return common_intervals[i];
        }
    }
    
    /* 如果所有常见间隔都太小，使用最后一个并逐步翻倍 */
    uint32_t interval = common_intervals[interval_count - 1];
    while (interval < min_interval_seconds && interval < time_span) {
        interval *= 2;
    }
    
    return interval;
}

Pdivoom_stock_chart_t divoom_stock_chart_create(lv_obj_t *parent, lv_coord_t x, lv_coord_t y, 
                                                 lv_coord_t width, lv_coord_t height, 
                                                 const char *stock_name, uint8_t style)
{

    /* 分配内存 */
    Pdivoom_stock_chart_t chart = (Pdivoom_stock_chart_t)divoom_system_malloc(sizeof(divoom_stock_chart_t));
    if (chart == NULL) {
        return NULL;
    }
    memset(chart, 0, sizeof(divoom_stock_chart_t));

    chart->style = style;
    chart->line_color = DIVOOM_STOCK_CHART_GLOW_LINE_COLOR;
    chart->fill_color = DIVOOM_STOCK_CHART_GLOW_FILL_COLOR;

    /* 分配价格数据数组 */
    chart->price_data = (float *)divoom_system_malloc(sizeof(float) * STOCK_CHART_DEFAULT_POINTS);
    if (chart->price_data == NULL) {
        divoom_system_free(chart);
        return NULL;
    }
    memset(chart->price_data, 0, sizeof(float) * STOCK_CHART_DEFAULT_POINTS);

    if (style == DIVOOM_STOCK_CHART_STYLE_GLOW)
    {
        uint32_t buf_size = (uint32_t)width * (uint32_t)height * 2u;

        chart->canvas_w = width;
        chart->canvas_h = height;
        chart->canvas_buf = (uint8_t *)divoom_system_malloc(buf_size);
        if (chart->canvas_buf == NULL)
        {
            divoom_system_free(chart->price_data);
            divoom_system_free(chart);
            return NULL;
        }

        chart->canvas_obj = lv_canvas_create(parent);
        if (chart->canvas_obj == NULL)
        {
            divoom_system_free(chart->canvas_buf);
            divoom_system_free(chart->price_data);
            divoom_system_free(chart);
            return NULL;
        }

        lv_canvas_set_buffer(chart->canvas_obj, chart->canvas_buf, width, height, LV_COLOR_FORMAT_RGB565);
        lv_obj_set_size(chart->canvas_obj, width, height);
        lv_obj_set_pos(chart->canvas_obj, x, y);
        lv_obj_clear_flag(chart->canvas_obj, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_img_opa(chart->canvas_obj, LV_OPA_COVER, 0);
        lv_canvas_fill_bg(chart->canvas_obj, lv_color_black(), LV_OPA_COVER);
        return chart;
    }

    /* 创建图表对象 */
    chart->chart_obj = lv_chart_create(parent);
    if (chart->chart_obj == NULL) {
        divoom_system_free(chart->price_data);
        divoom_system_free(chart);
        return NULL;
    }

    /* 设置图表位置和大小 */
    lv_obj_set_pos(chart->chart_obj, x, y);
    lv_obj_set_size(chart->chart_obj, width, height);

    /* 设置图表类型为折线图 */
    lv_chart_set_type(chart->chart_obj, LV_CHART_TYPE_LINE);

    /* 设置数据点数量 */
    lv_chart_set_point_count(chart->chart_obj, STOCK_CHART_DEFAULT_POINTS);

    /* 设置网格线 */
    lv_chart_set_div_line_count(chart->chart_obj, 6, 6);

    /* 设置更新模式为循环模式（适合实时数据） */
    lv_chart_set_update_mode(chart->chart_obj, LV_CHART_UPDATE_MODE_CIRCULAR);

    /* 添加价格曲线系列（使用绿色表示上涨，红色表示下跌） */
    chart->price_series = lv_chart_add_series(chart->chart_obj, lv_color_hex(0x00FF00), LV_CHART_AXIS_PRIMARY_Y);
    
    /* 隐藏数据点（圆点）：通过设置点的半径为0 */
    if (chart->price_series != NULL) {
        /* 设置数据点的半径为0，隐藏圆点 */
        lv_obj_set_style_radius(chart->chart_obj, 0, LV_PART_INDICATOR);
        /* 或者尝试设置点的宽度和高度为0 */
        lv_obj_set_style_width(chart->chart_obj, 0, LV_PART_INDICATOR);
        lv_obj_set_style_height(chart->chart_obj, 0, LV_PART_INDICATOR);
    }

    /* 设置初始价格范围 */
    chart->min_price = 0;
    chart->max_price = 100;
    chart->actual_min_price = 0;
    chart->actual_max_price = 0;
    chart->data_count = 0;
    chart->current_price = 0;
    chart->prev_price = 0;
    chart->time_period_mode = 0;  /* 默认普通模式 */
    chart->last_period_end_price = 0;
    chart->time_start = 0;
    chart->time_end = 0;
    chart->time_interval_minutes = 30;  /* 默认30分钟间隔 */
    chart->time_labels = NULL;
    chart->time_label_count = 0;

    /* 初始化标签指针 */
    chart->label_max_price = NULL;
    chart->label_min_price = NULL;
    chart->label_time_start = NULL;
    chart->label_time_end = NULL;
    
    /* 分配时间标签数组 */
    if (parent != NULL) {
        chart->time_labels = (lv_obj_t **)divoom_system_malloc(sizeof(lv_obj_t *) * STOCK_CHART_MAX_TIME_LABELS);
        if (chart->time_labels != NULL) {
            memset(chart->time_labels, 0, sizeof(lv_obj_t *) * STOCK_CHART_MAX_TIME_LABELS);
        }
    }

    /* 创建Y轴价格标签 */
    if (parent != NULL) {
        /* 最高价格标签（右侧顶部，图表内部） */
        chart->label_max_price = lv_label_create(parent);
        lv_obj_set_style_text_color(chart->label_max_price, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        lv_obj_set_style_text_font(chart->label_max_price, divoom_menu_control_get_font_info(14, 0), LV_PART_MAIN);
        lv_label_set_text(chart->label_max_price, "Max: --");
        lv_obj_set_pos(chart->label_max_price, x + width + 5, y + 5);
        
        /* 最低价格标签（右侧底部，图表内部，但留出时间标签空间） */
        chart->label_min_price = lv_label_create(parent);
        lv_obj_set_style_text_color(chart->label_min_price, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        lv_obj_set_style_text_font(chart->label_min_price, divoom_menu_control_get_font_info(14, 0), LV_PART_MAIN);
        lv_label_set_text(chart->label_min_price, "Min: --");
        lv_obj_set_pos(chart->label_min_price, x + width + 5, y + height - 25);
        
        /* X轴开始时间标签（左下角，图表下方） */
        chart->label_time_start = lv_label_create(parent);
        lv_obj_set_style_text_color(chart->label_time_start, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        lv_obj_set_style_text_font(chart->label_time_start, divoom_menu_control_get_font_info(14, 0), LV_PART_MAIN);
        lv_label_set_text(chart->label_time_start, "--:--");
        lv_obj_set_pos(chart->label_time_start, x + 5, y + height + 2);
        
        /* X轴结束时间标签（右下角，图表下方） */
        chart->label_time_end = lv_label_create(parent);
        lv_obj_set_style_text_color(chart->label_time_end, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        lv_obj_set_style_text_font(chart->label_time_end, divoom_menu_control_get_font_info(14, 0), LV_PART_MAIN);
        lv_label_set_text(chart->label_time_end, "--:--");
        lv_obj_set_pos(chart->label_time_end, x + width - 50, y + height + 2);
    }

    /* 设置默认样式 */
    lv_obj_set_style_bg_color(chart->chart_obj, lv_color_hex(0x1C1C1E), LV_PART_MAIN);
    lv_obj_set_style_border_color(chart->chart_obj, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_line_color(chart->chart_obj, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_line_width(chart->chart_obj, 1, LV_PART_MAIN);

    /* 设置曲线样式 */
    lv_obj_set_style_line_width(chart->chart_obj, 3, LV_PART_ITEMS);
    lv_obj_set_style_line_color(chart->chart_obj, lv_color_hex(0x00FF00), LV_PART_ITEMS);

    return chart;
}

int divoom_stock_chart_destroy(divoom_stock_chart_t *chart)
{
    if (chart == NULL) 
	{
        return -1;
    }

	if (chart->canvas_obj != NULL)
	{
		lv_obj_del(chart->canvas_obj);
		chart->canvas_obj = NULL;
	}

	if (chart->canvas_buf != NULL)
	{
		divoom_system_free(chart->canvas_buf);
		chart->canvas_buf = NULL;
	}

	/* 先移除系列，再删除图表对象 */
	if (chart->chart_obj != NULL)
	{
		/* 移除价格系列 */
		if (chart->price_series != NULL)
		{
			lv_chart_remove_series(chart->chart_obj, chart->price_series);
			chart->price_series = NULL;
		}
		
		/* 移除成交量系列（如果存在） */
		if (chart->volume_series != NULL)
		{
			lv_chart_remove_series(chart->chart_obj, chart->volume_series);
			chart->volume_series = NULL;
		}
		
		/* 删除图表对象 */
		lv_obj_del(chart->chart_obj);
		chart->chart_obj = NULL;
	}

    /* 删除标签对象 */
    if (chart->label_max_price != NULL) {
        lv_obj_del(chart->label_max_price);
        chart->label_max_price = NULL;
    }
    if (chart->label_min_price != NULL) {
        lv_obj_del(chart->label_min_price);
        chart->label_min_price = NULL;
    }
    if (chart->label_time_start != NULL) {
        lv_obj_del(chart->label_time_start);
        chart->label_time_start = NULL;
    }
    if (chart->label_time_end != NULL) {
        lv_obj_del(chart->label_time_end);
        chart->label_time_end = NULL;
    }
    
    /* 删除所有时间标签 */
    if (chart->time_labels != NULL) {
        for (int i = 0; i < STOCK_CHART_MAX_TIME_LABELS; i++) {
            if (chart->time_labels[i] != NULL) {
                lv_obj_del(chart->time_labels[i]);
                chart->time_labels[i] = NULL;
            }
        }
        divoom_system_free(chart->time_labels);
        chart->time_labels = NULL;
    }
    chart->time_label_count = 0;

    /* 释放价格数据数组 */
    if (chart->price_data != NULL) 
	{
        divoom_system_free(chart->price_data);
        chart->price_data = NULL;
    }

    /* 释放成交量数据数组 */
    if (chart->volume_data != NULL) 
	{
        divoom_system_free(chart->volume_data);
        chart->volume_data = NULL;
    }

    /* 释放结构体 */
    divoom_system_free(chart);
    return 0;
}

int divoom_stock_chart_set_time_range(divoom_stock_chart_t *chart, uint32_t time_start, uint32_t time_end, float max_price, float min_price)
{
    if (chart == NULL) {
        return -1;
    }

    if (divoom_stock_chart_is_glow(chart))
    {
        return 0;
    }
    
    chart->time_start = time_start;
    chart->time_end = time_end;
    
    /* 调试信息 */
    divoom_debug("Set time range: start=%u, end=%u, span=%u seconds\n", time_start, time_end, (time_end > time_start) ? (time_end - time_start) : 0);
    /* 更新标签显示 */
    divoom_stock_chart_update_axis_labels(chart, max_price, min_price);
    return 0;
}

int divoom_stock_chart_update_axis_labels(divoom_stock_chart_t *chart, float max_price, float min_price)
{
    if (chart == NULL || chart->chart_obj == NULL) {
        return -1;
    }

    if (divoom_stock_chart_is_glow(chart))
    {
        return 0;
    }
    
    char buffer[32];
    lv_obj_t *parent = lv_obj_get_parent(chart->chart_obj);
    
    /* 获取图表位置和大小，用于更新标签位置 */
    lv_coord_t chart_x, chart_y, chart_width, chart_height;
    chart_x = lv_obj_get_x(chart->chart_obj);
    chart_y = lv_obj_get_y(chart->chart_obj);
    chart_width = lv_obj_get_width(chart->chart_obj);
    chart_height = lv_obj_get_height(chart->chart_obj);
    
    /* 更新Y轴最高价格标签（显示实际数据中的最高价格） */
    if (chart->label_max_price != NULL) {
        float display_max = (chart->actual_max_price > 0) ? chart->actual_max_price : chart->max_price;
        format_price_string(display_max, buffer, sizeof(buffer));
        char text[64];
        snprintf(text, sizeof(text), "Max: %f", max_price);
        lv_label_set_text(chart->label_max_price, text);
        /* 更新位置：图表右侧顶部 */
        lv_obj_set_pos(chart->label_max_price, chart_x + chart_width + 5, chart_y + 5);
    }
    
    /* 更新Y轴最低价格标签（显示实际数据中的最低价格） */
    if (chart->label_min_price != NULL) {
        float display_min = (chart->actual_min_price > 0) ? chart->actual_min_price : chart->min_price;
        format_price_string(display_min, buffer, sizeof(buffer));
        char text[64];
        snprintf(text, sizeof(text), "Min: %f", min_price);
        lv_label_set_text(chart->label_min_price, text);
        /* 更新位置：图表右侧底部，留出时间标签空间 */
        lv_obj_set_pos(chart->label_min_price, chart_x + chart_width + 5, chart_y + chart_height - 25);
    }

    /* 更新X轴时间标签 */
    if (parent != NULL && chart->time_labels != NULL) {
        
        /* 如果时间范围无效，只显示开始和结束时间 */
        if (chart->time_start == 0 || chart->time_end == 0 || 
            chart->time_end <= chart->time_start) {
            /* 更新开始时间标签 */
            if (chart->label_time_start != NULL) {
                format_time_string(chart->time_start, buffer, sizeof(buffer));
                lv_label_set_text(chart->label_time_start, buffer);
                /* 更新位置：图表左下角下方 */
                lv_obj_set_pos(chart->label_time_start, chart_x + 5, chart_y + chart_height + 2);
            }
            
            /* 更新结束时间标签 */
            if (chart->label_time_end != NULL) {
                format_time_string(chart->time_end, buffer, sizeof(buffer));
                lv_label_set_text(chart->label_time_end, buffer);
                /* 更新位置：图表右下角下方 */
                lv_obj_set_pos(chart->label_time_end, chart_x + chart_width - 50, chart_y + chart_height + 2);
            }
            
            /* 隐藏所有中间时间标签 */
            for (int i = 0; i < chart->time_label_count; i++) {
                if (chart->time_labels[i] != NULL) {
                    lv_obj_add_flag(chart->time_labels[i], LV_OBJ_FLAG_HIDDEN);
                }
            }
            return 0;
        }
        
        /* 计算时间范围（秒） */
        uint32_t time_span = chart->time_end - chart->time_start;
        
        /* 根据图表宽度智能计算最小间隔（每个标签至少需要60像素宽度） */
        lv_coord_t min_label_width = 60;
        int max_labels_by_width = (chart_width / min_label_width);
        int max_labels = (max_labels_by_width < STOCK_CHART_MAX_TIME_LABELS) ? 
                         max_labels_by_width : STOCK_CHART_MAX_TIME_LABELS;
        
        /* 根据时间范围和最大标签数量自动计算最合适的时间间隔 */
        uint32_t interval_seconds = calculate_optimal_time_interval(time_span, max_labels);
        
        /* 计算需要显示的时间点数量 */
        int time_point_count = (time_span / interval_seconds) + 1;  /* +1 包括结束点 */
        
        /* 确保不超过最大标签数量（如果仍然太多，进一步增加间隔） */
        while (time_point_count > max_labels && interval_seconds < time_span) {
            interval_seconds *= 2;  /* 翻倍间隔 */
            time_point_count = (time_span / interval_seconds) + 1;
        }
        
        /* 确保不超过最大标签数量 */
        if (time_point_count > max_labels) {
            time_point_count = max_labels;
        }
        
        /* 创建或更新时间标签 */
        for (int i = 0; i < time_point_count; i++) {
            /* 计算当前时间点的时间戳 */
            uint32_t current_time = chart->time_start + (i * interval_seconds);
            if (current_time > chart->time_end) {
                current_time = chart->time_end;
            }
            
            /* 计算标签位置（X轴均匀分布） */
            float ratio = (time_span > 0) ? ((float)(current_time - chart->time_start) / (float)time_span) : 0.0f;
            if (ratio > 1.0f) ratio = 1.0f;
            lv_coord_t label_x = chart_x + (lv_coord_t)(ratio * chart_width);
            
            /* 创建标签（如果不存在） */
            if (chart->time_labels[i] == NULL) {
                chart->time_labels[i] = lv_label_create(parent);
                if (chart->time_labels[i] != NULL) {
                    lv_obj_set_style_text_color(chart->time_labels[i], lv_color_hex(0xFFFFFF), LV_PART_MAIN);
                    lv_obj_set_style_text_font(chart->time_labels[i], divoom_menu_control_get_font_info(14, 0), LV_PART_MAIN);
                }
            }
            
            /* 更新标签文本和位置 */
            if (chart->time_labels[i] != NULL) {
                format_time_string(current_time, buffer, sizeof(buffer));
                lv_label_set_text(chart->time_labels[i], buffer);
                /* 获取标签宽度，用于居中显示 */
                lv_obj_update_layout(chart->time_labels[i]);
                lv_coord_t label_width = lv_obj_get_width(chart->time_labels[i]);
                lv_coord_t label_y = chart_y + chart_height + 2;
                
                /* 计算标签X位置，确保不超出图表边界 */
                lv_coord_t label_x_pos = label_x - label_width / 2;
                
                /* 第一个标签（开始时间）：左对齐，不居中，避免超出左边界 */
                if (i == 0) {
                    label_x_pos = chart_x + 5;
                }
                /* 最后一个标签（结束时间）：右对齐，不居中，避免超出右边界 */
                else if (i == time_point_count - 1) {
                    label_x_pos = chart_x + chart_width - label_width - 5;
                }
                /* 中间标签：居中显示 */
                else {
                    /* 确保标签不超出左边界 */
                    if (label_x_pos < chart_x + 5) {
                        label_x_pos = chart_x + 5;
                    }
                    /* 确保标签不超出右边界 */
                    else if (label_x_pos + label_width > chart_x + chart_width - 5) {
                        label_x_pos = chart_x + chart_width - label_width - 5;
                    }
                }
                
                lv_obj_set_pos(chart->time_labels[i], label_x_pos, label_y);
                lv_obj_clear_flag(chart->time_labels[i], LV_OBJ_FLAG_HIDDEN);
                
                /* 调试信息：显示前3个和后3个标签的信息 */
                if (i < 3 || i >= time_point_count - 3) 
				{
                	divoom_debug("Time label[%d/%d]: timestamp=%u, text=%s, pos=(%d,%d), width=%d\n", 
                	            i, time_point_count, current_time, buffer, 
                                label_x_pos, label_y, label_width);
                }
            }
        }
        
        /* 更新标签数量 */
        chart->time_label_count = time_point_count;
        
        /* 隐藏多余的时间标签 */
        for (int i = time_point_count; i < STOCK_CHART_MAX_TIME_LABELS; i++) {
            if (chart->time_labels[i] != NULL) {
                lv_obj_add_flag(chart->time_labels[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
        
        /* 当显示多个时间标签时，隐藏独立的开始和结束标签，避免与循环中的首尾标签重叠 */
        if (chart->label_time_start != NULL) {
            lv_obj_add_flag(chart->label_time_start, LV_OBJ_FLAG_HIDDEN);
        }
        if (chart->label_time_end != NULL) {
            lv_obj_add_flag(chart->label_time_end, LV_OBJ_FLAG_HIDDEN);
        }
    } else {
        /* 如果没有时间标签数组，使用旧的开始和结束标签 */
        if (chart->label_time_start != NULL) {
            format_time_string(chart->time_start, buffer, sizeof(buffer));
            lv_label_set_text(chart->label_time_start, buffer);
        }
        if (chart->label_time_end != NULL) {
            format_time_string(chart->time_end, buffer, sizeof(buffer));
            lv_label_set_text(chart->label_time_end, buffer);
        }
    }
    
    return 0;
}

int divoom_stock_chart_update_prices(divoom_stock_chart_t *chart, float *prices, int *time_stamp, int count, float max, float min)
{
	int amp_times = 100; //放大倍数

    if (chart == NULL || prices == NULL || count <= 0) 
	{
        return -1;
    }

    if (divoom_stock_chart_is_glow(chart))
    {
        return divoom_stock_chart_update_prices_glow(chart, prices, count);
    }

	for (int i = 0; i < count; i++)
	{
//		divoom_debug("[%d]: %f ", i, prices[i]);
	}

    /* 限制数据点数量 */
    if (count > STOCK_CHART_DEFAULT_POINTS) 
	{
        count = STOCK_CHART_DEFAULT_POINTS;
    }

    /* 统计非零点的数量，并创建非零点数组 */
    int valid_count = 0;
    float valid_prices[STOCK_CHART_DEFAULT_POINTS];
    
	if (max - min < 10)
	{
		amp_times = 1000;
	}

    for (int i = 0; i < count; i++) 
	{
        if (prices[i] != 0.0f) 
		{
            valid_prices[valid_count] = prices[i]*amp_times;
            valid_count++;
        }
    }
    
    /* 如果没有有效数据点，返回错误 */
    if (valid_count == 0) 
	{
        divoom_debug("No valid (non-zero) data points found\n");
        return -1;
    }
    
    divoom_debug("Filtered: %d valid points from %d total points\n", valid_count, count);

    /* 计算实际数据的最高和最低价格（不包含边距） */
    float actual_min = valid_prices[0];
    float actual_max = valid_prices[0];
    for (int i = 1; i < valid_count; i++) 
	{
        if (valid_prices[i] < actual_min) 
		{
            actual_min = valid_prices[i];
        }
        if (valid_prices[i] > actual_max) 
		{
            actual_max = valid_prices[i];
        }
    }
    chart->actual_min_price = actual_min/amp_times;
    chart->actual_max_price = actual_max/amp_times;

    /* 计算价格范围（只使用非零点，包含边距用于Y轴显示） */
    float min_price, max_price;
    calculate_price_range(valid_prices, valid_count, &min_price, &max_price);
    
    /* 先获取旧的数据点数量，用于清空旧数据 */
    uint16_t old_point_count = lv_chart_get_point_count(chart->chart_obj);
    
    /* 更新数据点数量为实际非零点数量 */
    if (valid_count != chart->data_count) 
	{
        lv_chart_set_point_count(chart->chart_obj, valid_count);
        chart->data_count = valid_count;
    }
    
    /* 最彻底的方法：先隐藏图表，然后清空所有可能的数据点 */
//    lv_obj_add_flag(chart->chart_obj, LV_OBJ_FLAG_HIDDEN);
    
    /* 先更新数据点数量为最大值（旧数量和新数量中的较大值），确保能清空所有旧数据 */
    uint16_t max_point_count = (old_point_count > valid_count) ? old_point_count : valid_count;
    if (max_point_count > 0) 
	{
        lv_chart_set_point_count(chart->chart_obj, max_point_count);
        /* 使用lv_chart_set_all_value清空所有数据点 */
        if (chart->price_series != NULL) 
		{
            /* 使用一个明显超出范围的值来清空，确保旧数据不会显示 */
            lv_coord_t clear_value = 0;//(lv_coord_t)(min_price - (max_price - min_price) * 100.0f);
            lv_chart_set_all_value(chart->chart_obj, chart->price_series, clear_value);
            lv_chart_refresh(chart->chart_obj);
        }
    }
    
    /* 然后设置正确的数据点数量 */
    lv_chart_set_point_count(chart->chart_obj, valid_count);
    chart->data_count = valid_count;
    lv_chart_refresh(chart->chart_obj);
    
    /* 只有当新范围与旧范围差异超过阈值时才更新，避免频繁跳动 */
    int need_update_range = 0;
    if (chart->data_count > 0 && chart->max_price > chart->min_price) 
	{
        float old_range = chart->max_price - chart->min_price;
        float new_range = max_price - min_price;
        float range_change = (new_range > old_range) ? 
            (new_range - old_range) / old_range : (old_range - new_range) / old_range;
        
        float min_change = (min_price < chart->min_price) ? 
            (chart->min_price - min_price) / old_range : (min_price - chart->min_price) / old_range;
        float max_change = (max_price > chart->max_price) ? 
            (max_price - chart->max_price) / old_range : (chart->max_price - max_price) / old_range;
        
        /* 只有当范围变化或边界变化超过阈值时才更新 */
        if (range_change > STOCK_CHART_RANGE_UPDATE_THRESHOLD || 
            min_change > STOCK_CHART_RANGE_UPDATE_THRESHOLD ||
            max_change > STOCK_CHART_RANGE_UPDATE_THRESHOLD) 
		{
            need_update_range = 1;
        }
    } 
	else 
	{
        /* 首次更新或范围无效，直接设置 */
        need_update_range = 1;
    }
    
    /* 先更新价格范围（必须在清空数据之前更新） */
    if (need_update_range) 
	{
        chart->min_price = min_price;
        chart->max_price = max_price;
        lv_chart_set_range(chart->chart_obj, LV_CHART_AXIS_PRIMARY_Y, (lv_coord_t)min_price, (lv_coord_t)max_price);
    } 
	else 
	{
        /* 即使不需要更新范围，也要更新chart中的值，用于后续清空数据 */
        chart->min_price = min_price;
        chart->max_price = max_price;
    }

//	divoom_debug("wft test min_price: %f, max_price: %f\n", chart->min_price, chart->max_price);

    /* 切换到SHIFT模式，确保数据从索引0开始显示 */
    lv_chart_set_update_mode(chart->chart_obj, LV_CHART_UPDATE_MODE_SHIFT);
    
    /* 强制刷新，确保数据点数量变化生效 */
    lv_chart_refresh(chart->chart_obj);
    lv_obj_invalidate(chart->chart_obj);
    
    /* 等待一下，确保LVGL内部状态更新完成 */
    /* 注意：这里不能使用延时，但可以通过多次刷新来确保状态更新 */
    lv_chart_refresh(chart->chart_obj);
    
    /* 方法1：直接操作系列的数据数组（最可靠的方法，参考测试代码） */
    if (chart->price_series != NULL) 
	{
        /* 此时数据点数量已经设置为valid_count，所以y_points数组的大小就是valid_count */
        /* 直接设置新的数据点（只使用非零点） */
        for (int i = 0; i < valid_count; i++) 
        {
            /* 保存价格数据 */
            if (i < STOCK_CHART_DEFAULT_POINTS) 
			{
                chart->price_data[i] = valid_prices[i];
            }
            
            /* 确保价格值在范围内，然后直接赋值到y_points数组 */
            float price = valid_prices[i];
            if (price < chart->min_price) 
				price = chart->min_price;
            if (price > chart->max_price) 
				price = chart->max_price;
            lv_chart_set_series_value_by_id(chart->chart_obj, chart->price_series, (uint32_t)i,
                                            (int32_t)price);
        }
        
        /* 设置完数据后立即刷新 */
        lv_chart_refresh(chart->chart_obj);
        
    } 
    
    /* 多次强制刷新，确保数据更新生效 */
    lv_chart_refresh(chart->chart_obj);
    lv_obj_invalidate(chart->chart_obj);
    
    /* 再次刷新，确保所有缓存清除 */
    lv_chart_refresh(chart->chart_obj);
    
    /* 显示图表 */
    lv_obj_clear_flag(chart->chart_obj, LV_OBJ_FLAG_HIDDEN);
    
    /* 显示后再次刷新和无效化，确保立即显示新数据 */
    lv_chart_refresh(chart->chart_obj);
    lv_obj_invalidate(chart->chart_obj);
    
    /* 保持SHIFT模式，这样数据会从左侧开始显示 */
    /* 如果需要实时更新，可以在外部调用时切换回CIRCULAR模式 */

    /* 保存当前价格和前一个价格（使用非零点） */
    if (valid_count > 0) 
	{
        chart->current_price = valid_prices[valid_count - 1];
        if (valid_count > 1) 
		{
            chart->prev_price = valid_prices[valid_count - 2];
        }
    }

    /* 根据涨跌设置曲线颜色 */
    if (chart->current_price >= chart->prev_price) 
	{
        lv_obj_set_style_line_color(chart->chart_obj, lv_color_hex(0x00FF00), LV_PART_ITEMS); /* 绿色上涨 */
    } 
	else 
	{
        lv_obj_set_style_line_color(chart->chart_obj, lv_color_hex(0xFF0000), LV_PART_ITEMS); /* 红色下跌 */
    }

    /* 强制刷新图表 */
    lv_chart_refresh(chart->chart_obj);
    lv_obj_invalidate(chart->chart_obj);

    /* 更新轴标签 */
	divoom_stock_chart_set_time_range(chart, time_stamp[0], time_stamp[valid_count -1], max, min);

    return 0;
}

int divoom_stock_chart_set_glow_colors(divoom_stock_chart_t *chart, uint32_t line_color, uint32_t fill_color)
{
    float valid_prices[STOCK_CHART_DEFAULT_POINTS];
    int i;

    if (chart == NULL || !divoom_stock_chart_is_glow(chart))
    {
        return -1;
    }

    chart->line_color = line_color;
    chart->fill_color = fill_color;

    if (chart->data_count <= 0)
    {
        return 0;
    }

    for (i = 0; i < chart->data_count; i++)
    {
        valid_prices[i] = chart->price_data[i];
    }

    return divoom_stock_chart_render_glow(chart, valid_prices, chart->data_count);
}

int divoom_stock_chart_set_line_color_hex(divoom_stock_chart_t *chart, uint32_t color)
{
    if (chart == NULL) {
        return -1;
    }

    if (divoom_stock_chart_is_glow(chart))
    {
        return divoom_stock_chart_set_glow_colors(chart, color, chart->fill_color);
    }

    if (chart->chart_obj == NULL) {
        return -1;
    }
    
    /* 设置曲线颜色 */
    lv_obj_set_style_line_color(chart->chart_obj, lv_color_hex(color), LV_PART_ITEMS);
    
    /* 同时更新系列的颜色 */
    if (chart->price_series != NULL) {
        lv_chart_set_series_color(chart->chart_obj, chart->price_series, lv_color_hex(color));
    }
    
    /* 刷新图表 */
    lv_chart_refresh(chart->chart_obj);
    lv_obj_invalidate(chart->chart_obj);
    
    return 0;
}


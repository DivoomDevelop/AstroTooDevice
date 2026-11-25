#ifndef __DIVOOM_MARBLE_PATH_H__
#define __DIVOOM_MARBLE_PATH_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "divoom_lvgl9_compat.h"

/*======================= 常量定义 =======================*/
#define PINBALL_BALL_SIZE           40      /**< 弹珠直径 (像素) */
#define PINBALL_BALL_RADIUS         20      /**< 弹珠半径 (像素) */

#define PINBALL_GRAVITY             0.35f   /**< 重力加速度 - 让下落更真实 */
#define PINBALL_FRICTION            0.99f   /**< 摩擦系数 - 减小让速度衰减更快趋于匀速 */
#define PINBALL_BOUNCE_FACTOR       0.85f   /**< 反弹系数 - 增大让碰撞后速度变化更小 */
#define PINBALL_MAX_VELOCITY        18.0f   /**< 最大速度 */

#define PINBALL_UPDATE_INTERVAL_MS  16      /**< 物理更新间隔 (约60fps) */
#define PINBALL_RESTART_DELAY_MS    2000    /**< 到达终点后重启延迟 (毫秒) */

//#define PINBALL_START_X             355     /**< 弹珠起点X坐标 */
//#define PINBALL_START_Y             1169    /**< 弹珠起点Y坐标 */

#define PINBALL_END_ZONE_X          320     /**< 终点区域X坐标 */
#define PINBALL_END_ZONE_Y          1280    /**< 终点区域Y坐标 */
#define PINBALL_END_ZONE_WIDTH      80      /**< 终点区域宽度 */
#define PINBALL_END_ZONE_HEIGHT     30      /**< 终点区域高度 */

#define CHECK_INTERVAL 30    // 每30帧（0.5秒）检测一次
#define STUCK_DISTANCE 20.0f // 0.5秒内移动小于x像素算卡住
#define STUCK_THRESHOLD 5    // 连续3次（1.5秒）都卡住则重启


/*======================= 游戏阶段枚举 =======================*/
/**
 * @brief 游戏阶段枚举
 */
typedef enum {
    PINBALL_PHASE_UP,           /**< 第一阶段：垂直向上 */
    PINBALL_PHASE_LEFT_UP,      /**< 第二阶段：沿左上方 */
    PINBALL_PHASE_PHYSICS,      /**< 第三阶段：正常物理模拟 */
} pinball_game_phase_t;

/*======================= 障碍物类型枚举 =======================*/
/**
 * @brief 障碍物类型枚举
 */
typedef enum {
    PINBALL_OBSTACLE_RECT,      /**< 矩形障碍物 */
    PINBALL_OBSTACLE_CIRCLE,    /**< 圆形障碍物 (bumper) */
    PINBALL_OBSTACLE_LINE,      /**< 线段障碍物 */
    PINBALL_OBSTACLE_FLIPPER,   /**< 挡板 (flipper) */
} pinball_obstacle_type_t;

/*======================= 障碍物结构体 =======================*/
/**
 * @brief 障碍物结构体
 */
typedef struct {
    pinball_obstacle_type_t type;   /**< 障碍物类型 */
    float x;                        /**< X坐标位置 */
    float y;                        /**< Y坐标位置 */
    float w;                        /**< 宽度 (矩形) / 半径 (圆形) / 终点X (线段) */
    float h;                        /**< 高度 (矩形) / 未使用 (圆形) / 终点Y (线段) */
    float angle;                    /**< 角度 (用于挡板) */
    float bounce;                   /**< 反弹力度系数 */
} pinball_obstacle_t;

/*======================= 弹珠状态结构体 =======================*/
/**
 * @brief 弹珠状态结构体
 */
typedef struct {
    float x;            /**< 当前X坐标 */
    float y;            /**< 当前Y坐标 */
    float vx;           /**< X方向速度 */
    float vy;           /**< Y方向速度 */
    int active;         /**< 是否激活 (1=激活, 0=未激活) */
    lv_obj_t *obj;      /**< LVGL图片对象 */
	float history_x;      // N帧前的X位置
    float history_y;      // N帧前的Y位置
	int frame_counter;    // 帧计数器
	int stuck_counter;	  // 新增：卡住计数器
} pinball_ball_t;

/*======================= 路径点结构体 =======================*/
/**
 * @brief 路径点结构体 (内部使用)
 */
typedef struct {
    float x;    /**< X坐标 */
    float y;    /**< Y坐标 */
} pinball_path_point_t;

/*======================= 游戏句柄结构体 =======================*/
/**
 * @brief 弹珠游戏句柄结构体
 */
typedef struct _divoom_pinball_game_handle 
{
    lv_obj_t *parent;               /**< 父容器对象 */
    pinball_ball_t ball;            /**< 弹珠状态 */
    lv_timer_t *update_timer;       /**< 路径更新定时器 */
    lv_timer_t *restart_timer;      /**< 重启延迟定时器 */
    pinball_obstacle_t *obstacles;  /**< 障碍物数组指针 (保留兼容) */
    int obstacle_count;             /**< 障碍物数量 (保留兼容) */
    int start_x;                    /**< 弹珠起始X坐标 */
    int start_y;                    /**< 弹珠起始Y坐标 */
    
    /* 游戏阶段 */
    pinball_game_phase_t game_phase;  /**< 当前游戏阶段 */
    
    /* 路径相关变量 */
    const pinball_path_point_t *current_path;  /**< 当前路径点数组 */
    int current_path_index;         /**< 当前路径索引 */
    int path_point_count;           /**< 当前路径的点数 */
    int current_point_index;        /**< 当前在路径上的点索引 */
    float path_progress;            /**< 当前段的进度 (0.0~1.0) */
    float path_speed;               /**< 路径移动速度 */

	int pending_start_x;
	int pending_start_y;
	int start_pos_updated;
} divoom_pinball_game_handle, *Pdivoom_pinball_game_handle;

/*======================= 公共函数声明 =======================*/

/**
 * @brief 创建弹珠游戏
 * @param parent 父容器对象 (通常是背景图片对象)
 * @return 成功返回游戏句柄指针, 失败返回NULL
 */
Pdivoom_pinball_game_handle divoom_pinball_game_marble_create(lv_obj_t *parent, lv_obj_t *image_obj, int start_x, int start_y);

/**
 * @brief 更新弹珠起始坐标
 * @param handle 游戏句柄指针
 * @param start_x 新的起始X坐标
 * @param start_y 新的起始Y坐标
 * @return 成功返回0, 失败返回-1
 */
int divoom_pinball_game_marble_update(Pdivoom_pinball_game_handle handle, int start_x, int start_y);

/**
 * @brief 销毁弹珠游戏
 * @param handle 游戏句柄指针
 */
void divoom_pinball_game_marble_destroy(Pdivoom_pinball_game_handle handle);

/**
 * @brief 获取弹珠当前位置
 * @param handle 游戏句柄指针
 * @param x 输出参数, 返回X坐标
 * @param y 输出参数, 返回Y坐标
 * @return 成功返回0, 失败返回-1
 */
int pinball_game_marble_get_position(Pdivoom_pinball_game_handle handle, int *x, int *y);

/**
 * @brief 重置弹珠到起点位置
 * @param handle 游戏句柄指针
 * @return 成功返回0, 失败返回-1
 */
int pinball_game_marble_reset(Pdivoom_pinball_game_handle handle);

/**
 * @brief 暂停弹珠游戏
 * @param handle 游戏句柄指针
 * @return 成功返回0, 失败返回-1
 */
int pinball_game_marble_pause(Pdivoom_pinball_game_handle handle);

/**
 * @brief 恢复弹珠游戏
 * @param handle 游戏句柄指针
 * @return 成功返回0, 失败返回-1
 */
int pinball_game_marble_resume(Pdivoom_pinball_game_handle handle);

#ifdef __cplusplus
}
#endif


#endif

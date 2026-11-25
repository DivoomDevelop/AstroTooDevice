#include "divoom_marble_path.h"
#include "divoom.h"
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>

/*======================= 游戏区域边界常量 =======================*/
#define GAME_AREA_LEFT      25.0f
#define GAME_AREA_RIGHT     775.0f
#define GAME_AREA_TOP       440.0f
#define GAME_AREA_BOTTOM    1280.0f
#define COLLISION_SUBSTEPS  4

/*======================= 终点线定义 =======================*/
#define END_LINE_Y  1280.0f  /* 终点水平线Y坐标 */

/* 检测弹球是否碰到终点线 */
static int check_end_line_collision(float bx, float by)
{
    /* 检测球的底部是否到达或超过 y=1280 这条水平线 */
    if (by + PINBALL_BALL_RADIUS >= END_LINE_Y) {
        return 1;  /* 碰到终点线 */
    }
    return 0;
}

/*======================= 入场阶段定义 =======================*/
/* 起始位置 */
#define ENTRY_START_X       755.0f//736.0f
#define ENTRY_START_Y       942.0f

/* 第一阶段终点：垂直向上到达的Y坐标 */
#define PHASE1_END_Y        441.0f

/* 第二阶段终点：左上方到达的位置 */
#define PHASE2_END_X        250.0f
#define PHASE2_END_Y        500.0f

/* 入场阶段的速度 */
#define PHASE_UP_SPEED      12.0f   /* 垂直向上的速度 */
#define PHASE_LEFT_UP_VX    -8.0f  /* 左上方阶段X方向速度（向左） */
#define PHASE_LEFT_UP_VY    -3.0f   /* 左上方阶段Y方向速度（向上） */

/*======================= 障碍物定义数据 =======================*/
/* 线段格式: {type, x1, y1, x2, y2, angle(unused), bounce} */
static pinball_obstacle_t obstacles_data[] = {
    /* 红色大bumper - 圆心(410, 505) 半径40 */
    {PINBALL_OBSTACLE_CIRCLE, 410, 505, 40, 0, 0, 1.3f},
    
    /* 左侧弧形灯带 - 碰到反弹 */
   /* {PINBALL_OBSTACLE_CIRCLE, 75, 500, 18, 0, 0, 1.0f},
    {PINBALL_OBSTACLE_CIRCLE, 65, 550, 18, 0, 0, 1.0f},
    {PINBALL_OBSTACLE_CIRCLE, 60, 600, 18, 0, 0, 1.0f},
    {PINBALL_OBSTACLE_CIRCLE, 60, 650, 18, 0, 0, 1.0f},
    {PINBALL_OBSTACLE_CIRCLE, 65, 700, 18, 0, 0, 1.0f},
    {PINBALL_OBSTACLE_CIRCLE, 75, 750, 18, 0, 0, 1.0f},
  */
	/* 左侧弧形灯带 - 碰到反弹 */
	{PINBALL_OBSTACLE_CIRCLE, 170, 466, 8+4, 0, 0, 1.1f},
    {PINBALL_OBSTACLE_CIRCLE, 141, 482, 12+4, 0, 0, 1.1f},
    {PINBALL_OBSTACLE_CIRCLE, 116, 507, 12+4, 0, 0, 1.1f},
    {PINBALL_OBSTACLE_CIRCLE, 93, 542, 13+4, 0, 0, 1.1f},
    {PINBALL_OBSTACLE_CIRCLE, 83, 583, 15+4, 0, 0, 1.1f},
    {PINBALL_OBSTACLE_CIRCLE, 83, 622, 15+4, 0, 0, 1.1f},
    {PINBALL_OBSTACLE_CIRCLE, 84, 660, 13+4, 0, 0, 1.1f},
	{PINBALL_OBSTACLE_CIRCLE, 95, 699, 14+4, 0, 0, 1.1f},
    {PINBALL_OBSTACLE_CIRCLE, 114, 734, 15+4, 0, 0, 1.1f},
    {PINBALL_OBSTACLE_CIRCLE, 140, 765, 14+2, 0, 0, 1.1f},
	{PINBALL_OBSTACLE_CIRCLE, 157, 779, 14+2, 0, 0, 1.1f},//填充
    {PINBALL_OBSTACLE_CIRCLE, 175, 791, 15+2, 0, 0, 1.1f},

	/* 左侧梯形: (60, 540) → (60, 1070) */
    {PINBALL_OBSTACLE_LINE, 108, 915, 162, 915, 0, 1.1f},
	{PINBALL_OBSTACLE_LINE, 162, 915, 186, 1028, 0, 1.1f},
	{PINBALL_OBSTACLE_LINE, 108, 915, 108, 1000, 0, 1.1f},
	{PINBALL_OBSTACLE_LINE, 108, 1000, 186, 1028, 0, 1.1f},

	/* 左侧斜矩形: 四条线组成 */
    {PINBALL_OBSTACLE_LINE, 67, 790, 119, 822, 0, 1.1f},
	{PINBALL_OBSTACLE_LINE, 67, 790, 67, 815, 0, 1.1f},
	{PINBALL_OBSTACLE_LINE, 67, 815, 118, 850, 0, 1.1f},
	{PINBALL_OBSTACLE_LINE, 119, 822, 119, 850, 0, 1.1f},

	/* 右侧梯形: (60, 540) → (60, 1070) */
    {PINBALL_OBSTACLE_LINE, 587, 915, 643, 915, 0, 1.1f},
	{PINBALL_OBSTACLE_LINE, 643, 915, 643, 1000, 0, 1.1f},
	{PINBALL_OBSTACLE_LINE, 561, 1034, 587, 915, 0, 1.1f},
	{PINBALL_OBSTACLE_LINE, 561, 1034, 643, 1000, 0, 1.1f},

	/* 右侧斜矩形: 四条线组成 */
    {PINBALL_OBSTACLE_LINE, 642, 823, 683, 796, 0, 1.1f},
	{PINBALL_OBSTACLE_LINE, 642, 823, 642, 849, 0, 1.1f},
	{PINBALL_OBSTACLE_LINE, 642, 849, 683, 821, 0, 1.1f},
	{PINBALL_OBSTACLE_LINE, 683, 796, 683, 821, 0, 1.1f},


	/* 右上靠左障碍 - 矩形 */
    {PINBALL_OBSTACLE_RECT, 577, 578, 40, 145, 0, 1.0f},
    
    /* 右上靠右障碍 - 矩形   */
    {PINBALL_OBSTACLE_RECT, 640, 635, 30, 85, 0, 1.0f},

    /* 左侧边界线段: (60, 540) → (60, 1070) */
    {PINBALL_OBSTACLE_LINE, 60, 540, 60, 1070, 0, 1.0f},
    
    /* 右侧边界线段: (700, 560) → (700, 1075) */
    {PINBALL_OBSTACLE_LINE, 700, 560, 700, 1075, 0, 1.0f},
    
	/* 右上线段1: (594, 460) → (657, 496) */
    {PINBALL_OBSTACLE_LINE, 594, 460, 657, 496, 0, 1.0f},
    
    /* 右上线段2: (657, 496) → (697, 564) */
    {PINBALL_OBSTACLE_LINE, 657, 496, 697, 564, 0, 1.0f},

    /* 左下区域障碍 - 矩形 */
    {PINBALL_OBSTACLE_RECT, 74, 918, 20, 100, 0, 1.0f},
    
    /* 右下区域障碍 (660, 920) */
    {PINBALL_OBSTACLE_RECT, 657, 918, 20, 100, 0, 1.0f},
    
    /* 左挡板 - 上线段: (90, 1020) → (333, 1140) */
    {PINBALL_OBSTACLE_LINE, 90, 1020, 333, 1140, 0, 1.2f},
    /* 左挡板 - 下线段: (80, 1049) → (333, 1179) */
    {PINBALL_OBSTACLE_LINE, 80, 1049, 295, 1160, 0, 1.1f},
    
    /* 右挡板 - 上线段: (658, 1017) → (409, 1138) */
    {PINBALL_OBSTACLE_LINE, 658, 1017, 414, 1137, 0, 1.2f},
    /* 右挡板 - 下线段: (673, 1042) → (428, 1176) */
    {PINBALL_OBSTACLE_LINE, 673, 1042, 463, 1157, 0, 1.1f},
};
static const int obstacles_count = sizeof(obstacles_data) / sizeof(obstacles_data[0]);

/*======================= 私有函数实现 =======================*/

/**
 * @brief 生成随机浮点数
 */
static float random_float(float min, float max)
{
    return min + (float)rand() / (float)RAND_MAX * (max - min);
}

/**
 * @brief 检测弹珠与圆形障碍物碰撞
 */
static int check_collision_circle(float bx, float by, float ox, float oy, float radius)
{
    float dx = bx - ox;
    float dy = by - oy;
    return (dx * dx + dy * dy) < (radius * radius) ? 1 : 0;
}

/**
 * @brief 检测弹珠与矩形障碍物碰撞
 */
static int check_collision_rect(float bx, float by, float rx, float ry, float rw, float rh)
{
    return (bx > rx - PINBALL_BALL_RADIUS && bx < rx + rw + PINBALL_BALL_RADIUS &&
            by > ry - PINBALL_BALL_RADIUS && by < ry + rh + PINBALL_BALL_RADIUS) ? 1 : 0;
}

/**
 * @brief 检测弹珠与线段障碍物碰撞
 */
static int check_collision_line(float bx, float by, float x1, float y1, float x2, float y2, float *nx, float *ny)
{
    float dx = x2 - x1;
    float dy = y2 - y1;
    float len_sq = dx * dx + dy * dy;
    
    if (len_sq == 0) return 0;
    
    float t = ((bx - x1) * dx + (by - y1) * dy) / len_sq;
    if (t < 0) t = 0;
    if (t > 1) t = 1;
    
    float closest_x = x1 + t * dx;
    float closest_y = y1 + t * dy;
    
    float dist_x = bx - closest_x;
    float dist_y = by - closest_y;
    float dist = sqrtf(dist_x * dist_x + dist_y * dist_y);
    
    if (dist < PINBALL_BALL_RADIUS) {
        float line_len = sqrtf(len_sq);
        *nx = -dy / line_len;
        *ny = dx / line_len;
        
        if ((*nx) * dist_x + (*ny) * dist_y < 0) {
            *nx = -(*nx);
            *ny = -(*ny);
        }
        return 1;
    }
    return 0;
}

/**
 * @brief 应用反弹效果
 */
static void apply_bounce(Pdivoom_pinball_game_handle handle, float nx, float ny, float bounce_factor)
{
    float dot = handle->ball.vx * nx + handle->ball.vy * ny;
    handle->ball.vx = (handle->ball.vx - 2 * dot * nx) * bounce_factor;
    handle->ball.vy = (handle->ball.vy - 2 * dot * ny) * bounce_factor;
}

/**
 * @brief 内部重置弹珠位置（进入第一阶段：垂直向上）
 */
static void reset_ball_internal(Pdivoom_pinball_game_handle handle)
{
    /* 初始化为第一阶段：垂直向上 */
    handle->game_phase = PINBALL_PHASE_UP;
    
    /* 设置球到起始位置 */
    handle->ball.x = ENTRY_START_X;
    handle->ball.y = ENTRY_START_Y;
    
    /* 设置垂直向上的速度 */
    handle->ball.vx = 0;
    handle->ball.vy = -PHASE_UP_SPEED;  /* 负值表示向上 */
    
    handle->ball.history_x = handle->ball.x;
    handle->ball.history_y = handle->ball.y;
    handle->ball.frame_counter = 0;
    handle->ball.stuck_counter = 0;
    handle->ball.active = 1;
    
    lv_obj_set_pos(handle->ball.obj, 
                   (lv_coord_t)(handle->ball.x - PINBALL_BALL_RADIUS), 
                   (lv_coord_t)(handle->ball.y - PINBALL_BALL_RADIUS));
}

/**
 * @brief 切换到第二阶段：左上方
 */
static void switch_to_phase_left_up(Pdivoom_pinball_game_handle handle)
{
    handle->game_phase = PINBALL_PHASE_LEFT_UP;
    
    /* 设置左上方的速度 */
    handle->ball.vx = PHASE_LEFT_UP_VX;
    handle->ball.vy = PHASE_LEFT_UP_VY;
}

/**
 * @brief 切换到第三阶段：正常物理模拟
 */
static void switch_to_phase_physics(Pdivoom_pinball_game_handle handle)
{
    handle->game_phase = PINBALL_PHASE_PHYSICS;
    
    /* 在定时器线程中安全地应用新坐标 */
    if (handle->start_pos_updated) {
        handle->start_x = handle->pending_start_x;
        handle->start_y = handle->pending_start_y;
        handle->start_pos_updated = 0;
    }
    
    /* 记录当前位置用于卡住检测 */
    handle->ball.history_x = handle->ball.x;
    handle->ball.history_y = handle->ball.y;
    handle->ball.frame_counter = 0;
    handle->ball.stuck_counter = 0;

    /* 保持当前速度方向，添加一些随机性 */
    float speed = random_float(30.0f, 45.0f);
    float y_speed = random_float(30.0f, 45.0f);
    handle->ball.vx = -speed * 0.25f;  /* 向左 */
    handle->ball.vy = y_speed * 0.95f; /* 向下（因为重力会向下） */
}

/**
 * @brief 重启游戏定时器回调
 */
static void restart_game_cb(lv_timer_t *timer)
{
    Pdivoom_pinball_game_handle handle = (Pdivoom_pinball_game_handle)lv_timer_get_user_data(timer);
    if (handle == NULL) return;
    
    handle->restart_timer = NULL;
    reset_ball_internal(handle);
}

/**
 * @brief 单步碰撞检测和处理
 */
static void do_collision_step(Pdivoom_pinball_game_handle handle, float *px, float *py)
{
    int i;
    float new_x = *px;
    float new_y = *py;
    
    for (i = 0; i < obstacles_count; i++) {
        pinball_obstacle_t *obs = &obstacles_data[i];
        float nx = 0, ny = 0;
        
        switch (obs->type) {
            case PINBALL_OBSTACLE_CIRCLE:
                if (check_collision_circle(new_x, new_y, obs->x, obs->y, obs->w + PINBALL_BALL_RADIUS)) {
                    float dx = new_x - obs->x;
                    float dy = new_y - obs->y;
                    float dist = sqrtf(dx * dx + dy * dy);
                    if (dist > 0.001f) {
                        nx = dx / dist;
                        ny = dy / dist;
                        apply_bounce(handle, nx, ny, obs->bounce);
                        new_x = obs->x + nx * (obs->w + PINBALL_BALL_RADIUS + 7);
                        new_y = obs->y + ny * (obs->w + PINBALL_BALL_RADIUS + 7);
                    }
                }
                break;
                
            case PINBALL_OBSTACLE_RECT:
                if (check_collision_rect(new_x, new_y, obs->x, obs->y, obs->w, obs->h)) {
                    float cx = obs->x + obs->w / 2;
                    float cy = obs->y + obs->h / 2;
                    float dx = new_x - cx;
                    float dy = new_y - cy;
                    float half_w = obs->w / 2 + PINBALL_BALL_RADIUS;
                    float half_h = obs->h / 2 + PINBALL_BALL_RADIUS;
                    
                    if (fabsf(dx) / half_w > fabsf(dy) / half_h) {
                        if (dx > 0) {
                            nx = 1; ny = 0;
                            new_x = obs->x + obs->w + PINBALL_BALL_RADIUS + 7;
                        } else {
                            nx = -1; ny = 0;
                            new_x = obs->x - PINBALL_BALL_RADIUS - 7;
                        }
                    } else {
                        if (dy > 0) {
                            nx = 0; ny = 1;
                            new_y = obs->y + obs->h + PINBALL_BALL_RADIUS + 7;
                        } else {
                            nx = 0; ny = -1;
                            new_y = obs->y - PINBALL_BALL_RADIUS - 7;
                        }
                    }
                    apply_bounce(handle, nx, ny, obs->bounce);
                }
                break;
                
            case PINBALL_OBSTACLE_LINE:
            case PINBALL_OBSTACLE_FLIPPER:
                if (check_collision_line(new_x, new_y, obs->x, obs->y, obs->w, obs->h, &nx, &ny)) {
                    apply_bounce(handle, nx, ny, obs->bounce);
                    new_x += nx * (PINBALL_BALL_RADIUS + 2);
                    new_y += ny * (PINBALL_BALL_RADIUS + 2);
                }
                break;
        }
    }
    
    /* 边界检测 */
    if (new_x < GAME_AREA_LEFT + PINBALL_BALL_RADIUS) {
        new_x = GAME_AREA_LEFT + PINBALL_BALL_RADIUS;
        handle->ball.vx = -handle->ball.vx * PINBALL_BOUNCE_FACTOR;
    }
    if (new_x > GAME_AREA_RIGHT - PINBALL_BALL_RADIUS) {
        new_x = GAME_AREA_RIGHT - PINBALL_BALL_RADIUS;
        handle->ball.vx = -handle->ball.vx * PINBALL_BOUNCE_FACTOR;
    }
    if (new_y < GAME_AREA_TOP + PINBALL_BALL_RADIUS) {
        new_y = GAME_AREA_TOP + PINBALL_BALL_RADIUS;
        handle->ball.vy = -handle->ball.vy * PINBALL_BOUNCE_FACTOR;
    }
    if (new_y > GAME_AREA_BOTTOM - PINBALL_BALL_RADIUS) {
        new_y = GAME_AREA_BOTTOM - PINBALL_BALL_RADIUS;
        handle->ball.vy = -handle->ball.vy * 0.5f;
    }
    
    *px = new_x;
    *py = new_y;
}

/**
 * @brief 物理模式更新逻辑
 */
static void update_physics_mode(Pdivoom_pinball_game_handle handle)
{
    /* 应用重力 */
    handle->ball.vy += PINBALL_GRAVITY;
    
    /* 应用摩擦力 */
    handle->ball.vx *= PINBALL_FRICTION;
    handle->ball.vy *= PINBALL_FRICTION;

    /* 限制最大速度 */
    float max_vel = PINBALL_MAX_VELOCITY;
    if (handle->ball.vx > max_vel) handle->ball.vx = max_vel;
    if (handle->ball.vx < -max_vel) handle->ball.vx = -max_vel;
    if (handle->ball.vy > max_vel) handle->ball.vy = max_vel;
    if (handle->ball.vy < -max_vel) handle->ball.vy = -max_vel;
    
    /* 子步进碰撞检测 */
    float step_vx = handle->ball.vx / COLLISION_SUBSTEPS;
    float step_vy = handle->ball.vy / COLLISION_SUBSTEPS;
    float new_x = handle->ball.x;
    float new_y = handle->ball.y;

    int step;
    for (step = 0; step < COLLISION_SUBSTEPS; step++) {
        new_x += step_vx;
        new_y += step_vy;
        do_collision_step(handle, &new_x, &new_y);
    }
    
    /* 更新弹珠位置 */
    handle->ball.x = new_x;
    handle->ball.y = new_y;
    
    lv_obj_set_pos(handle->ball.obj, 
                   (lv_coord_t)(handle->ball.x - PINBALL_BALL_RADIUS), 
                   (lv_coord_t)(handle->ball.y - PINBALL_BALL_RADIUS));

    /* 检测底部停留 */
    float speed_sq = handle->ball.vx * handle->ball.vx + handle->ball.vy * handle->ball.vy;
	
    //if (handle->ball.y > GAME_AREA_BOTTOM - 50 && speed_sq < 4.0f) {
    //    handle->ball.vx = random_float(-5.0f, 5.0f);
    //    handle->ball.vy = random_float(-10.0f, -6.0f);
    //}

    /* 检查是否碰到终点线 */
    if (check_end_line_collision(handle->ball.x, handle->ball.y))
	{
        handle->ball.active = 0;
        if (handle->restart_timer == NULL) {
            handle->restart_timer = lv_timer_create(restart_game_cb, PINBALL_RESTART_DELAY_MS, handle);
            lv_timer_set_repeat_count(handle->restart_timer, 1);
        }
    }
	else	//检测是否卡住不动,检测一段时间内的累积位移
	{
		handle->ball.frame_counter++;
	
		if (handle->ball.frame_counter >= CHECK_INTERVAL) {
			// 计算这段时间内的实际位移
			float dx = handle->ball.x - handle->ball.history_x;
			float dy = handle->ball.y - handle->ball.history_y;
			float total_dist = sqrtf(dx * dx + dy * dy);
		
			if (total_dist < STUCK_DISTANCE || dx == 0 || dy == 0) {
				handle->ball.stuck_counter++;
				if (handle->ball.stuck_counter >= STUCK_THRESHOLD) {
					// 触发重启
					handle->ball.active = 0;
        			if (handle->restart_timer == NULL) {
            			handle->restart_timer = lv_timer_create(restart_game_cb, PINBALL_RESTART_DELAY_MS, handle);
            			lv_timer_set_repeat_count(handle->restart_timer, 1);
       				}
				}
			} else {
				handle->ball.stuck_counter = 0;
			}
		
			// 记录当前位置，重置计数器
			handle->ball.history_x = handle->ball.x;
			handle->ball.history_y = handle->ball.y;
			handle->ball.frame_counter = 0;
		}
	}
}

/**
 * @brief 入场阶段更新（第一、第二阶段共用，无重力无碰撞）
 */
static void update_entry_phase(Pdivoom_pinball_game_handle handle)
{
    /* 按速度移动（无重力、无摩擦、无碰撞） */
    handle->ball.x += handle->ball.vx;
    handle->ball.y += handle->ball.vy;
    
    /* 更新显示位置 */
    lv_obj_set_pos(handle->ball.obj, 
                   (lv_coord_t)(handle->ball.x - PINBALL_BALL_RADIUS), 
                   (lv_coord_t)(handle->ball.y - PINBALL_BALL_RADIUS));
}

/**
 * @brief 物理更新定时器回调
 */
static void update_physics_cb(lv_timer_t *timer)
{
    Pdivoom_pinball_game_handle handle = (Pdivoom_pinball_game_handle)lv_timer_get_user_data(timer);
    if (handle == NULL || !handle->ball.active) return;
    
    switch (handle->game_phase) {
        case PINBALL_PHASE_UP:
            /* 第一阶段：垂直向上 */
            update_entry_phase(handle);
            
            /* 检查是否到达第一阶段终点 */
            if (handle->ball.y <= PHASE1_END_Y) {
                switch_to_phase_left_up(handle);
            }
            break;
            
        case PINBALL_PHASE_LEFT_UP:
            /* 第二阶段：沿左上方 */
            update_entry_phase(handle);
            
            /* 检查是否到达第二阶段终点 */
            if (handle->ball.x <= PHASE2_END_X || handle->ball.y >= PHASE2_END_Y) {
                switch_to_phase_physics(handle);
            }
            break;
            
        case PINBALL_PHASE_PHYSICS:
            /* 第三阶段：正常物理模拟 */
            update_physics_mode(handle);
            break;
    }
}

/**
 * @brief 创建弹珠对象
 */
static void create_ball(Pdivoom_pinball_game_handle handle)
{
    reset_ball_internal(handle);
}

/*======================= 公共函数实现 =======================*/

Pdivoom_pinball_game_handle divoom_pinball_game_marble_create(lv_obj_t *parent, lv_obj_t *image_obj, int start_x, int start_y)
{
    if (parent == NULL) {
        return NULL;
    }
    
    srand((unsigned int)time(NULL));
    
    Pdivoom_pinball_game_handle handle = (Pdivoom_pinball_game_handle)divoom_system_malloc(sizeof(divoom_pinball_game_handle));
    if (handle == NULL) {
        return NULL;
    }
    
    memset(handle, 0, sizeof(divoom_pinball_game_handle));
    handle->parent = parent;
    handle->start_x = start_x;
    handle->start_y = start_y;
    handle->ball.obj = image_obj;
    
    /* 初始化障碍物 */
    handle->obstacles = obstacles_data;
    handle->obstacle_count = obstacles_count;
    
    create_ball(handle);
    
    /* 使用物理更新 */
    handle->update_timer = lv_timer_create(update_physics_cb, PINBALL_UPDATE_INTERVAL_MS, handle);
    handle->restart_timer = NULL;
    
    return handle;
}

int divoom_pinball_game_marble_update(Pdivoom_pinball_game_handle handle, int start_x, int start_y)
{
    if (handle == NULL) {
        return -1;
    }
    
    // 先暂存新值
    handle->pending_start_x = start_x;
    handle->pending_start_y = start_y;
    handle->start_pos_updated = 1;  // 标记需要更新
    
    return 0;
}


void divoom_pinball_game_marble_destroy(Pdivoom_pinball_game_handle handle)
{
    if (handle == NULL) {
        return;
    }
    
    if (handle->update_timer) {
        lv_timer_del(handle->update_timer);
        handle->update_timer = NULL;
    }
    
    if (handle->restart_timer) {
        lv_timer_del(handle->restart_timer);
        handle->restart_timer = NULL;
    }
    
    if (handle->ball.obj) {
        lv_obj_del(handle->ball.obj);
        handle->ball.obj = NULL;
    }
    
    divoom_system_free(handle);
}

int pinball_game_marble_get_position(Pdivoom_pinball_game_handle handle, int *x, int *y)
{
    if (handle == NULL || x == NULL || y == NULL) {
        return -1;
    }
    
    *x = (int)handle->ball.x;
    *y = (int)handle->ball.y;
    
    return 0;
}

int pinball_game_marble_reset(Pdivoom_pinball_game_handle handle)
{
    if (handle == NULL) {
        return -1;
    }
    
    if (handle->restart_timer) {
        lv_timer_del(handle->restart_timer);
        handle->restart_timer = NULL;
    }
    
    reset_ball_internal(handle);
    
    return 0;
}

int pinball_game_marble_pause(Pdivoom_pinball_game_handle handle)
{
    if (handle == NULL) {
        return -1;
    }
    
    if (handle->update_timer) {
        lv_timer_pause(handle->update_timer);
    }
    
    if (handle->restart_timer) {
        lv_timer_pause(handle->restart_timer);
    }
    
    return 0;
}

int pinball_game_marble_resume(Pdivoom_pinball_game_handle handle)
{
    if (handle == NULL) {
        return -1;
    }
    
    if (handle->update_timer) {
        lv_timer_resume(handle->update_timer);
    }
    
    if (handle->restart_timer) {
        lv_timer_resume(handle->restart_timer);
    }
    
    return 0;
}

#ifndef _DIVOOM_LINE_IN_H_
#define _DIVOOM_LINE_IN_H_

#include "divoom_type.h"

#ifdef __cplusplus
extern "C" {
#endif

// Line In 播放状态
typedef enum {
    LINE_IN_STATUS_IDLE = 0,       // 空闲状态
    LINE_IN_STATUS_PLAYING,        // 播放中
    LINE_IN_STATUS_PAUSED,         // 暂停
    LINE_IN_STATUS_STOPPED         // 停止
} LineInStatus;

// 初始化 Line In 模块
void divoom_line_in_init(void);

// 反初始化 Line In 模块
void divoom_line_in_deinit(void);

// 是否已成功初始化（用于调用方判断 init 是否成功）
int divoom_line_in_is_initialized(void);

// 开始 Line In 播放
void divoom_line_in_start(void);

// 停止 Line In 播放
void divoom_line_in_stop(void);

// 暂停 Line In 播放
void divoom_line_in_pause(void);

// 恢复 Line In 播放
void divoom_line_in_resume(void);

// 设置 Line In 音量 (0-100)
void divoom_line_in_set_volume(uint8 volume);

// 获取 Line In 音量
uint8 divoom_line_in_get_volume(void);

// 获取当前播放状态
LineInStatus divoom_line_in_get_status(void);

#ifdef __cplusplus
}
#endif

#endif /* _DIVOOM_LINE_IN_H_ */


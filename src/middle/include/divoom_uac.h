#ifndef _DIVOOM_UAC_H_
#define _DIVOOM_UAC_H_

#include "divoom_type.h"

#ifdef __cplusplus
extern "C" {
#endif

// UAC 播放状态
typedef enum {
    UAC_STATUS_IDLE = 0,           // 空闲状态
    UAC_STATUS_PLAYING,            // 播放中
    UAC_STATUS_PAUSED,             // 暂停
    UAC_STATUS_STOPPED             // 停止
} UacStatus;

// 初始化 UAC 模块
void divoom_uac_init(void);

// 反初始化 UAC 模块
void divoom_uac_deinit(void);

// 开始 UAC 播放
void divoom_uac_start(void);

// 停止 UAC 播放
void divoom_uac_stop(void);

// 暂停 UAC 播放
void divoom_uac_pause(void);

// 恢复 UAC 播放
void divoom_uac_resume(void);

// 设置 UAC 音量 (0-100)
void divoom_uac_set_volume(uint8 volume);

// 获取 UAC 音量
uint8 divoom_uac_get_volume(void);

// 获取当前播放状态
UacStatus divoom_uac_get_status(void);

#ifdef __cplusplus
}
#endif

#endif /* _DIVOOM_UAC_H_ */

#ifndef _divoom_buddy_h_
#define _divoom_buddy_h_

#include "divoom_type.h"
#include "lvgl.h"

/*
 * Buddy 表情图层 LVGL 控件：外部只设 state id，
 * 内部加载 states state.json 及引用的 clips JSON，按配置随机组合播放。
 */
lv_obj_t *divoom_buddy_create(lv_obj_t *parent);
void divoom_buddy_set_config_dir(lv_obj_t *obj, const char *cfg_dir);
void divoom_buddy_set_state(lv_obj_t *obj, const char *state_id);
const char *divoom_buddy_get_state(lv_obj_t *obj);
void divoom_buddy_set_layer_pos(lv_obj_t *obj, const char *layer_id, int x, int y);
/* dial_menu 持 LVGL 锁时调用，立即加载 pending state 并出首帧 */
void divoom_buddy_flush_pending_mode(lv_obj_t *obj);
/* dial_menu pause 时调用，停止 Buddy 内部动画 timer；对象释放仍由 lv_obj_del 触发 destructor */
void divoom_buddy_pause(lv_obj_t *obj);
/* dial_menu 复用暂停控件时调用，重建 timer，并重新应用暂停前的状态。 */
void divoom_buddy_resume(lv_obj_t *obj);
/* 在当前状态之上播放一次指定动作；sequence 相同的重绘不会重复启动。 */
int divoom_buddy_trigger_action(lv_obj_t *obj, const char *clip_id,
    uint32 sequence);
#if defined(DIVOOM_SIMULATOR)
/* Buddy 14 种可见状态的白盒覆盖位，只记录真实首帧显示，不参与正式调度。 */
#define DIVOOM_BUDDY_WB_SIT_NEUTRAL       (1u << 0)
#define DIVOOM_BUDDY_WB_SIT_HAPPY         (1u << 1)
#define DIVOOM_BUDDY_WB_SIT_CURIOUS       (1u << 2)
#define DIVOOM_BUDDY_WB_SIT_SNIFF         (1u << 3)
#define DIVOOM_BUDDY_WB_SIT_SLEEPY        (1u << 4)
#define DIVOOM_BUDDY_WB_PRONE_NEUTRAL     (1u << 5)
#define DIVOOM_BUDDY_WB_PRONE_SLEEPY      (1u << 6)
#define DIVOOM_BUDDY_WB_PRONE_SAD         (1u << 7)
#define DIVOOM_BUDDY_WB_STRETCH           (1u << 8)
#define DIVOOM_BUDDY_WB_TURN_BACK         (1u << 9)
#define DIVOOM_BUDDY_WB_PLAY_BALL         (1u << 10)
#define DIVOOM_BUDDY_WB_SING              (1u << 11)
#define DIVOOM_BUDDY_WB_SIT_TO_PRONE      (1u << 12)
#define DIVOOM_BUDDY_WB_PRONE_TO_SIT      (1u << 13)
/* 白盒生命周期探针：返回该 Buddy 当前持有的 PNG 解码输出缓冲数量。 */
int divoom_buddy_whitebox_get_image_buffer_count(lv_obj_t *obj);
/* 白盒生命周期探针：按 Buddy 实际所有权统计动画 timer。 */
int divoom_buddy_whitebox_get_timer_count(lv_obj_t *obj);
/* 白盒生命周期探针：统计实际 active 通道，并暴露累计解码失败。 */
int divoom_buddy_whitebox_get_active_channel_count(lv_obj_t *obj);
uint32 divoom_buddy_whitebox_get_decode_fail_count(lv_obj_t *obj);
void divoom_buddy_whitebox_reset_coverage(lv_obj_t *obj);
/* 覆盖用例期间锁定指定状态，避免正式 AI 调度覆盖正在验证的资源池。 */
void divoom_buddy_whitebox_set_forced_state(lv_obj_t *obj, const char *state_id);
uint32 divoom_buddy_whitebox_get_coverage(lv_obj_t *obj);
uint32 divoom_buddy_whitebox_get_action_start_count(lv_obj_t *obj);
const char *divoom_buddy_whitebox_get_last_action_clip(lv_obj_t *obj);
#endif

/* 兼容旧名：state_id 对应 states/<state_id>.json */
#define divoom_buddy_set_mode             divoom_buddy_set_state
#define divoom_buddy_get_mode             divoom_buddy_get_state
#define divoom_buddy_set_display_mode     divoom_buddy_set_state

#endif

#ifndef _DIVOOM_INPUT_HOTPLUG_H_
#define _DIVOOM_INPUT_HOTPLUG_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Line-in / USB / SD卡(TF) / 充电满 检测：以普通 GPIO 轮询实现。
 * 内部开一线程循环读取各检测脚电平，状态变化时回调。
 * DTS 中对应引脚需由本模块独占（勿与 gpio-keys 重复占用）。
 */

typedef void (*DIVOOM_INPUT_HOTPLUG_CB)(int key_code, int state, void *user);
typedef void (*DIVOOM_DET_HOTPLUG_CB)(int inserted);

/* 默认 key code（建议各检测项用不同 code，避免混淆） */
#define DIVOOM_USB_KEY_CODE_DEFAULT      183 /* KEY_F13 */
#define DIVOOM_LINEIN_KEY_CODE_DEFAULT   184 /* KEY_F14 */
#define DIVOOM_TF_KEY_CODE_DEFAULT       185 /* KEY_F15 */
#define DIVOOM_CHG_FULL_KEY_CODE_DEFAULT 186 /* KEY_F16, 1=充满(低电平触发) */
#define DIVOOM_FAST_CHG_KEY_CODE_DEFAULT  187 /* KEY_F17, 1=快充口接入(PF5) */

/* 注册一个 key_code 的状态变化回调
 * - key_code: Linux input key code（例如 KEY_F13 = 183）
 * - state: 1=插入/有效，0=拔出/无效
 */
int divoom_input_hotplug_register(int key_code, DIVOOM_INPUT_HOTPLUG_CB cb, void *user);
/* 简单模式：只关心 state(1/0)，USB/Line-in/TF 都可复用 */
int divoom_input_hotplug_register_simple(int key_code, DIVOOM_DET_HOTPLUG_CB cb);
int divoom_input_hotplug_unregister(int key_code);
int divoom_input_hotplug_get_state(int key_code); /* 返回 1/0，未知返回 -1 */

/* 启动/停止监听线程 */
int divoom_input_hotplug_start(void);
void divoom_input_hotplug_stop(void);

/* -------------------- 兼容接口（默认 key code 见上面宏） -------------------- */
/* 仅当 USB 接 PC 时内核会置为 CONFIGURED，接充电器不会；用于判断是否可进入 UAC */
int divoom_usb_is_configured(void);
int divoom_usb_hotplug_init(DIVOOM_DET_HOTPLUG_CB cb);
int divoom_usb_hotplug_get_state(void);
void divoom_usb_hotplug_deinit(void);

int divoom_linein_hotplug_init(DIVOOM_DET_HOTPLUG_CB cb);
int divoom_linein_hotplug_get_state(void);
void divoom_linein_hotplug_deinit(void);

int divoom_tf_hotplug_init(DIVOOM_DET_HOTPLUG_CB cb);
int divoom_tf_hotplug_get_state(void);
void divoom_tf_hotplug_deinit(void);

/* 充电状态（CHG-STAT）：回调 inserted=1 表示“充满”，inserted=0 表示“未充满/充电中” */
int divoom_chg_full_init(DIVOOM_DET_HOTPLUG_CB cb);
int divoom_chg_full_get_state(void);
void divoom_chg_full_deinit(void);

/* 快充口检测（PF5/USB12V-DET）：回调 inserted=1 表示快充口接入，0 表示未接入 */
int divoom_fast_chg_hotplug_init(DIVOOM_DET_HOTPLUG_CB cb);
int divoom_fast_chg_hotplug_get_state(void);
void divoom_fast_chg_hotplug_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* _DIVOOM_INPUT_HOTPLUG_H_ */


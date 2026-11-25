#ifndef DIVOOM_LVGL_TOP_RADIO_WAIT_H
#define DIVOOM_LVGL_TOP_RADIO_WAIT_H

#include "divoom_type.h"

/* 菜单初始化时在 LVGL 线程注册一次；浮层不接管按键或页面焦点。 */
void divoom_lvgl_top_radio_wait_init(void);
/* 当前目录视图由 LVGL 线程报送。退出或加载结束时传 loading=0。 */
void divoom_lvgl_top_radio_wait_set_catalog(uint8 loading, int32 view, uint32 request);
/* 任意物理键请求收起当前提示，可从按键线程调用。 */
void divoom_lvgl_top_radio_wait_dismiss_on_key(void);

#endif

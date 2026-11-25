#ifndef _divoom_lvgl_top_source_switch_menu_h_
#define _divoom_lvgl_top_source_switch_menu_h_

#include "divoom_type.h"

/** 顶栏切源 OSD 文案类型（notice_type 参数） */
#define DIVOOM_SOURCE_SWITCH_OSD_SWITCH     0
#define DIVOOM_SOURCE_SWITCH_OSD_PLS_INSERT 1
#define DIVOOM_SOURCE_SWITCH_OSD_CURRENT    2  /* 无其它音源：只显示源名，如「蓝牙」 */

/** M 键切源 / 音源页校验失败等顶栏提示（2.2s 自动消失；连按刷新） */
void divoom_lvgl_top_source_switch_menu_init(uint8 music_mode, uint8 notice_type);

void divoom_lvgl_top_source_switch_menu_exit(void);

#endif

#ifndef _divoom_silent_snow_bg_h_
#define _divoom_silent_snow_bg_h_

#include "lvgl.h"
#include "divoom_type.h"

/*
  Function:         divoom_silent_snow_bg_create
  Description:      创建 Silent Snow 表盘背景控件；控件只绘制雪夜渐变、雪花、底部雪丘和呼吸光感，不包含任何文字信息。
  Input:            parent: LVGL 父对象
  Return:           成功返回控件对象，失败返回 NULL
*/
lv_obj_t *divoom_silent_snow_bg_create(lv_obj_t *parent);

/*
  Function:         divoom_silent_snow_bg_set_active
  Description:      控制雪花飘落与呼吸光感动画启停；停止时保留当前画面。
  Input:            obj: Silent Snow 控件对象；active: 1 播放，0 暂停
  Return:           NONE
*/
void divoom_silent_snow_bg_set_active(lv_obj_t *obj, uint8 active);

/*
  Function:         divoom_silent_snow_bg_set_seed
  Description:      设置雪花初始分布种子，用于复现背景或错开多个控件的动画相位。
  Input:            obj: Silent Snow 控件对象；seed: 随机种子
  Return:           NONE
*/
void divoom_silent_snow_bg_set_seed(lv_obj_t *obj, uint32 seed);

#endif

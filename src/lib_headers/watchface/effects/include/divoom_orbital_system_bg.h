#ifndef _divoom_orbital_system_bg_h_
#define _divoom_orbital_system_bg_h_

#include "lvgl.h"
#include "divoom_type.h"

/*
  Function:         divoom_orbital_system_bg_create
  Description:      创建抽象轨道系统背景控件；只绘制深色宇宙底、轨道弧线、中心光球、星点和行星动效，不包含任何文字信息。
  Input:            parent - LVGL 父对象
  Return:           成功返回控件对象，失败返回 NULL
*/
lv_obj_t *divoom_orbital_system_bg_create(lv_obj_t *parent);

/*
  Function:         divoom_orbital_system_bg_set_active
  Description:      设置轨道星体是否刷新；页面切出时可暂停以降低 CPU 开销。
  Input:            obj - 控件对象；active - 1 启动刷新，0 暂停刷新
  Return:           无
*/
void divoom_orbital_system_bg_set_active(lv_obj_t *obj, uint8 active);

/*
  Function:         divoom_orbital_system_bg_set_phase
  Description:      设置轨道动效相位，用于测试固定帧或后续与外部数据时间线同步。
  Input:            obj - 控件对象；phase_ms - 动效相位，单位毫秒
  Return:           无
*/
void divoom_orbital_system_bg_set_phase(lv_obj_t *obj, uint32 phase_ms);

#endif

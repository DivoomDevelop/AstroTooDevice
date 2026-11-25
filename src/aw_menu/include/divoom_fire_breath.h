#ifndef _divoom_fire_breath_h_
#define _divoom_fire_breath_h_

#include "lvgl.h"
#include "divoom_type.h"

/*
  Function:         divoom_fire_breath_create
  Description:      创建壁炉火焰精灵图控件；控件内部只播放 PNG sprite sheet，不使用 SVG 或矢量绘制火苗。
  Input:            parent: LVGL 父对象
  Return:           成功返回控件对象，失败返回 NULL
*/
lv_obj_t *divoom_fire_breath_create(lv_obj_t *parent);

/*
  Function:         divoom_fire_breath_set_active
  Description:      控制火焰呼吸动画启停；停止时保留当前帧，不释放资源。
  Input:            obj: 火焰控件对象；active: 1 播放，0 暂停
  Return:           NONE
*/
void divoom_fire_breath_set_active(lv_obj_t *obj, uint8 active);

/*
  Function:         divoom_fire_breath_set_seed
  Description:      设置火焰初始相位，用于让同一页面内多个火焰错开节奏。
  Input:            obj: 火焰控件对象；seed: 相位种子
  Return:           NONE
*/
void divoom_fire_breath_set_seed(lv_obj_t *obj, uint32 seed);

/*
  Function:         divoom_fire_breath_set_layout
  Description:      设置火焰控件承载区域；PNG 每帧保持原始像素尺寸，区域小于帧尺寸时由父对象裁剪。
  Input:            obj: 火焰控件对象；x/y/w/h: 控件区域位置和尺寸，单位 px
  Return:           NONE
*/
void divoom_fire_breath_set_layout(lv_obj_t *obj, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h);

#endif

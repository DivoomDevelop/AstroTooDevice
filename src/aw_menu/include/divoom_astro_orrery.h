#ifndef _divoom_astro_orrery_h_
#define _divoom_astro_orrery_h_

#include "lvgl.h"
#include "divoom_type.h"

/*
  Function:         divoom_astro_orrery_create
  Description:      创建 AstroToo 太阳系运行控件；控件只绘制太阳、地球、月球及轨道光影。
  Input:            parent - LVGL 父对象
  Return:           成功返回控件对象，失败返回 NULL
*/
lv_obj_t *divoom_astro_orrery_create(lv_obj_t *parent);

/*
  Function:         divoom_astro_orrery_set_active
  Description:      设置控件是否按秒刷新；暂停时保留当前画面。
  Input:            obj - 控件对象；active - 1 启动刷新，0 暂停刷新
  Return:           无
*/
void divoom_astro_orrery_set_active(lv_obj_t *obj, uint8 active);

/*
  Function:         divoom_astro_orrery_set_time_override
  Description:      覆盖 UTC 时间用于固定画面测试；关闭覆盖后使用系统 UTC 时间驱动角度。
  Input:            obj - 控件对象；enable - 1 启用覆盖，0 使用系统时间；utc_sec - 覆盖用 UTC 秒
  Return:           无
*/
void divoom_astro_orrery_set_time_override(lv_obj_t *obj, uint8 enable, uint32 utc_sec);

#endif

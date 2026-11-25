#ifndef _divoom_astrootoo_settings_ex_h_
#define _divoom_astrootoo_settings_ex_h_

#include "divoom_astrootoo_settings.h"
#include "divoom_astrootoo_settings_internal.h"
#include "divoom_astrootoo_language_route.h"

/* 系统设置语言页通过无中间屏闪烁的专用路由包装提交切换事件。 */
#define divoom_menu_control_switch_language divoom_astrootoo_language_route_switch

int divoom_astrootoo_settings_ex_is_page(divoom_astrootoo_settings_page_t page);
void divoom_astrootoo_settings_ex_build(Pdivoom_astrootoo_settings_ctrl phandle);
void divoom_astrootoo_settings_ex_release(Pdivoom_astrootoo_settings_ctrl phandle);
void divoom_astrootoo_settings_ex_group_bind(Pdivoom_astrootoo_settings_ctrl phandle, lv_obj_t *focus_target);

#endif

#include "divoom.h"
#include "divoom_menu_control.h"
#include "divoom_astrootoo_main_menu.h"
#include "divoom_astrootoo_language_route.h"

static int s_language_route_suppress_main_disp;
static int s_language_route_main_disp_suppressed;

/*
 * 语言切换必须先构造新的主菜单作为设置页上级，但不能把该中间页显示出来。
 * 注册表统一经过此包装；普通主菜单显示不受影响，语言路由只跳过一次。
 */
void divoom_astrootoo_language_route_main_disp(void *handle, void *active_obj)
{
    if (s_language_route_suppress_main_disp)
    {
        s_language_route_suppress_main_disp = 0;
        s_language_route_main_disp_suppressed = 1;
        return;
    }
    divoom_astrootoo_main_menu_disp(handle, active_obj);
}

int divoom_astrootoo_language_route_switch(int lang, int root_menu_id, int target_menu_id)
{
    int result;

    s_language_route_suppress_main_disp = 1;
    s_language_route_main_disp_suppressed = 0;
    result = divoom_menu_control_switch_language(lang, root_menu_id, target_menu_id);
    if (!result)
    {
        /* 入队失败时撤销一次性状态，不能吞掉后续正常进入主菜单的显示。 */
        s_language_route_suppress_main_disp = 0;
    }
    return result;
}

int divoom_astrootoo_language_route_was_main_disp_suppressed(void)
{
    return s_language_route_main_disp_suppressed;
}

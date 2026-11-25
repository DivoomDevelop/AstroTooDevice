#ifndef _divoom_astrootoo_menu_registry_h_
#define _divoom_astrootoo_menu_registry_h_

/*
 * AstroToo 菜单注册宏：与 divoom_menu_control + push_flag 栈语义一致。
 * push_flag=1：load_next 压栈，ESC/return_pre 弹栈；push_flag=0：替换当前层（表盘根/开机全屏）。
 * 屏清单 push_flag 真源：.cursor/astrootoo-ui/screen-registry.yaml
 */
#define ASTRO_MENU_ROOT(mid, init, clean, disp, pause) \
    { 0, 0, (mid), (init), (clean), (disp), (pause) }

#define ASTRO_MENU_STACK(key_flag, mid, init, clean, disp, pause) \
    { (key_flag), 1, (mid), (init), (clean), (disp), (pause) }

#define ASTRO_MENU_OVERLAY(key_flag, mid, init, clean, disp, pause) \
    { (key_flag), 0, (mid), (init), (clean), (disp), (pause) }

#endif

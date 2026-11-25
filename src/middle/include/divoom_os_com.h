#ifndef _divoom_os_com_h_
#define _divoom_os_com_h_

/*
 * 关机关屏：停 LVGL flush 后走 disp close_flow（bl → 停 video → 0x28/0x10 → RST）。
 * 可重复调用；已 disable 时直接返回。模拟器为空操作。
 */
void divoom_os_com_lcd_disable(void);

/* Caller must hold the LVGL lock through the check and hardware submission. */
int divoom_os_com_lcd_is_disabled(void);

#endif

#ifndef _divoom_lvgl_top_leave_msg_notice_menu_h_
#define _divoom_lvgl_top_leave_msg_notice_menu_h_

#include <stdint.h>

/** 顶部系统通知条：保存留言成功后提示（参考 divoom_lvgl_top_anniversary_reminder_menu_init） */
/**
 * 顶部留言保存成功提示条。内部调用 divoom_system_msg_notice_add（会自行加/解 lvgl 锁）。
 * 任意键先播放刚保存的留言；时长行在未开播前显示总时长，解码开始后按剩余时间倒计时；
 * 播放结束后等同原 ENTER（回表盘并关闭条）。
 * 磁盘只有一条留言：已有顶栏时刷新同一张卡（时长/待播），不再 notice_add 叠第二条。
 */
void divoom_lvgl_top_leave_msg_notice_init(uint32_t duration_sec);
int divoom_lvgl_top_leave_msg_notice_is_playing(void);

#endif

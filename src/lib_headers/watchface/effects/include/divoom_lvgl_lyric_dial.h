#ifndef _divoom_lvgl_lyric_dial_h_
#define _divoom_lvgl_lyric_dial_h_

#include "divoom_device_json.h"

/* 歌词表盘沿用普通表盘配置的底图、视频和其它元素，只单独管理全屏歌词元素。 */
void *divoom_lvgl_lyric_dial_init(void *user_data, int menu_id,
    Pdivoom_clock_manage_item clock_info);
void divoom_lvgl_lyric_dial_disp(void *handle, void *active);
void divoom_lvgl_lyric_dial_pause(void *handle);
void divoom_lvgl_lyric_dial_exit(void *handle);

/* 持 LVGL 锁查询：只有当前活动歌词页确实使用视频显示模式时返回 1。 */
uint8 divoom_lvgl_lyric_dial_is_video_active(void);

#endif

#ifndef _divoom_lvgl_lyric_effect_h_
#define _divoom_lvgl_lyric_effect_h_

#include "divoom.h"

void *divoom_lvgl_lyric_effect_init(void *user_data, int menu_id);
void divoom_lvgl_lyric_effect_disp(void *handle, void *active_obj);
void divoom_lvgl_lyric_effect_pause(void *handle);

void divoom_lvgl_lyric_effect_exit(void *handle);

/** 是否为歌词表盘 clock_id */
int divoom_lvgl_lyric_effect_is_dial_clock_id(int clock_id);
/** clock_id → scene_index(0~11)；无效返回 -1 */
int divoom_lvgl_lyric_effect_scene_from_clock_id(int clock_id);
/** 表盘轮播切换时更新场景（背景视频 + 歌词特效） */
void divoom_lvgl_lyric_effect_set_dial_clock_id(void *handle, int clock_id);

/** 氛围歌词背景视频总数 */
int divoom_lvgl_lyric_effect_bg_count(void);

/** 按 1-based 序号取背景 mp4/mov 绝对路径；index 无效时返回 NULL */
const char *divoom_lvgl_lyric_effect_bg_name(int index);

#endif


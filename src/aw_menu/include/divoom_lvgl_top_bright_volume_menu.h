#ifndef _divoom_lvgl_top_bright_volume_menu_h_
#define _divoom_lvgl_top_bright_volume_menu_h_

/* bright_volume_flag: 0/1 旋钮记忆  2 视频音量  3 提示音  4 强制音乐音量(外部同步) */
#define DIVOOM_LVGL_TOP_BRIGHT_VOLUME_MENU_FORCE_MUSIC  4

void divoom_lvgl_top_bright_volume_menu_init(int bright_volume_flag);

void divoom_lvgl_top_bright_volume_menu_exit(void);

#ifdef DIVOOM_PRODUCT_ASTRO_TOO
/** 音量 +/- 短按、长按调亮度；返回 1 表示已消费按键 */
int divoom_lvgl_top_bright_volume_menu_handle_key(uint8 key_value, uint8 key_type);
/** JL 0x6F 上报：通话中刷新 OSD */
void divoom_lvgl_top_bright_volume_menu_on_hfp_call_volume(uint8 ui_percent);
/* 持 LVGL 锁：视频歌词复用正常音量条的颜色、PNG 图标和计时，只改变合成方式。 */
void divoom_lvgl_top_bright_volume_menu_show_composited(int brightness_mode, int fill_percent);
/* 借用根对象供歌词 OSD 合成；退出函数只清理本模块的合成音量条。 */
void *divoom_lvgl_top_bright_volume_menu_get_composited_obj(void);
void divoom_lvgl_top_bright_volume_menu_exit_composited(void);
#endif

#endif

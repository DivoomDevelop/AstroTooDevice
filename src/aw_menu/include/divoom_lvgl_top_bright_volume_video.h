#ifndef _DIVOOM_LVGL_TOP_BRIGHT_VOLUME_VIDEO_H_
#define _DIVOOM_LVGL_TOP_BRIGHT_VOLUME_VIDEO_H_

/* 助眠视频播放时使用的独立硬件 TOP OSD。brightness_mode: 0 音量，1 亮度。 */
void divoom_lvgl_top_bright_volume_video_show(int brightness_mode, int fill_percent);
void divoom_lvgl_top_bright_volume_video_exit(void);

#endif

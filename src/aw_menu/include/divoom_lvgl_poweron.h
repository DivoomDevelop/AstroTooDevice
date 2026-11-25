#ifndef _divoom_lvgl_poweron_h_
#define _divoom_lvgl_poweron_h_

#define DIVOOM_LVGL_POWER_ON_FILE_NAME "/usr/share/divoom_app/poweron_movie/power_on.mp4"

void *divoom_lvgl_poweron_init(void *user_data, int menu_id);
void divoom_lvgl_poweron_disp(void *handle, void *active_obj);
void divoom_lvgl_poweron_pause(void *handle);
void divoom_lvgl_poweron_exit(void *handle);

int divoom_lvgl_poweron_is_video_phase(void);
int divoom_lvgl_poweron_will_play_video(void);
/** 开机动画尚未结束（含即将播放）：抑制连接提示音、音量 OSD 等开机干扰 */
int divoom_lvgl_poweron_is_boot_pending(void);
int divoom_lvgl_poweron_should_defer_connect_tone(void);
/** 发 0x5f、结束 boot pending、补跑延后的 0x19；不解 mute（闹钟 HOLD 跳过动画用） */
void divoom_lvgl_poweron_notify_jl_audio_ready(void);
/** 开机门控结束：上者 + 解 JL 媒体/通话 mute。视频结束与 OOBE 跳过动画共用。 */
void divoom_lvgl_poweron_notify_boot_ready(void);
void divoom_lvgl_poweron_on_boot_video_exited(void);

#endif

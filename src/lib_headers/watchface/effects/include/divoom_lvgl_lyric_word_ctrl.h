#ifndef _divoom_lvgl_lyric_word_ctrl_h_
#define _divoom_lvgl_lyric_word_ctrl_h_

/** Compatibility setting for UART lyrics (type=1: line, type=2: title).
 *  Network lyrics retrieval is unavailable in the open-source SDK,
 *  regardless of this macro's value. */
#ifndef DIVOOM_LYRIC_USE_UART_ONLY
#define DIVOOM_LYRIC_USE_UART_ONLY  1
#endif

void *divoom_lvgl_lyric_word_ctrl_init(void *main_obj);
/** Compatibility fetch handle without UI; network retrieval is unavailable. */
void *divoom_lvgl_lyric_word_ctrl_init_fetch(void);
void divoom_lvgl_lyric_word_ctrl_add_word(void *handle, char *music_name);
/** 按播放毫秒查当前句和上一句，不触发 add_word。命中当前句返回 1。 */
int divoom_lvgl_lyric_word_ctrl_peek_line_at(void *handle, uint32_t time_ms, char *cur, uint32_t cur_sz, char *prev, uint32_t prev_sz);
/** UART type=1 逐句只用于校时，不覆盖网络歌词文案。 */
void divoom_lvgl_lyric_word_ctrl_note_uart_line(void *handle, const char *text, uint32_t pos_ms);
/** 进度回跳时丢掉校时样本和 offset。 */
void divoom_lvgl_lyric_word_ctrl_reset_calib(void *handle, uint32_t old_pos, uint32_t new_pos);

/** 清空当前歌词特效层上的行（须在 LVGL 线程调用；蓝牙切歌见 lyric_effect 内标志） */
void divoom_lvgl_lyric_word_ctrl_clear_disp_lines(void *handle);

/** Update song metadata; all network lyrics retrieval is disabled in the open-source SDK. */
void divoom_lvgl_lyric_word_ctrl_set_artist_track(void *handle, const char *artist_name, const char *track_name,
    const char *album_name, int duration_sec);

/** 传入当前播放时间（秒），根据已拉取的歌词时间表查当前行并触发 add_word 更新显示 */
void divoom_lvgl_lyric_word_ctrl_set_playback_time(void *handle, float time_sec);
/** Fetch status: open-source SDK returns state=-1 (unavailable); ver counts metadata changes. */
void divoom_lvgl_lyric_word_ctrl_get_fetch_state(void *handle, int *state, int *ver);
/** 切歌后丢掉上一首歌词表并作废进行中的请求，不发新的取词。总时长到了再 set_artist_track。 */
void divoom_lvgl_lyric_word_ctrl_drop_song(void *handle);
/** 已取用的时长（秒）和是否已有同步词。总时长变化时用来决定要不要重取。 */
void divoom_lvgl_lyric_word_ctrl_get_duration(void *handle, int *duration_sec, int *synced);
void divoom_lvgl_lyric_word_ctrl_auto_disp_mode(void *handle, int flag);

/** 切换背景场景 + 场景内字体效果索引；多行效果未播完时会延后（pending）
 *  effect_index：>=0 指定效果；DIVOOM_LYRIC_WORD_EFFECT_INDEX_RANDOM(-1) 随机（尽量避免与 avoid 重复） */
#define DIVOOM_LYRIC_WORD_EFFECT_INDEX_RANDOM   (-1)

void divoom_lvgl_lyric_word_ctrl_change_selected_scene(void *handle, int scene_index, int effect_index);
/** 兼容：切换背景 scene，效果从该 scene 内随机选取（仅多 effect 时有效） */
void divoom_lvgl_lyric_word_ctrl_change_selected_mode(void *handle, int scene_index);

/** 是否有延后中的场景/效果切换（MUL_SLASH/SLASH 等多行未填满） */
int divoom_lvgl_lyric_word_ctrl_is_switch_pending(void *handle);
/** 取并清除「需切换背景视频」的 scene 索引；无则返回 -1 */
int divoom_lvgl_lyric_word_ctrl_consume_pending_bg_switch(void *handle);

void divoom_lvgl_lyric_word_ctrl_exit(void *handle);
#endif

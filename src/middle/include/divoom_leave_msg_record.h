#ifndef _divoom_leave_msg_record_h_
#define _divoom_leave_msg_record_h_

#include <stddef.h>
#include <stdint.h>

#include "divoom_paths.h"

/** 录音过程临时文件（MP3，JL 16k/mono/32k 或模拟器 pcm_record 编码） */
#define DIVOOM_LEAVE_MSG_RECORD_TMP_MP3 DIVOOM_TMP_DIR "/divoom_leave_msg_cap.mp3"

/** 兼容旧名 */
#define DIVOOM_LEAVE_MSG_RECORD_TMP_WAV DIVOOM_LEAVE_MSG_RECORD_TMP_MP3

#define DIVOOM_LEAVE_MSG_MIN_CLIP_BYTES 256

void divoom_leave_msg_record_stop_capture(void);
int divoom_leave_msg_record_start_capture(void);
int divoom_leave_msg_record_capture_is_active(void);

void divoom_leave_msg_record_delete_temp_file(void);
int divoom_leave_msg_record_temp_clip_bytes(void);

int divoom_leave_msg_record_commit_to_leave_msg_file(uint32_t duration_sec);
/** 写 leave_msg_last.meta（时长 + 当天月日）；App 下载完成与本机 commit 共用 */
void divoom_leave_msg_record_write_last_saved_meta(uint32_t duration_sec);
int divoom_leave_msg_record_read_last_saved_meta(uint32_t *duration_sec, int *mon, int *mday);
void divoom_leave_msg_record_delete_saved_on_disk(void);
int divoom_leave_msg_record_get_playback_path(char *path_buf, size_t path_buf_len);
/** 留言试听音量（暂时跟当前媒体音量，与机身 +/- 一致） */
int divoom_leave_msg_record_get_play_volume(void);
void divoom_leave_msg_record_stop_preview_playback(void);
/** 文件头可识别为 MP2/MP3/AAC/WAV 且大小达标 */
int divoom_leave_msg_record_clip_path_is_playable(const char *path);
/** 顶栏试听播完后：先停播再删 tmp + 240.bin/meta（播放中勿调） */
void divoom_leave_msg_record_safe_discard_after_notice_play(void);

#endif

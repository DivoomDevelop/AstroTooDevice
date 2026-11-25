#ifndef _divoom_pcm_record_h_
#define _divoom_pcm_record_h_

#include "divoom_type.h"

typedef enum
{
	DIVOOM_PCM_RECORD_FMT_WAV = 0,
	DIVOOM_PCM_RECORD_FMT_MP3 = 1,
} divoom_pcm_record_format_e;

/** 录音完成回调：result 0=成功，-1=失败或未录到有效数据 */
typedef void (*divoom_pcm_record_done_callback)(const char *file_path, uint32 duration_sec, int result);

/**
 * 启动 PCM 录音（通过 divoom_pcm_set_callback 采集）。
 * @param file_path       输出文件路径（WAV 或 MP3）
 * @param duration_sec    录音时长（秒），0 表示不限时，需手动 divoom_pcm_record_stop
 * @param format          DIVOOM_PCM_RECORD_FMT_WAV / DIVOOM_PCM_RECORD_FMT_MP3
 * @param mp3_bitrate_kbps MP3 码率 kbps，WAV 时可传 0；MP3 传 0 则用默认 64
 * @param done_cb         录音结束回调（自动到时或 stop 后调用，可为 NULL）
 * @return 0 成功，-1 失败
 */
int divoom_pcm_record_start(const char *file_path, uint32 duration_sec,
			    divoom_pcm_record_format_e format, int mp3_bitrate_kbps,
			    divoom_pcm_record_done_callback done_cb);

/** 停止录音，写回文件尾并恢复默认 PCM 回调（finalize 在后台线程） */
void divoom_pcm_record_stop(void);

/** 停止并阻塞直到 MP3/WAV 落盘完成（提交录音文件前必须调用） */
void divoom_pcm_record_stop_sync(void);

/** 等待录音 finalize 线程结束 */
void divoom_pcm_record_wait_idle(void);

/** 是否正在录音 */
int divoom_pcm_record_is_active(void);

#endif

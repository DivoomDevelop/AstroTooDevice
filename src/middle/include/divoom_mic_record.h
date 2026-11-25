#ifndef _divoom_mic_record_h_
#define _divoom_mic_record_h_

#include "divoom_type.h"

/** 录音完成回调：result 0=成功，-1=失败或未录到有效数据 */
typedef void (*divoom_mic_record_done_fn)(const char *path, uint32 duration_sec, int result);

/**
 * 启动 MIC 录音（真机 JL btcore / 模拟器 pcm_record）。
 * @param path         输出文件路径（真机 MP3）
 * @param duration_sec 最长秒数，0=不限（JL 侧 60s 封顶）
 * @param done_cb      结束回调，可为 NULL
 */
int divoom_mic_record_start(const char *path, uint32 duration_sec,
			    divoom_mic_record_done_fn done_cb);

void divoom_mic_record_stop(void);
void divoom_mic_record_stop_sync(void);
int divoom_mic_record_is_active(void);

/* Optional observer: 0=mono signed PCM16, 1=MP3. The callback must only copy
 * into a bounded queue; never stop recording or change this observer in it.
 * Register before starting; unregister after stop_sync (waits for callbacks).
 * A second observer is rejected. Open-source builds return -1 / do nothing. */
typedef void (*divoom_mic_record_stream_fn)(const void *, unsigned, int, unsigned);
int divoom_mic_record_set_stream_callback(divoom_mic_record_stream_fn);
int divoom_mic_record_stream_is_current(divoom_mic_record_stream_fn);
void divoom_mic_record_stream_stop_sync(divoom_mic_record_stream_fn);
/* Backend hook; does not change legacy recording behavior. */
void divoom_mic_record_stream_data(const void *, unsigned, int, unsigned);

/** 32kbps CBR MP3：按文件大小四舍五入到秒（落盘后回看用；录音中请用墙钟）。 */
#define DIVOOM_MIC_RECORD_MP3_CBR32_BPS  4000u

static inline uint32 divoom_mic_record_mp3_cbr32_sec(int file_bytes)
{
	uint32 sec;

	if (file_bytes < 128)
		return 0u;
	sec = ((uint32)file_bytes + (DIVOOM_MIC_RECORD_MP3_CBR32_BPS / 2u))
	      / DIVOOM_MIC_RECORD_MP3_CBR32_BPS;
	return sec > 0u ? sec : 1u;
}

/**
 * 落盘文件回看时长（秒）。
 * ADTS AAC（APP 闹钟留言）：按帧长/采样率估算，对齐杰理
 * `divoom_os_com_music_file_get_duration_sec`。
 * 其它（本地 MIC 32kbps MP3）：按 CBR 文件大小。
 */
uint32 divoom_mic_record_file_duration_sec(const char *path);

#endif

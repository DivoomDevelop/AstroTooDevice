#ifndef _divoom_local_music_ctrl_h_

#define _divoom_local_music_ctrl_h_


enum{
    DIVOOM_LOCAL_MUSIC_TYPE_PLAY,
    DIVOOM_LOCAL_MUSIC_TYPE_SET_VOLUME,
    DIVOOM_LOCAL_MUSIC_TYPE_STOP,
    DIVOOM_LOCAL_MUSIC_TYPE_PAUSE,
    DIVOOM_LOCAL_MUSIC_TYPE_RESUME,
    /** Step5a：切歌控制项，由 local_tone 线程处理（勿在调用方线程 request_stop） */
    DIVOOM_LOCAL_MUSIC_TYPE_CUT,
};

void divoom_local_music_ctrl_music_init(void);
/*********************************************************************
  Function:        divoom_local_music_ctrl_get_music_status
  Description:     获取当前播放状态
  Input:           
  Return:          1：播放；0：暂停
**********************************************************************/

int divoom_local_music_ctrl_get_music_status(void);

/*********************************************************************
  Function:        divoom_local_music_ctrl_get_tone_status
  Description:     获取当前提示音播放状态
  Input:           
  Return:          1：播放；0：暂停
**********************************************************************/
int divoom_local_music_ctrl_get_tone_status(void);

/** 提示音是否正在播放指定路径（playhandle 有效且路径一致）。 */
int divoom_local_music_ctrl_is_tone_playing_path(const char *music_name);

/** 无提示音在播且队列无待发项（可安全恢复 DIY snd_pcm）。 */
int divoom_local_music_ctrl_tone_idle_for_resume(void);

//status:1:正常播放结束 ；0：文件异常
typedef int divoom_local_music_ctrl_music_play_get_next(char *music_name, int status);

/*********************************************************************
  Function:        divoom_local_music_ctrl_music_play
  Description:     指定设备播放某个音频文件
  Input:           music_name
                   vol           音量 0-100
                   pnext_callback 播放完了触发回调通知上面,如果返回0，将结束播放，否则继续启动播放
  Return:          none
**********************************************************************/
void divoom_local_music_ctrl_music_play(char *music_name, uint8 vol, divoom_local_music_ctrl_music_play_get_next *pnext_callback);

/*********************************************************************
  Function:        divoom_local_music_ctrl_set_music_volume
  Description:     设置提示音播放音量
  Input:           volume
  Return:          none
**********************************************************************/

void divoom_local_music_ctrl_set_music_volume(int volume);

/*********************************************************************
  Function:        divoom_local_music_ctrl_get_music_volume
  Description:     获取当前音乐音量（0-100）
  Input:           none
  Return:          当前音量，未初始化返回 0
**********************************************************************/

int divoom_local_music_ctrl_get_music_volume(void);

/*********************************************************************
  Function:        divoom_local_music_ctrl_get_music_position_sec
  Description:     当前音乐播放位置（秒），未播放返回 0
**********************************************************************/
int divoom_local_music_ctrl_get_music_position_sec(void);

/*********************************************************************
  Function:        divoom_local_music_ctrl_get_music_duration_sec
  Description:     当前音乐总时长（秒），未知返回 0
**********************************************************************/
int divoom_local_music_ctrl_get_music_duration_sec(void);

/*********************************************************************
  Function:        divoom_local_music_ctrl_is_music_active
  Description:     本地音乐是否已加载（含暂停），仿真用于 TF 进度同步
**********************************************************************/
int divoom_local_music_ctrl_is_music_active(void);

/*********************************************************************
  Function:        divoom_local_music_ctrl_stop_music
  Description:     停止播放音乐
  Input:           none
  Return:          none
**********************************************************************/


void divoom_local_music_ctrl_stop_music(void);

void divoom_local_music_ctrl_stop_music_and_wait(unsigned int timeout_ms);

#if DIVOOM_SHOW_MODE
/* 格式化 /userdata 前停掉展示机曲目，并等到 mp3 句柄关掉。 */
void divoom_local_music_ctrl_show_release_for_format(void);
#endif


/*********************************************************************
  Function:        divoom_local_music_ctrl_music_ctrl
  Description:     控制播放
  Input:           play     1：播放；0：暂停
                   none
  Return:          none
**********************************************************************/



void divoom_local_music_ctrl_music_ctrl(uint8 play);

/*********************************************************************
  Function:        divoom_local_music_ctrl_tone_play
  Description:     播放某个提示音文件，如果有音乐在播放，将音乐的音量降低一半，再播放提示音
  Input:           music_name
                   volume      音量，最大100
                   pnext_callback
                   play_time        播放多长时间结束
  Return:          none
**********************************************************************/

int divoom_local_music_ctrl_tone_play(char *music_name, int volume,divoom_local_music_ctrl_music_play_get_next *pnext_callback, int play_time);

/*********************************************************************
  Function:        divoom_local_music_ctrl_set_tone_volume
  Description:     设置提示音播放音量
  Input:           volume
  Return:          none
**********************************************************************/

void divoom_local_music_ctrl_set_tone_volume(int volume);

/*********************************************************************
  Function:        divoom_local_music_ctrl_stop_tone
  Description:     停止播放提示音
  Input:           none
  Return:          none
**********************************************************************/

void divoom_local_music_ctrl_stop_tone(void);

/** Phase0 P5：local_tone 队列与播放快照（供 audio_diag） */
void divoom_local_music_ctrl_tone_diag_print(void);

#include "divoom_mic_record.h"

/** 用户语音片段录音（留言/闹钟/助眠），与 tone_play 同属提示音域；业务勿直调 divoom_mic_record。 */
int divoom_local_music_ctrl_voice_clip_record_start(const char *path, uint32 duration_sec,
						    divoom_mic_record_done_fn done_cb);
void divoom_local_music_ctrl_voice_clip_record_stop(void);
void divoom_local_music_ctrl_voice_clip_record_stop_sync(void);
int divoom_local_music_ctrl_voice_clip_record_is_active(void);

#endif



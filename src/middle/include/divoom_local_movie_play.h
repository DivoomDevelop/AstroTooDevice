#ifndef _divoom_local_movie_play_h_
#define _divoom_local_movie_play_h_

/*********************************************************************
  Function:        divoom_local_movie_play_get_music_status
  Description:     获取当前播放状态
  Input:           
  Return:          1：播放；0：暂停
**********************************************************************/

int divoom_local_movie_play_get_movie_status(void *handle);

/*********************************************************************
  Function:        divoom_local_movie_play_play
  Description:     指定设备播放某个音频文件
  Input:           music_name
                   pnext_callback: status=0 为重播询问，返回0原地重播；status=1为释放后通知
  Return:          none
**********************************************************************/
typedef int divoom_local_movie_play_play_get_next(void *para,int status);

/* status=0 may return this to retain the last frame until play/stop is called. */
#define DIVOOM_LOCAL_MOVIE_HOLD_LAST_FRAME 2

void divoom_local_movie_play_play(char *music_name, int volume,int play_time, divoom_local_movie_play_play_get_next *pnext_callback, void *para);

/*********************************************************************
  Function:        divoom_local_movie_play_play_with_pos
  Description:     带显示区域的指定设备播放某个视频文件,如果视频时间少于play_time，将循环播放
  Input:           music_name
                   pnext_callback: status=0 为重播询问，返回0原地重播；status=1为释放后通知
  Return:          none
**********************************************************************/

void divoom_local_movie_play_play_with_pos(char *music_name, int volume,int play_time, divoom_local_movie_play_play_get_next *pnext_callback, void *para,
                                            int start_x, int start_y, int width, int high);

/*********************************************************************
  Function:        divoom_local_movie_play_stop_music
  Description:     停止播放音乐
  Input:           none
  Return:          none
**********************************************************************/


void divoom_local_movie_play_set_movie_volume(uint8 volume);

/*********************************************************************
  Function:        divoom_local_movie_play_is_playing
  Description:     是否在播放音乐
  Input:           none
  Return:          1 or 0
**********************************************************************/

int divoom_local_movie_play_is_playing(void);

/* 等待 VO 首帧开始送显；成功返回 0，超时或播放退出返回 -1。 */
int divoom_local_movie_play_wait_rendering(unsigned int timeout_ms);

int divoom_local_movie_play_is_rendering(void);
/* Pauses playback and copies the displayed NV12/NV21 frame into RGB565 + A8. */
int divoom_local_movie_play_copy_last_frame(uint8 *buffer, int width, int height);

/* 等待 MPP 播放线程完成销毁；成功返回 0，超时返回 -1。 */
int divoom_local_movie_play_wait_stopped(unsigned int timeout_ms);
/* Nonblocking query: true only after pipeline and all player allocations are freed. */
int divoom_local_movie_play_is_idle(void);

/*********************************************************************
  Function:        divoom_local_movie_play_stop_music
  Description:     停止播放音乐
  Input:           none
  Return:          none
**********************************************************************/


void divoom_local_movie_play_stop_movie(void);

/* 停止视频并清理内部状态，不触发播放结束回调（用于页面切换时的安全销毁） */
void divoom_local_movie_play_stop_movie_no_cb(void);

/* Stop only this callback owner, then wait until its final callback has ended. */
int divoom_local_movie_play_stop_owner(void *owner, unsigned int timeout_ms);

/*********************************************************************
  Function:        divoom_local_movie_play_ctrl
  Description:     控制播放
  Input:           play     1：播放；0：暂停
                   none
  Return:          none
**********************************************************************/



void divoom_local_movie_play_ctrl(uint8 play);

/*********************************************************************
  Function:        divoom_local_movie_play_have_handle
  Description:     是否有播放句柄
  Input:           NONE
                   none
  Return:          1 OR 0
**********************************************************************/



int divoom_local_movie_play_have_handle(void);

/*********************************************************************
  Function:        divoom_local_movie_play_seek
  Description:     定位播放
  Input:           start_time     以秒为单位
                   none
  Return:          none
**********************************************************************/



void divoom_local_movie_play_seek(int start_time);

int divoom_local_movie_play_is_album_slideshow(void);
int divoom_local_movie_play_should_keep_jl_tone_volume(void);

#endif



#ifndef _divoom_lvgl_movie_play_h_
#define _divoom_lvgl_movie_play_h_

typedef int divoom_lvgl_movie_play_play_get_next(void *user_data, int status);

/*********************************************************************
  Function:        divoom_lvgl_movie_play_play
  Description:     指定LVGL对象播放某个视频文件
  Input:           video_obj
                   image_w         控件宽度
                   image_h         控件高度
                   music_name      播放文件名
                   volume          播放音量
                   play_time       播放时间
                   pnext_callback: status=0 为重播询问，返回0原地重播；status=1为释放后通知
  Return:          相关句柄
**********************************************************************/

void *divoom_lvgl_movie_play_play(void *video_obj, void *user_data,int start_x, int start_y,int image_w, int image_h,
                                 char *music_name, int volume, int start_time, int play_time, divoom_lvgl_movie_play_play_get_next *pnext_callback);

/*********************************************************************
  Function:        divoom_lvgl_movie_play_set_volume
  Description:     设置视频播放的音量
  Input:           handle
                   volume
  Return:          none
**********************************************************************/

void divoom_lvgl_movie_play_set_volume(void *handle, int volume);
                                 
/*********************************************************************
  Function:        divoom_lvgl_movie_play_stop
  Description:     停止播放视频，释放相关资源
  Input:           handle
  Return:          none
**********************************************************************/

void divoom_lvgl_movie_play_stop(void *handle, uint8 *image_buf);
/*********************************************************************
  Function:        divoom_lvgl_movie_play_ctrl
  Description:     播放与暂停视频，释放相关资源
  Input:           handle
                   status
  Return:          none
**********************************************************************/

void divoom_lvgl_movie_play_ctrl(void *handle, int status);

#endif




#ifndef _divoom_sonos_ctrl_h_
#define _divoom_sonos_ctrl_h_

/*********************************************************************
  Function:         divoom_sonos_ctrl_get_play_status
  Description:      获取sonos的播放状态
  Input:            base_url:
                    phead_info
                    head_num
  Return:           1:play; 0:pause
  Author:           albert
**********************************************************************/

int divoom_sonos_ctrl_get_play_status(char *base_url, Pdivoom_http_client_url_head_info phead_info, int head_num);

/*********************************************************************
  Function:         divoom_sonos_ctrl_set_play_status
  Description:      控制sonos的播放状态
  Input:            base_url:
                    phead_info
                    head_num
                    play_ctrl  ：1：play: 0: puase
  Return:           none
  Author:           albert
**********************************************************************/

int divoom_sonos_ctrl_set_play_ctrl(char *base_url, Pdivoom_http_client_url_head_info phead_info, int head_num, int play_ctrl);

/*********************************************************************
  Function:         divoom_sonos_ctrl_play_next_pre
  Description:      sonos的上下首歌曲切换
  Input:            base_url:
                    phead_info
                    head_num
                    play_ctrl  ：1：next: 0: pre
  Return:           none
  Author:           albert
**********************************************************************/

int divoom_sonos_ctrl_play_next_pre(char *base_url, Pdivoom_http_client_url_head_info phead_info, int head_num, int play_ctrl);

/*********************************************************************
  Function:         divoom_sonos_ctrl_get_music_name
  Description:      获取播放歌曲名称
  Input:            base_url:
                    phead_info
                    head_num
                    music_name  ：当前播放歌曲名称
  Return:           none
  Author:           albert
**********************************************************************/

int divoom_sonos_ctrl_get_music_name(char *base_url, Pdivoom_http_client_url_head_info phead_info, int head_num, char *music_name);


/*********************************************************************
  Function:         divoom_sonos_ctrl_set_play_volume
  Description:      设置播放音量
  Input:            base_url:
                    phead_info
                    head_num
                    flag  ：1:加音量；0：减少音量
  Return:           none
  Author:           albert
**********************************************************************/

int divoom_sonos_ctrl_set_play_volume(char *base_url, Pdivoom_http_client_url_head_info phead_info, int head_num, uint8 flag);

/*********************************************************************
  Function:         divoom_sonos_ctrl_key_do_with
  Description:      SONOS按键处理, 
  Input:            base_url:
                    phead_info
                    head_num
                    key_flag  ：需要执行的操作 PlayPre, PlayNext, VolAdd, VolDec, PlayMuted, PlayCtrl
  Return:           none
  Author:           albert
************************************************************************/

void divoom_sonos_ctrl_key_do_with(char *base_url, Pdivoom_http_client_url_head_info phead_info, int head_num, char *key_flag);

#endif



#include "divoom.h"

/*********************************************************************
  Function:         divoom_sonos_ctrl_get_play_status
  Description:      获取sonos的播放状态
  Input:            base_url:
                    phead_info
                    head_num
  Return:           1:play; 0:pause
  Author:           albert
**********************************************************************/

int divoom_sonos_ctrl_get_play_status(char *base_url, Pdivoom_http_client_url_head_info phead_info, int head_num)
{
    //https://api.ws.sonos.com/control/api/v1/groups/groupId/playback
    char *url_info, *net_data, play_status[64];
	divoom_cJSON *root = NULL;

    url_info = divoom_system_malloc(256);
    if(url_info == NULL)
    {
        return 0;
    }
    strcpy(url_info, base_url);
    strcat(url_info, "/playback");

    net_data = divoom_http_send_data(url_info, NULL, 0, phead_info, head_num);
    divoom_system_free(url_info);
    if(net_data == NULL)
    {
        return 0;
    }

	root = divoom_cJSON_Parse(net_data);
    divoom_system_free(net_data);
	if (!root)
	{
		return 0;
	}
    divoom_cJSON_GetObjectString(root, "playbackState", play_status);
    divoom_cJSON_Delete(root);
    if(strlen(play_status) == 0)
    {
		return 0;        
    }
    if(strcmp(play_status, "PLAYBACK_STATE_IDLE") == 0 ||
        strcmp(play_status, "PLAYBACK_STATE_PAUSED") == 0)
    {
        return 0;
    }
    return 1;    
}

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

int divoom_sonos_ctrl_set_play_ctrl(char *base_url, Pdivoom_http_client_url_head_info phead_info, int head_num, int play_ctrl)
{
    //https://api.ws.sonos.com/control/api/v1/groups/groupId/playback
    char *url_info, *net_data;

    url_info = divoom_system_malloc(256);
    if(url_info == NULL)
    {
        return 0;
    }
    strcpy(url_info, base_url);
    if(play_ctrl)
    {
        strcat(url_info, "/play");
    }
    else
    {
        strcat(url_info, "/pause");
    }

    net_data = divoom_http_send_data(url_info, NULL, 1, phead_info, head_num);
    divoom_system_free(url_info);
    if(net_data == NULL)
    {
        return 0;
    }
    divoom_system_free(net_data);
    return 1;

}

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

int divoom_sonos_ctrl_play_next_pre(char *base_url, Pdivoom_http_client_url_head_info phead_info, int head_num, int play_ctrl)
{
    //https://api.ws.sonos.com/control/api/v1/groups/groupId/playback/skipToNextTrac
    //https://api.ws.sonos.com/control/api/v1/groups/groupId/playback/skipToPreviousTrack
    char *url_info, *net_data;

    url_info = divoom_system_malloc(256);
    if(url_info == NULL)
    {
        return 0;
    }
    strcpy(url_info, base_url);
    if(play_ctrl)
    {
        strcat(url_info, "/playback/skipToNextTrac");
    }
    else
    {
        strcat(url_info, "/playback/skipToPreviousTrack");
    }

    net_data = divoom_http_send_data(url_info, NULL, 1, phead_info, head_num);
    divoom_system_free(url_info);
    if(net_data == NULL)
    {
        return 0;
    }
    divoom_system_free(net_data);
    return 1;

}

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

int divoom_sonos_ctrl_get_music_name(char *base_url, Pdivoom_http_client_url_head_info phead_info, int head_num, char *music_name)
{
    //https://api.ws.sonos.com/control/api/v1/groups/groupId/playbackMetadata
    char *url_info, *net_data;
	divoom_cJSON *root = NULL, *obj;

    url_info = divoom_system_malloc(256);
    if(url_info == NULL)
    {
        return 0;
    }
    strcpy(url_info, base_url);
    strcat(url_info, "/playbackMetadata");

    net_data = divoom_http_send_data(url_info, NULL, 0, phead_info, head_num);
    divoom_system_free(url_info);
    
    divoom_system_free(net_data);
    if(net_data == NULL)
    {
        return 0;
    }
    

	root = divoom_cJSON_Parse(net_data);
    divoom_system_free(net_data);
	if (!root)
	{
		return 0;
	}
    obj = divoom_cJSON_GetObjectItem(root, "currentItem");
    if(obj == NULL)
    {
        divoom_cJSON_Delete(root);
		return 0;
    }
    obj = divoom_cJSON_GetObjectItem(obj, "track");
    if(obj == NULL)
    {
        divoom_cJSON_Delete(root);
		return 0;
    }
    divoom_cJSON_GetObjectString(obj, "name", music_name);
    divoom_cJSON_Delete(root);
    return 1;

}


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

int divoom_sonos_ctrl_set_play_volume(char *base_url, Pdivoom_http_client_url_head_info phead_info, int head_num, uint8 flag)
{
    //https://api.ws.sonos.com/control/api/v1/groups/groupId/playbackMetadata
    int volume;
    char *url_info, *net_data, music_id[64], *cmd_string;
	divoom_cJSON *root = NULL, *obj;

    url_info = divoom_system_malloc(256);
    if(url_info == NULL)
    {
        return 0;
    }
    strcpy(url_info, base_url);
    strcat(url_info, "/playbackMetadata");

    net_data = divoom_http_send_data(url_info, NULL, 0, phead_info, head_num);
    
    if(net_data == NULL)
    {
        divoom_system_free(url_info);
        return 0;
    }
    

	root = divoom_cJSON_Parse(net_data);
    divoom_system_free(net_data);
	if (!root)
	{
        divoom_system_free(url_info);
		return 0;
	}
    obj = divoom_cJSON_GetObjectItem(root, "currentItem");
    if(obj == NULL)
    {
        divoom_cJSON_Delete(root);
        divoom_system_free(url_info);
		return 0;
    }
    divoom_cJSON_GetObjectString(obj, "id", music_id);
    if(strlen(music_id) == 0)
    {
        divoom_cJSON_Delete(root);
        divoom_system_free(url_info);
		return 0;
    }
    divoom_cJSON_Delete(root);
    //https://api.ws.sonos.com/control/api/v1/players/playerId/playerVolume
    strcpy(url_info, "https://api.ws.sonos.com/control/api/v1/players/");
    strcat(url_info, music_id);
    strcat(url_info, "/playerVolume");
    

    net_data = divoom_http_send_data(url_info, NULL, 0, phead_info, head_num);
    
    if(net_data == NULL)
    {
        divoom_system_free(url_info);
        return 0;
    }
    
	root = divoom_cJSON_Parse(net_data);
    divoom_system_free(net_data);
	if (!root)
	{
        divoom_system_free(url_info);
		return 0;
	}
    volume = divoom_cJSON_GetObjectInt(root, "volume");
    divoom_cJSON_Delete(root);
    if(flag)
    {
        if(volume >= 100)
        {
            divoom_cJSON_Delete(root);
            divoom_system_free(url_info);
            return 1;
        }
        else if(volume >= 90)
        {
            volume = 100;
        }
        else
        {
            volume += 10;
        }
    }
    else
    {
        if(volume == 0)
        {
            divoom_cJSON_Delete(root);
            divoom_system_free(url_info);
            return 1;
        }
        else if(volume < 10)
        {
            volume = 0;
        }
        else
        {
            volume -= 10;
        }
    }

    root = divoom_cJSON_CreateObject();
    if(root == NULL)
    {
        divoom_system_free(url_info);
		return 0;
    }
	divoom_cJSON_AddNumberToObject(root,"volume",volume);
	cmd_string = divoom_cJSON_Print(root);
    divoom_cJSON_Delete(root);
    net_data = divoom_http_send_data(url_info, cmd_string, 1, phead_info, head_num);
    divoom_system_free(net_data);
    divoom_system_free(cmd_string);    
    divoom_system_free(url_info);
    return 1;

}

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

int divoom_sonos_ctrl_play_muted_ctrl(char *base_url, Pdivoom_http_client_url_head_info phead_info, int head_num)
{
    //https://api.ws.sonos.com/control/api/v1/groups/groupId/playbackMetadata
    int volume;
    char *url_info, *net_data, music_id[64], *cmd_string;
	divoom_cJSON *root = NULL, *obj;

    url_info = divoom_system_malloc(256);
    if(url_info == NULL)
    {
        return 0;
    }
    strcpy(url_info, base_url);
    strcat(url_info, "/playbackMetadata");

    net_data = divoom_http_send_data(url_info, NULL, 0, phead_info, head_num);
    
    if(net_data == NULL)
    {
        divoom_system_free(url_info);
        return 0;
    }
    

	root = divoom_cJSON_Parse(net_data);
    divoom_system_free(net_data);
	if (!root)
	{
        divoom_system_free(url_info);
		return 0;
	}
    obj = divoom_cJSON_GetObjectItem(root, "currentItem");
    if(obj == NULL)
    {
        divoom_cJSON_Delete(root);
        divoom_system_free(url_info);
		return 0;
    }
    divoom_cJSON_GetObjectString(obj, "id", music_id);
    if(strlen(music_id) == 0)
    {
        divoom_cJSON_Delete(root);
        divoom_system_free(url_info);
		return 0;
    }
    divoom_cJSON_Delete(root);
    //https://api.ws.sonos.com/control/api/v1/players/playerId/playerVolume
    strcpy(url_info, "https://api.ws.sonos.com/control/api/v1/players/");
    strcat(url_info, music_id);
    strcat(url_info, "/playerVolume");
    

    net_data = divoom_http_send_data(url_info, NULL, 0, phead_info, head_num);
    
    if(net_data == NULL)
    {
        divoom_system_free(url_info);
        return 0;
    }
    
	root = divoom_cJSON_Parse(net_data);
    divoom_system_free(net_data);
	if (!root)
	{
        divoom_system_free(url_info);
		return 0;
	}
    volume = divoom_cJSON_GetObjectInt(root, "muted");
    divoom_cJSON_Delete(root);
    volume = volume?0:1;

    root = divoom_cJSON_CreateObject();
    if(root == NULL)
    {
        divoom_system_free(url_info);
		return 0;
    }
	divoom_cJSON_AddBoolToObject(root,"muted",volume);
	cmd_string = divoom_cJSON_Print(root);
    divoom_cJSON_Delete(root);
    net_data = divoom_http_send_data(url_info, cmd_string, 1, phead_info, head_num);
    divoom_system_free(net_data);
    divoom_system_free(cmd_string);    
    divoom_system_free(url_info);
    return 1;

}

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

void divoom_sonos_ctrl_key_do_with(char *base_url, Pdivoom_http_client_url_head_info phead_info, int head_num, char *key_flag)
{
    if(strcmp(key_flag, "PlayPre") == 0)
    {
        divoom_sonos_ctrl_play_next_pre(base_url, phead_info, head_num, 0);
    }
    else if(strcmp(key_flag, "PlayNext") == 0)
    {
        divoom_sonos_ctrl_play_next_pre(base_url, phead_info, head_num, 1);
    }
    else if(strcmp(key_flag, "VolAdd") == 0)
    {
        divoom_sonos_ctrl_set_play_volume(base_url, phead_info, head_num, 1);
    }
    else if(strcmp(key_flag, "VolDec") == 0)
    {
        divoom_sonos_ctrl_set_play_volume(base_url, phead_info, head_num, 0);
    }
    else if(strcmp(key_flag, "PlayMuted") == 0)
    {
        divoom_sonos_ctrl_play_muted_ctrl(base_url, phead_info, head_num);
    }
    else if(strcmp(key_flag, "PlayCtrl") == 0)
    {
        if(divoom_sonos_ctrl_get_play_status(base_url, phead_info, head_num))
        {
            divoom_sonos_ctrl_set_play_ctrl(base_url, phead_info, head_num, 0);
        }
        else
        {
            divoom_sonos_ctrl_set_play_ctrl(base_url, phead_info, head_num, 1);
        }
    }
}


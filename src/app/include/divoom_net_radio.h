#ifndef _DIVOOM_NET_RADIO_H_
#define _DIVOOM_NET_RADIO_H_
enum{

	DIVOOM_NET_RADIO_PLAY_EVENT,
};
typedef enum
{
	DIVOOM_NET_RADIO_GET_STATION, 		//根据类型获取电台
	DIVOOM_NET_RADIO_PLAY,				//播放
	DIVOOM_NET_RADIO_PAUSE,				//暂停
	DIVOOM_NET_RADIO_PLAY_NEXT,			//下一个
	DIVOOM_NET_RADIO_PLAY_PRE,			//上一个
	DIVOOM_NET_RADIO_ADDVOL,			//增加音量
	DIVOOM_NET_RADIO_DOWNVOL			//减少音量
}DIVOOM_NET_RADIO_EVENT;

/**
 * *  NAME          divoom_net_radio_get_station_list_top500
 * *  @brief       	top500电台
 * *  @param[in] 	none
 * *  @author       wangft                                   
 * *  @return       none
 * */
void divoom_net_radio_get_station_list_top500(void);

/**
 * *  NAME          divoom_net_radio_get_station_list_from_genre
 * *  @brief       	通过类型获取电台列表
 * *  @param[in] 	genre 类型
 * *  @author       wangft                                   
 * *  @return       none
 * */
void divoom_net_radio_get_station_list_from_genre(char *genre);

/**
 * *  NAME          divoom_net_radio_play_station
 * *  @brief       	播放
 * *  @param[in] 	none
 * *  @author       wangft                                   
 * *  @return       none
 * */
void divoom_net_radio_play_station(void);

/**
 * *  NAME          divoom_net_radio_init
 * *  @brief       	初始化
 * *  @param[in] 	none
 * *  @author       wangft                                   
 * *  @return       none
 * */
void divoom_net_radio_init(void);

/**
 * *  NAME          divoom_net_radio_stop
 * *  @brief       	停止
 * *  @param[in] 	none
 * *  @author       wangft                                   
 * *  @return       none
 * */
void divoom_net_radio_stop(void);

/**
 * *  NAME          divoom_net_radio_get_cur_radio_name
 * *  @brief       	获取播放电台名称
 * *  @param[in] 	
 * *  @return       电台名称
 * */
char *divoom_net_radio_get_cur_radio_name(void);

uint8 divoom_net_radio_app_schedule_play(uint8 event_id, void *user_data, uint32 resouce_type);

uint8 divoom_net_radio_app_schedule_pause(uint8 event_id, void *user_data, uint8 active_model, uint8 active_event, uint32 resouce_type);

uint8 divoom_net_radio_app_schedule_destroy(uint8 event_id, void *user_data, uint32 status, uint32 resouce_type);

uint8 divoom_net_radio_app_schedule_key_handle(uint8 key_id, uint8 key_type, uint32 resouce_type);

#endif


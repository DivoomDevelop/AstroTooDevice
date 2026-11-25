#include "divoom.h"

#define DIVOOM_FM_DEVELOPER_ID			"Mw1hzAxXXZDfUxcG"
/* Directory credential shared by the paged radio service. */
const char *divoom_net_radio_directory_key(void)
{
    return DIVOOM_FM_DEVELOPER_ID;
}

#if 0
#define SHOUTCAST_YP_SERVER "yp.shoutcast.com"
#define SHOUTCAST_API_SERVER "api.shoutcast.com"
#define PORT "80"

char GET_SHOUT[] = "GET %s HTTP/1.0\r\n" //%s 如
    "Host: yp.shoutcast.com\r\n"
    "User-Agent: esp-idf/1.0 esp32\r\n"
    "\r\n";

char POST_SHOUT[] = "POST %s HTTP/1.1\r\n"
	"Host: api.shoutcast.com\r\n"
	"User-Agent: esp-idf/1.0 esp32\r\n"
	"Accept: */*\r\n"
	"Content-Type: application/x-www-form-urlencoded\r\n"
	"Content-Length: %d\r\n\n"
	"%s\r\n" //所带的参数 如：
	"\r\n";

#define DIVOOM_NET_RADIO_TAG 			"DIVOOM_NET_RADIO"
#define DIVOOM_NET_BASE_RADIO_URL		"/sbin/tunein-station."
#define DIVOOM_RADIO_BASE_M3U			"m3u"
#define DIVOOM_RADIO_BASE_PLS			"pls"
#define DIVOOM_RADIO_BASE_XSPF			"xspf"
#define DIVOOM_RADIO_ID					"?id="
//获取播放URL:DIVOOM_NET_BASE_RADIO_URL + (DIVOOM_RADIO_BASE_M3U(PLS)(XSPF)) +　DIVOOM_RADIO_ID + 当前激活的id值
#define DIVOOM_GENRE_SEARCH_FORMAT		"/legacy/genresearch"
#define DIVOOM_TOP500_SEARCH_FORMAT "/legacy/Top500"

#define DIVOOM_DEVELOPER_ID_FORMAT		"k="
#define DIVOOM_GENRE_BASE				"&genre="
#define DIVOOM_SEARCH_LIMIT				"&limit="  //分段获取电台
#define DIVOOM_PART_NUM	10 //每一段为10个
//通过类型获取电台列表url:DIVOOM_GENRE_SEARCH_FORMAT + DIVOOM_FM_DEVELOPER_ID + DIVOOM_GENRE_BASE + 类型
#define FM_LIST_NUM 50
#define FM_VOL_MAX	10

typedef struct _divoom_station_info
{
	char station_name[64];
	char station_id[24];
}divoom_station_info, *Pdivoom_station_info;

typedef struct _divoom_net_radio_info
{
    char genre_name[128];
	divoom_station_info fm_list[FM_LIST_NUM]; //保存当前类型的电台列表
	uint8 run_flag;
	uint8 fm_list_num; //记录当前fm的个数
	uint8 fm_cur_index; //当前正在播放的fm
	uint8 play_status;
	uint8 break_flag;
	int fm_cur_vol;	//当前音量 0~10
	void *get_radio_sem; //信号量
	uint8 radio_cmd; //控制命令
}divoom_net_radio_info, *Pdivoom_net_radio_info;

static Pdivoom_net_radio_info pdivoom_net_radio_handle = NULL;

/**
 * *  NAME          divoom_net_radio_get_cur_radio_name
 * *  @brief       	获取播放电台名称
 * *  @param[in] 	
 * *  @return       none
 * */
char *divoom_net_radio_get_cur_radio_name(void)
{
	uint8 station_index = 0;

    if(pdivoom_net_radio_handle)
    {
		station_index = pdivoom_net_radio_handle->fm_cur_index;
		return pdivoom_net_radio_handle->fm_list[station_index].station_name;
    }
    return NULL;
}

/**
 * *  NAME          divoom_net_radio_http_client_socket_get
 * *  @brief       	获取播放url
 * *  @param[in] 	web_srv 电台服务	
 					reques 请求的数据
 					read_len 获取到的长度
 * *  @author       wangft                                   
 * *  @return       none
 * */
char *divoom_net_radio_http_client_socket_get(char *srv, char *request, int *read_len)
{
	const struct addrinfo hints = {
		.ai_family = AF_INET,
		.ai_socktype = SOCK_STREAM,
	};
	struct addrinfo *res;
	struct in_addr *addr;
	int s, r, length = 0, buf_len = 0;
	int err = getaddrinfo(srv, PORT, &hints, &res);
	char *recv_buf = NULL;

	if (err != 0 || res == NULL) 
	{
		divoom_print("DNS lookup failed err=%d res=%p", err, res);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		return NULL;
	}

	addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
	divoom_print("DNS lookup succeeded. IP=%s", inet_ntoa(*addr));
	s = socket(res->ai_family, res->ai_socktype, 0);
	if(s < 0) 
	{
		divoom_print("... Failed to allocate socket.");
		freeaddrinfo(res);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		return NULL;
	}
	divoom_print("... allocated socket");
	if(connect(s, res->ai_addr, res->ai_addrlen) != 0) 
	{
		divoom_print("... socket connect failed errno=%d", errno);
		close(s);
		freeaddrinfo(res);
		vTaskDelay(4000 / portTICK_PERIOD_MS);
		return NULL;
	}

	divoom_print("... connected");
	freeaddrinfo(res);
	if (write(s, request, strlen(request)) < 0) 
	{
		divoom_print("... socket send failed");
		close(s);
		vTaskDelay(4000 / portTICK_PERIOD_MS);
		return NULL;
	}
	divoom_print("... socket send success");
	struct timeval receiving_timeout;
	receiving_timeout.tv_sec = 5;
	receiving_timeout.tv_usec = 0;
	if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout, sizeof(receiving_timeout)) < 0) 
	{
		divoom_print("... failed to set socket receiving timeout");
		close(s);
		vTaskDelay(4000 / portTICK_PERIOD_MS);
		return NULL;
	}
	divoom_print("... set socket receiving timeout success");

	if (strcmp(srv, SHOUTCAST_YP_SERVER) == 0)
	{
		recv_buf = divoom_malloc(1024, DIVOOM_MODEL_NET, __LINE__, __FILE__);
		r = read(s, recv_buf, 1024-1); //简单粗暴只获取前面一部分信息，得到一个播放url便可
	}
	else
	{
		char read_buf[10] = {0}, *p = NULL;
		int flag = 0;
		do{
			bzero(read_buf, sizeof(read_buf));
			r = read(s, &read_buf[0], 1);
			if (read_buf[0] == '\n' || read_buf[0] == '\r')
			{
				flag++;
				if (flag == 4) //得到获取长度的位置
				{			
					break;
				}
			}
			else 
			{
				flag = 0;
			}
			//putchar(read_buf[0]);
		}while (r > 0);

		bzero(read_buf, sizeof(read_buf));
		p = read_buf;
		do{
			r = read(s, p, 1);
			//putchar(*p);
			if (*p == '\n') //得到长度
			{
				*p = '\0';
				break;
			}
			p++;
		}while (r > 0);
		sscanf(read_buf, "%x", &buf_len); //把十六进制数据写入字段
		divoom_print("length %d", buf_len);
		recv_buf = divoom_malloc(buf_len + 10, DIVOOM_MODEL_NET, __LINE__, __FILE__);
		length = 0;
		p = recv_buf;
		r = 0;
		do{
			r = read(s, p, 1);
			//putchar(*p);
			p += r;
			length += r;
		}while (r > 0 && length < buf_len);
		*read_len = length;
	}
	divoom_print("... done reading from socket. Last read return=%d errno=%d.", length, errno);
	close(s);
	return recv_buf;
}

/**
 * *  NAME          divoom_net_radio_handle_data
 * *  @brief       	处理得到的数据
 * *  @param[in] 	buffer 得到的数据
 * *  @author       wangft                                   
 * *  @return       none
 * */
void divoom_net_radio_handle_data(char *buffer)
{
	char *p, *q;
	int i = 0, table_index = 0;
	
	p = buffer;
	table_index = pdivoom_net_radio_handle->fm_list_num;
	divoom_print("table_index %d", table_index);
	for (i = 0; i < DIVOOM_PART_NUM; i++)
	{
		q = strstr(p, "station name");
		if (q)
		{
			p = q + strlen("station name");
			p += 2; //越过="
			q = strchr(p, '"');
			strncpy(pdivoom_net_radio_handle->fm_list[i + table_index].station_name, p, q -p);
		}
		else
			break;
		p = q;
		q = strstr(p, "id");
		if (q)
		{
			p = q + strlen("id");
			p += 2; //越过="
			q = strchr(p, '"');
			strncpy(pdivoom_net_radio_handle->fm_list[i + table_index].station_id, p, q -p);
		}
		else 
			break;
		p = q;
	}
	pdivoom_net_radio_handle->fm_list_num += i;
}

/**
 * *  NAME          divoom_net_radio_send_cmd
 * *  @brief       	发命令到任务，任务里面去执行播放
 * *  @param[in] 	none
 * *  @author       wangft                                   
 * *  @return       none
 * */
void divoom_net_radio_send_cmd(uint8 radio_cmd)
{
	pdivoom_net_radio_handle->radio_cmd = radio_cmd;
	divoom_system_put_sem(pdivoom_net_radio_handle->get_radio_sem);
}

/**
 * *  NAME          divoom_net_radio_play_pre
 * *  @brief       	播放上一个station
 * *  @param[in] 	none
 * *  @author       wangft                                   
 * *  @return       none
 * */
void divoom_net_radio_play_pre(void)
{
	if (pdivoom_net_radio_handle->fm_cur_index == 0)
	{
		pdivoom_net_radio_handle->fm_cur_index = pdivoom_net_radio_handle->fm_list_num -1;
	}
	else
	{
		pdivoom_net_radio_handle->fm_cur_index--;
	}
	divoom_net_radio_send_cmd(DIVOOM_NET_RADIO_PLAY);
}

/**
 * *  NAME          divoom_net_radio_set_vol
 * *  @brief       	设置电台音量
 * *  @param[in] 	flag 1 增加音量 0 降低音量
 * *  @author       wangft                                   
 * *  @return       none
 * */
void divoom_net_radio_set_vol(uint8 flag)
{
	if (flag)
	{
		if (pdivoom_net_radio_handle->fm_cur_vol < FM_VOL_MAX)
			pdivoom_net_radio_handle->fm_cur_vol++;
	}
	else
	{
		if (pdivoom_net_radio_handle->fm_cur_vol > 0)
			pdivoom_net_radio_handle->fm_cur_vol--;
	}
	
	divoom_local_music_volume(pdivoom_net_radio_handle->fm_cur_vol);
}

/**
 * *  NAME          divoom_net_radio_play_next
 * *  @brief       	播放下一个station
 * *  @param[in] 	none
 * *  @author       wangft                                   
 * *  @return       none
 * */
void divoom_net_radio_play_next(void)
{
	if (pdivoom_net_radio_handle->fm_cur_index == pdivoom_net_radio_handle->fm_list_num -1)
	{
		pdivoom_net_radio_handle->fm_cur_index = 0;
	}
	else
	{
		pdivoom_net_radio_handle->fm_cur_index++;
	}
	divoom_net_radio_send_cmd(DIVOOM_NET_RADIO_PLAY);
}

/**
 * *  NAME          divoom_net_radio_pause_station
 * *  @brief       	暂停
 * *  @param[in] 	none
 * *  @author       wangft                                   
 * *  @return       none
 * */
void divoom_net_radio_pause_station(void)
{
	pdivoom_net_radio_handle->play_status = 0;
	divoom_local_music_ctrl(0);
}

/**
 * *  NAME          divoom_net_radio_get_play_url
 * *  @brief       	获取播放url
 * *  @param[in] 	buffer 得到的数据
 * *  @author       wangft                                   
 * *  @return       1 成功找到播放url 	 0失败
 * */
char *divoom_net_radio_get_play_url(char *buffer)
{
	char *p, *q, *play_url = NULL;

	if (buffer == NULL)
		return NULL;
	
	p = buffer;
	q = strstr(p, "http");
	
	if (q)
	{
		p = q;
		q = strchr(p, '\n');
		play_url = divoom_malloc(128, DIVOOM_MODEL_NET_RADIO, __LINE__, __FILE__);
		if (play_url)
			strncpy(play_url, p, q-p);
	}
	return play_url;
}

/**
 * *  NAME          divoom_net_radio_play_callback
 * *  @brief       	电台播放回调
 * *  @param[in] 	status 状态
 * *  @author       wangft                                   
 * *  @return       none
 * */
void divoom_net_radio_play_callback(uint8 status)
{
	switch (status)
	{
		case DIVOOM_MUSIC_PLAY_STATUS_NO_EXIST:
			divoom_net_radio_play_next();
			break;
		case DIVOOM_MUSIC_PLAY_STATUS_END:
			divoom_net_radio_play_next();
			break;
		case DIVOOM_MUSIC_PLAY_STATUS_ERR:
			divoom_net_radio_play_next();
			break;
        case DIVOOM_MUSIC_PLAY_STATUS_PLAY:
            pdivoom_net_radio_handle->play_status = 1;
            break;
		case DIVOOM_MUSIC_PLAY_STATUS_PAUSE:
            pdivoom_net_radio_handle->play_status = 0;
			break;
	}
}

/**
 * *  NAME          divoom_net_radio_play_station
 * *  @brief       	播放
 * *  @param[in] 	none
 * *  @author       wangft                                   
 * *  @return       none
 * */
void divoom_net_radio_play_station(void)
{
	char url[128];
	uint8 station_index = 0;
	char *buffer = NULL, *play_url = NULL, request_buffer[256] = {0};
	int data_len = 0;
	
	station_index = pdivoom_net_radio_handle->fm_cur_index;
	sprintf(url, "%s%s%s%s", DIVOOM_NET_BASE_RADIO_URL, DIVOOM_RADIO_BASE_M3U, DIVOOM_RADIO_ID, pdivoom_net_radio_handle->fm_list[station_index].station_id); //针对每个类型获取前50个电台来进行播放
	sprintf(request_buffer, GET_SHOUT, url);
	buffer = divoom_net_radio_http_client_socket_get(SHOUTCAST_YP_SERVER, request_buffer, &data_len);
	play_url = divoom_net_radio_get_play_url(buffer);
	if (play_url)
	{
		pdivoom_net_radio_handle->play_status = 1;
		divoom_print("%s", play_url);
		divoom_local_music_stop();
		pdivoom_net_radio_handle->fm_cur_vol = divoom_main_get_sys_volume();
		divoom_local_music_play(play_url, pdivoom_net_radio_handle->fm_cur_vol, 0, divoom_net_radio_play_callback, 0);
	}
	if (buffer)
		divoom_free(buffer);
	if (play_url)
		divoom_free(play_url);
}

/**
 * *  NAME          divoom_net_radio_get_station_list_from_genre
 * *  @brief       	通过类型获取电台列表
 * *  @param[in] 	genre 类型
 * *  @author       wangft                                   
 * *  @return       none
 * */
void divoom_net_radio_get_station_list_from_genre(char *genre)
{
	int data_len = 0, len = 0, i= 0;
	char *buffer = NULL;
	char url[20] = {0};
	char *request_buffer = divoom_malloc(512, DIVOOM_MODEL_NET_RADIO, __LINE__, __FILE__);
	char *post_arg = divoom_malloc(128, DIVOOM_MODEL_NET_RADIO, __LINE__, __FILE__);

	if (request_buffer == NULL || post_arg == NULL)
	{
		divoom_print("divoom_malloc error");
		return;
	}
	
	sprintf(url, "%s", DIVOOM_GENRE_SEARCH_FORMAT);
	pdivoom_net_radio_handle->fm_list_num = 0;
	for (i = 0; i < 5 && pdivoom_net_radio_handle->run_flag; i++) //获取前50
	{
		sprintf(post_arg, "%s%s%s%s%s%d,%d", DIVOOM_DEVELOPER_ID_FORMAT, DIVOOM_FM_DEVELOPER_ID, DIVOOM_GENRE_BASE, genre, DIVOOM_SEARCH_LIMIT, i*DIVOOM_PART_NUM, DIVOOM_PART_NUM); //针对每个类型获取前50个电台来进行播放
		len = strlen(post_arg);
		sprintf(request_buffer, POST_SHOUT, url, len, post_arg);
		buffer = divoom_net_radio_http_client_socket_get(SHOUTCAST_API_SERVER, request_buffer, &data_len);
		if (buffer)
		{
			divoom_net_radio_handle_data(buffer);
		}
		divoom_free(buffer);
	}
	divoom_print("fm_list_num  %d", pdivoom_net_radio_handle->fm_list_num);
}

/**
 * *  NAME          divoom_net_radio_get_station_list_top500
 * *  @brief       	top500电台
 * *  @param[in] 	none
 * *  @author       wangft                                   
 * *  @return       none
 * */
void divoom_net_radio_get_station_list_top500(void)
{
	int data_len = 0, len = 0, i= 0;
	char *buffer = NULL;
	char url[20] = {0};
	char *request_buffer = divoom_malloc(512, DIVOOM_MODEL_NET_RADIO, __LINE__, __FILE__);
	char *post_arg = divoom_malloc(128, DIVOOM_MODEL_NET_RADIO, __LINE__, __FILE__);

	if (request_buffer == NULL || post_arg == NULL)
	{
		divoom_print("divoom_malloc error");
		return;
	}
	
	sprintf(url, "%s", DIVOOM_TOP500_SEARCH_FORMAT);
	pdivoom_net_radio_handle->fm_list_num = 0;
	for (i = 0; i < 5 && pdivoom_net_radio_handle->run_flag; i++) //获取前50
	{
		sprintf(post_arg, "%s%s%s%d,%d", DIVOOM_DEVELOPER_ID_FORMAT, DIVOOM_FM_DEVELOPER_ID, DIVOOM_SEARCH_LIMIT, i*DIVOOM_PART_NUM, DIVOOM_PART_NUM); //针对每个类型获取前50个电台来进行播放
		len = strlen(post_arg);
		sprintf(request_buffer, POST_SHOUT, url, len, post_arg);
		divoom_print("request_buffer %s", request_buffer);
		buffer = divoom_net_radio_http_client_socket_get(SHOUTCAST_API_SERVER, request_buffer, &data_len);
		if (buffer)
		{
			divoom_net_radio_handle_data(buffer);
		}
		divoom_free(buffer);
	}
	divoom_print("fm_list_num  %d", pdivoom_net_radio_handle->fm_list_num);
}

/**
 * *  NAME          divoom_net_radio_event
 * *  @brief       	radio模块事件处理
 * *  @param[in] 	msg 要处理的消息
 * *  @author       wangft                                   
 * *  @return       uint8
 * */
uint8 divoom_net_radio_event(Pdivoom_sys_msg_info msg)
{
    divoom_print("%s:event id: %d\n", __func__, msg->msg_id);
	switch (msg->msg_id)
	{
		case DIVOOM_NET_RADIO_GET_STATION:
			{
				char *genre = (char *)msg->data;

                if(genre && strcmp(genre, pdivoom_net_radio_handle->genre_name))
                {
                    strcpy(pdivoom_net_radio_handle->genre_name, genre);
					divoom_net_radio_send_cmd(DIVOOM_NET_RADIO_GET_STATION);
                }
			}
			break;
		case DIVOOM_NET_RADIO_PLAY:
            if(pdivoom_net_radio_handle->play_status)
            {
			    divoom_net_radio_pause_station();
            }
            else
            {
				divoom_net_radio_send_cmd(DIVOOM_NET_RADIO_PLAY);
            }
			break;
		case DIVOOM_NET_RADIO_PAUSE:
			divoom_net_radio_pause_station();
			break;
		case DIVOOM_NET_RADIO_PLAY_NEXT:
			divoom_net_radio_play_next();
			break;
		case DIVOOM_NET_RADIO_PLAY_PRE:
			divoom_net_radio_play_pre();
			break;
		case DIVOOM_NET_RADIO_ADDVOL:
			divoom_net_radio_set_vol(1);
			break;
		case DIVOOM_NET_RADIO_DOWNVOL:
			divoom_net_radio_set_vol(0);
			break;
	}
	return 0;
}

/**
 * NAME             divoom_net_radio_task
 * *  @brief       	获取电台比较耗时，单独开个任务独立获取
 * *  @param[in] 		
 * *	 @author         wangft                                   
 * *  @return        
 * */
void divoom_net_radio_task(void *para)
{
	while (pdivoom_net_radio_handle->run_flag)
	{
		divoom_system_get_sem(pdivoom_net_radio_handle->get_radio_sem);
        if(pdivoom_net_radio_handle->run_flag == 0)
        {
            break;
        }
		if (pdivoom_net_radio_handle->radio_cmd == DIVOOM_NET_RADIO_GET_STATION)
		{
			if (strcmp(pdivoom_net_radio_handle->genre_name, "Top 500") == 0) //top500
			{
				divoom_net_radio_get_station_list_top500();
			}
			else //普通类型
			{
				divoom_net_radio_get_station_list_from_genre(pdivoom_net_radio_handle->genre_name);
			}
		}
		else if (pdivoom_net_radio_handle->radio_cmd == DIVOOM_NET_RADIO_PLAY)
		{
			divoom_net_radio_play_station();
		}
		pdivoom_net_radio_handle->radio_cmd = 0xff;
		vTaskDelay(100 / portTICK_PERIOD_MS);
	}
}


uint8 divoom_net_radio_app_schedule_play(uint8 event_id, void *user_data, uint32 resouce_type)
{
	if(resouce_type & DIVOOM_SCHEDULE_VOICE_RESOUCE_TYPE)
	{
		if(pdivoom_net_radio_handle->break_flag)
		{
			//如果被打断前为播放状态，重新恢复播放
			pdivoom_net_radio_handle->break_flag = 0;
            pdivoom_net_radio_handle->play_status = 1;
		}
	    if(pdivoom_net_radio_handle->play_status)
	    {
			divoom_net_radio_send_cmd(DIVOOM_NET_RADIO_PLAY);
	    }
	}
	return 1;
}

//不同事件可以被打断,被打断不做任何操作
uint8 divoom_net_radio_app_schedule_pause(uint8 event_id, void *user_data, uint8 active_model, uint8 active_event, uint32 resouce_type)
{
	if(resouce_type & DIVOOM_SCHEDULE_VOICE_RESOUCE_TYPE)
	{
	    if(pdivoom_net_radio_handle->play_status)
	    {
			{
	        	divoom_net_radio_pause_station();
				pdivoom_net_radio_handle->break_flag = 1;
			}
	    }
	}
	return 1;
}

//需要把所有的资源全部释放了,16*16的不需要,会被别的模式直接退出
uint8 divoom_net_radio_app_schedule_destroy(uint8 event_id, void *user_data, uint32 status, uint32 resouce_type)
{
    if(pdivoom_net_radio_handle->play_status)
    {
		pdivoom_net_radio_handle->break_flag = 0;
        divoom_net_radio_pause_station();
    }
	return true;
}

uint8 divoom_net_radio_app_schedule_key_handle(uint8 key_id, uint8 key_type, uint32 resouce_type)
{
	//TODO
	divoom_print("%s: key_id: 0x%x, type: %d, resource: %d\n", __func__, key_id, key_type, resouce_type);
	if((resouce_type & DIVOOM_SCHEDULE_SCREEN_RESOUCE_TYPE))
	{
        switch(key_id)
        {
            case kbFUNC:
                if(key_type == PRESS_SHORT)
                {
                    if(pdivoom_net_radio_handle->play_status)
                    {
						pdivoom_net_radio_handle->break_flag = 0;
                        divoom_net_radio_pause_station();
                    }
                    else
                    {
                        pdivoom_net_radio_handle->play_status = 1;
						divoom_net_radio_send_cmd(DIVOOM_NET_RADIO_PLAY);
                    }
                    return 1;
                }
                break;
                
            case kbPRE:
                if(key_type == PRESS_LONG)
                {
                    divoom_net_radio_play_pre();
                    return 1;
                }
                break;
                
            case kbNEXT:
                if(key_type == PRESS_LONG)
                {
                    divoom_net_radio_play_next();
                    return 1;
                }
                break;
                
            case kbVOLADD:
                divoom_net_radio_set_vol(1);
                return 1;
                
            case kbVOLSUB:
                divoom_net_radio_set_vol(0);
                return 1;
        }
	}
	else if( pdivoom_net_radio_handle->play_status)
	{
        if(key_id == kbFUNC && key_type == PRESS_SHORT)
        {
			pdivoom_net_radio_handle->break_flag = 0;
            divoom_net_radio_pause_station();
            return 1;
        }
	}
	return 0;
}

/**
 * *  NAME          divoom_net_radio_stop
 * *  @brief       	停止
 * *  @param[in] 	none
 * *  @author       wangft                                   
 * *  @return       none
 * */
void divoom_net_radio_stop(void)
{
	if (pdivoom_net_radio_handle)
	{
        pdivoom_net_radio_handle->run_flag = 0;
	}
}

/**
 * *  NAME          divoom_net_radio_init
 * *  @brief       	初始化
 * *  @param[in] 	none
 * *  @author       wangft                                   
 * *  @return       none
 * */
void divoom_net_radio_init(void)
{
	divoom_app_schedule_item_info push_info = {0};
    
	if (pdivoom_net_radio_handle)
		return;
	pdivoom_net_radio_handle = divoom_malloc(sizeof(divoom_net_radio_info), DIVOOM_MODEL_NET_RADIO, __LINE__, __FILE__);
	memset(pdivoom_net_radio_handle, 0, sizeof(divoom_net_radio_info));
	pdivoom_net_radio_handle->radio_cmd = 0xff;
	pdivoom_net_radio_handle->get_radio_sem = divoom_system_create_sem(2, 0);
	divoom_sys_model_crl_register(DIVOOM_MODEL_NET_RADIO, divoom_net_radio_event);
	divoom_system_create_task_prio(divoom_net_radio_task, "DIVOOM_NET_RADIO_TASK", NULL, 2*1024 + 512, 1);
    pdivoom_net_radio_handle->play_status = 0;
    pdivoom_net_radio_handle->run_flag = 1;
	pdivoom_net_radio_handle->fm_cur_vol = divoom_main_get_sys_volume();

	push_info.event_id = DIVOOM_NET_RADIO_PLAY_EVENT;
	push_info.model_id = DIVOOM_MODEL_NET_RADIO;
    push_info.resource_type = DIVOOM_SCHEDULE_VOICE_RESOUCE_TYPE;
    push_info.attr = DIVOOM_APP_SCHEDULE_ATTR_WAIT;
	divoom_app_schedule_push(&push_info);
	return;
error:
	if (pdivoom_net_radio_handle)
	{
		divoom_free(pdivoom_net_radio_handle);
	}
}
#endif


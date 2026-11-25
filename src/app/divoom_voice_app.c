#if 0
#include "divoom.h"
//做voice的控制,别的上层应用模块发控制命令到此模块,进行控制,要输入地址,循环标志,播放完的回调等等
//貌似大部分模块都是play或者pause,基本上应该没有stop的操作,大部分stop操作应该都是自动进行的
//提示音和radio都用play,每次先stop前一个

#define DIVOOM_VOICE_ALL_NUM			(6)
#define DIVOOM_VOICE_TAG				("VOICE_APP")
#define DIVOOM_VOICE_STACK				(1<<10)
//默认第八级,最大100
#define DIVOOM_VOICE_DEFAULT_VOL		(77)

const static uint8 DIVOOM_VOL[] = {0, 7, 14, 21, 28, 35, 42, 49, 56, 63, 70, 77, 85, 92, 100};

typedef struct _divoom_voice_ctrl_
{
	divoom_voice_play_info voice_arr[DIVOOM_VOICE_ALL_NUM];
	uint8 cur_vol;
	uint32 cur_pause_pos;
	uint8 cur_active_num;
	uint8 all_voice_num;
	void *lock;
	//接收MediaControl.c发送的消息
	QueueHandle_t voice_play_status;
	//接收由event发出的消息
	QueueHandle_t voice_noplay_status;
	QueueSetMemberHandle_t queue_set_all;
	void *task_handle;
}divoom_voice_ctrl, *Pdivoom_voice_ctrl;

typedef struct _divoom_voice_status_
{
	VOICE_PLAY_STATUS status;
	uint8 event_active_num;
}divoom_voice_status, *Pdivoom_voice_status;

Pdivoom_voice_ctrl pdivoom_voice_handle = NULL;

uint8 divoom_voice_app_find_next_play(Pdivoom_voice_ctrl play_handle);

//如果不是正在播放的进行了stop,需要做的动作
void divoom_voice_app_pop_event(int index, Pdivoom_voice_ctrl play_handle)
{
	if(play_handle->voice_arr[index].addr_len > 0 && play_handle->voice_arr[index].voice_addr != NULL)
		divoom_free(play_handle->voice_arr[index].voice_addr);
	if(index == play_handle->all_voice_num - 1)
	{
		memset(&play_handle->voice_arr[index], 0, sizeof(divoom_voice_play_info));
	}
	else
	{
		memmove(&play_handle->voice_arr[index], &play_handle->voice_arr[index+1], sizeof(divoom_voice_play_info)*(play_handle->all_voice_num-1-index));
	}
	play_handle->cur_active_num = 0xFF;
	play_handle->all_voice_num--;
}

//检查这个是不是空的信息
static bool divoom_voice_app_check_play_info_is_null(Pdivoom_voice_play_info play_info)
{
	if(play_info && play_info->addr_len > 0 && play_info->play_cb != NULL && play_info->voice_addr != NULL)
	{
		return 1;
	}
	return 0;
}

void divoom_voice_app_task(void *data)
{
	Pdivoom_voice_ctrl task_handle = (Pdivoom_voice_ctrl)data;
	PlayerStatus playinfo;
	divoom_voice_status voiceinfo;
	//VOICE_PLAY_STATUS status;
	QueueSetMemberHandle_t member;
	uint8 old_active_num;
	if(task_handle && task_handle->lock && task_handle->voice_play_status && task_handle->voice_noplay_status && task_handle->queue_set_all)
	{
		while(1)
		{
			//从set中查找有变化的那个queue
			member = xQueueSelectFromSet(task_handle->queue_set_all, portMAX_DELAY);
			if(member == task_handle->voice_play_status)
			{
				memset(&voiceinfo, 0, sizeof(divoom_voice_status));
				if(xQueueReceive(task_handle->voice_play_status, &playinfo, portMAX_DELAY))
				{
					divoom_print("Get Play:%d--%d--%d--%d\n", playinfo.status, playinfo.errMsg, playinfo.mode, playinfo.musicSrc);
					switch(playinfo.status)
					{
						case AUDIO_STATUS_UNKNOWN:
							voiceinfo.status = DIVOOM_VOICE_PLAY_UNKNOW;
							break;
						case AUDIO_STATUS_ERROR:
							voiceinfo.status = DIVOOM_VOICE_PLAY_ERROR;
							break;
						case AUDIO_STATUS_PLAYING:
							voiceinfo.status = DIVOOM_VOICE_PLAY_PLAYING;
							break;
						case AUDIO_STATUS_PAUSED:
							voiceinfo.status = DIVOOM_VOICE_PLAY_PAUSE;
							break;
						case AUDIO_STATUS_FINISHED:
							voiceinfo.status = DIVOOM_VOICE_PLAY_OVER;
							break;
						case AUDIO_STATUS_STOP:
							voiceinfo.status = DIVOOM_VOICE_PLAY_STOP;
							break;
						default:
							voiceinfo.status = DIVOOM_VOICE_PLAY_ERROR;
							break;
					}
					divoom_system_get_lock(task_handle->lock);
					//这里有可能是radio进行处理出错处理,也有可能是本地文件播放或者服务器文件播放处理出错
					//但是本地文件播放和服务器文件播放一般不会出现URL失效的问题,但是radio要出来出错之后对URL进行重新获取并且如果是在播放状态,就继续播放
					if(task_handle->voice_arr[task_handle->cur_active_num].play_cb)
						task_handle->voice_arr[task_handle->cur_active_num].play_cb(voiceinfo.status, playinfo.errMsg, 
								task_handle->voice_arr[task_handle->cur_active_num].type, task_handle->cur_active_num, 0);
					//stop之后,自动找到下一个可以播放的进行播放
					//但是如果当前播放的就是找到的,那么不进行任何操作
					if(voiceinfo.status == DIVOOM_VOICE_PLAY_STOP || voiceinfo.status == DIVOOM_VOICE_PLAY_ERROR)
					{
						//执行对这个进行消除
						old_active_num = task_handle->cur_active_num;
						divoom_voice_app_pop_event(task_handle->cur_active_num, task_handle);
						task_handle->cur_active_num = divoom_voice_app_find_next_play(task_handle);
						//if(task_handle->voice_arr[task_handle->cur_active_num].addr_len > 0)
						if(task_handle->voice_arr[task_handle->cur_active_num].addr_len > 0 && old_active_num != task_handle->cur_active_num)
						{
							EspAudioPlay(task_handle->voice_arr[task_handle->cur_active_num].voice_addr, 0);
						}
					}
					else if(voiceinfo.status == DIVOOM_VOICE_PLAY_OVER && task_handle->voice_arr[task_handle->cur_active_num].loop_flag)
					{
						//考察是否是loop的,是的话从新设置播放
						EspAudioPlay(task_handle->voice_arr[task_handle->cur_active_num].voice_addr, 0);
					}
					divoom_system_free_lock(task_handle->lock);
				}
			}
			else if(member == task_handle->voice_noplay_status)
			{
				//接收不在播放的那些URL的play,暂停,stop等信息
				memset(&voiceinfo, 0, sizeof(divoom_voice_status));
				if(xQueueReceive(task_handle->voice_play_status, &voiceinfo, portMAX_DELAY))
				{
					divoom_system_get_lock(task_handle->lock);
					//这是由模块内部发送的事件,所以不会有出错的代码
					if(task_handle->voice_arr[voiceinfo.event_active_num].play_cb)
						task_handle->voice_arr[voiceinfo.event_active_num].play_cb(voiceinfo.status, 0, 
								task_handle->voice_arr[voiceinfo.event_active_num].type, voiceinfo.event_active_num, 0);
					//stop之后,自动找到下一个可以播放的进行播放
					//但是如果当前播放的就是找到的,那么不进行任何操作
					if(voiceinfo.status == DIVOOM_VOICE_PLAY_STOP || voiceinfo.status == DIVOOM_VOICE_PLAY_ERROR)
					{
						//执行对这个进行消除
						old_active_num = task_handle->cur_active_num;
						divoom_voice_app_pop_event(voiceinfo.event_active_num, task_handle);
						task_handle->cur_active_num = divoom_voice_app_find_next_play(task_handle);
						if(task_handle->voice_arr[task_handle->cur_active_num].addr_len > 0 && old_active_num != task_handle->cur_active_num)
						{
							EspAudioPlay(task_handle->voice_arr[task_handle->cur_active_num].voice_addr, 0);
						}
					}
					divoom_system_free_lock(task_handle->lock);
				}
			}
		}
	}
}

//返回找到的第一个不为空的播放信息的序号
uint8 divoom_voice_app_find_next_play(Pdivoom_voice_ctrl play_handle)
{
	if(play_handle)
	{
		int i = play_handle->all_voice_num - 1;
		for(i=play_handle->all_voice_num - 1;i>=0;i--)
		{
			if(divoom_voice_app_check_play_info_is_null(&play_handle->voice_arr[i]))
			{
				return 1;
			}
			/*if(play_handle->voice_arr[i].play_cb != NULL && play_handle->voice_arr[i].voice_addr != NULL && play_handle->voice_arr[i].addr_len != 0)
			{
				return i;
			}*/
		}
	}
	//如果没有可播放的,就返回最上面的那个
	return play_handle->all_voice_num - 1;
}

//1有,0 没有
uint8 divoom_voice_app_find_same_play(Pdivoom_voice_ctrl play_handle, Pdivoom_voice_play_info play_info)
{
	if(play_handle)
	{
		int i = play_handle->all_voice_num - 1;
		for(i=play_handle->all_voice_num - 1;i>=0;i--)
		{
			if(play_handle->voice_arr[i].play_cb != NULL && play_info->play_cb != NULL && 
					play_handle->voice_arr[i].play_cb == play_info->play_cb && 
					play_handle->voice_arr[i].voice_addr != NULL && play_info->voice_addr != NULL && 
					(strcmp(play_handle->voice_arr[i].voice_addr, play_info->voice_addr) == 0) && 
					play_handle->voice_arr[i].addr_len == play_info->addr_len && 
					play_handle->voice_arr[i].loop_flag == play_info->loop_flag && 
					play_handle->voice_arr[i].type == play_info->type &&
					play_handle->voice_arr[i].model_id == play_info->model_id && 
					play_handle->voice_arr[i].event_id == play_info->event_id)
			{
				return i;
			}
		}
	}
	return 0xFF;
}

//flag返回是否需要stop
uint8 divoom_voice_app_push_event(Pdivoom_voice_ctrl play_handle, Pdivoom_voice_play_info play_info, bool *flag)
{
	if(play_handle && play_info)
	{
		if(play_handle->all_voice_num+1 >= DIVOOM_VOICE_ALL_NUM)
		{
			//超了,给0xff,这样就可以不用保存,删除的时候可以直接返回
			return 0xFF;
		}
		else
		{
			//先查找有没有,避免重复写入,如果有,那不进行任何操作
			if(divoom_voice_app_find_same_play(play_handle, play_info) != 0xFF)
			{
				return play_handle->cur_active_num;
			}
			play_handle->all_voice_num++;
			memcpy(&play_handle->voice_arr[play_handle->all_voice_num], play_info, sizeof(divoom_voice_play_info));
			if(play_info->addr_len > 0)
			{
				play_handle->voice_arr[play_handle->all_voice_num].voice_addr = divoom_malloc(sizeof(char)*(play_info->addr_len + 1), DIVOOM_MODEL_NET_RADIO, __LINE__, __func__);
				memset(play_handle->voice_arr[play_handle->all_voice_num].voice_addr, 0, sizeof(char)*(play_info->addr_len + 1));
				memcpy(play_handle->voice_arr[play_handle->all_voice_num].voice_addr, play_info->voice_addr, play_info->addr_len);
			}
			//这里不再找下一个有效的,而是根据当前输入的是否是空的进行对flag操作
			//如果不是空的,那么就需要stop
			if(*flag)
			{
				*flag = divoom_voice_app_check_play_info_is_null(play_info);
			}
			//play_handle->cur_active_num = divoom_voice_app_find_next_play(play_handle);
			//play_info->play_result = pdivoom_voice_handle->all_voice_num - 1;
			return play_handle->cur_active_num;
		}
	}

	return 0xFF;
}

uint8 divoom_voice_app_event(Pdivoom_sys_msg_info msg)
{
	static PlayerStatus playinfo;
	divoom_voice_status voiceinfo;
	if(msg)
	{
		switch(msg->msg_id)
		{
			case DIVOOM_VOICE_PLAY:
				{
					bool need_stop = 0;
					//uint8 old_active_num;
					/*EspAudioStatusGet(&playinfo);
					if(playinfo.status == AUDIO_STATUS_PLAYING)
					{
						EspAudioStop();
					}*/
					//添加进播放队列,今开始播放
					divoom_system_get_lock(pdivoom_voice_handle->lock);
					if(msg->data)
					{
						pdivoom_voice_handle->cur_active_num = divoom_voice_app_push_event(pdivoom_voice_handle, (Pdivoom_voice_play_info)msg->data, &need_stop);
						//进行播放操作
						if((pdivoom_voice_handle->all_voice_num - 1) != pdivoom_voice_handle->cur_active_num)
						{
							//如果当前的不是可以播放,就直接调用相应的cb
							//((Pdivoom_voice_play_info)msg->data)->play_cb(DIVOOM_VOICE_PLAY_PLAYING, ((Pdivoom_voice_play_info)msg->data)->type, 0);
							//如果不是可以播放的,就发送到noplay,在task里面进行回调处理
							voiceinfo.status = DIVOOM_VOICE_PLAY_PLAYING;
							voiceinfo.event_active_num = pdivoom_voice_handle->all_voice_num - 1;
							xQueueSend(pdivoom_voice_handle->voice_noplay_status, &voiceinfo, 0);
						}
						else
						{
							//如果当前的这个不是空的可以播放的,进行播放
							//直接在task里面对播放的内容进行判断吧
							//下面的放在task里面处理
							//if(pdivoom_voice_handle->voice_arr[pdivoom_voice_handle->cur_active_num].addr_len > 0 && old_active_num != pdivoom_voice_handle->cur_active_num)
							//	EspAudioPlay(pdivoom_voice_handle->voice_arr[pdivoom_voice_handle->cur_active_num].voice_addr, 0);
							//只有在新来的不是重复的,并且是有效的,才进行stop,然后再在队列里面找到下一个,一定是刚刚加进去的那个进行播放
							if(need_stop)
							{
								EspAudioStop();
							}
						}
					}
					divoom_system_free_lock(pdivoom_voice_handle->lock);
				}
				break;
			case DIVOOM_VOICE_PAUSE:
				/*{
					divoom_system_get_lock(pdivoom_voice_handle->lock);
					//进行暂停操作
					//基本上不会有暂停操作了,因为除了fm就是提示音,这俩一个是直播流一个是文件流
					//每一个都是唯一的一个index
					if(msg->data)
					{
						if(*((int*)(msg->data)) == pdivoom_voice_handle->cur_active_num)
						{
							//是当前正在播放的就在task里面发送pause
							EspAudioPause();
						}
						else
						{
							//否则就直接发送pause
							//pdivoom_voice_handle->voice_arr[*(int*)(msg->data)].play_cb(DIVOOM_VOICE_PLAY_PAUSE, pdivoom_voice_handle->voice_arr[*(int*)(msg->data)].type, 0);
							voiceinfo.status = DIVOOM_VOICE_PLAY_PAUSE;
							voiceinfo.event_active_num = *((int*)(msg->data));
							xQueueSend(pdivoom_voice_handle->voice_noplay_status, &voiceinfo, 0);
						}
					}
					divoom_system_free_lock(pdivoom_voice_handle->lock);
				}
				break;*/
			case DIVOOM_VOICE_STOP:
				{
					uint8 num = 0xFF;
					divoom_system_get_lock(pdivoom_voice_handle->lock);
					//停止播放
					if(msg->data)
					{
						num = divoom_voice_app_find_same_play(pdivoom_voice_handle, (Pdivoom_voice_play_info)msg->data);
						//播放状态要先stop,然后再从队列里删除掉
						if(num == pdivoom_voice_handle->cur_active_num)
						{
							EspAudioStatusGet(&playinfo);
							//如果是在播放状态,那么先结束当前播放,在task里面处理删除操作
							if(playinfo.status == AUDIO_STATUS_PLAYING)
								EspAudioStop();
							else
							{
								//先执行stop,在从队列消除
								voiceinfo.status = DIVOOM_VOICE_PLAY_STOP;
								voiceinfo.event_active_num = num;
								xQueueSend(pdivoom_voice_handle->voice_noplay_status, &voiceinfo, 0);
							}
						}
						else
						{
							//主动调用callback函数,告知事件模块,当前已经取消了
							voiceinfo.status = DIVOOM_VOICE_PLAY_STOP;
							voiceinfo.event_active_num = num;
							xQueueSend(pdivoom_voice_handle->voice_noplay_status, &voiceinfo, 0);
						}
					}
					divoom_system_free_lock(pdivoom_voice_handle->lock);
				}
				break;
				//vol 跨度是3,给0-14,总共15级音量显示
			case DIVOOM_VOICE_SET_VOLUME:
				{
					if(msg->data && pdivoom_voice_handle)
					{
						if(*(uint8 *)msg->data >= 15)
							*(uint8 *)msg->data = 14;
						pdivoom_voice_handle->cur_vol = *(uint8 *)msg->data;
						EspAudioVolSet(DIVOOM_VOL[pdivoom_voice_handle->cur_vol]);
					}
				}
				break;
			case DIVOOM_VOICE_ADD_VOLUME:
				{
					//精度为3?每次增加减少音量7
					if(pdivoom_voice_handle->cur_vol++ >= 14)
					{
						pdivoom_voice_handle->cur_vol = 14;
					}
					EspAudioVolSet(DIVOOM_VOL[pdivoom_voice_handle->cur_vol]);
				}
				break;
			case DIVOOM_VOICE_SUBTRACT_VOLUME:
				{
					if(pdivoom_voice_handle->cur_vol != 0)
					{
						pdivoom_voice_handle->cur_vol--;
					}
					EspAudioVolSet(DIVOOM_VOL[pdivoom_voice_handle->cur_vol]);
				}
				break;
			case DIVOOM_VOICE_SET_MUTE:
				{
					EspAudioVolSet(DIVOOM_VOL[0]);
				}
				break;
				//保留选项
			case DIVOOM_VOICE_SEEK:
				break;
		}
	}
	return 0;
}

void divoom_voice_app_init(void)
{
	if(pdivoom_voice_handle == NULL)
	{
		pdivoom_voice_handle = divoom_malloc(sizeof(divoom_voice_ctrl), DIVOOM_MODEL_VOICE_APP, __LINE__, __func__);
	}
	else
	{
		goto ERROR_RETURN;
	}
	pdivoom_voice_handle->all_voice_num = 0;
	pdivoom_voice_handle->cur_active_num = 0xFF;
	pdivoom_voice_handle->lock = divoom_system_create_lock();
	//默认音量
	pdivoom_voice_handle->cur_vol = DIVOOM_VOICE_DEFAULT_VOL;
	//接收player 那边发送的状态
	pdivoom_voice_handle->voice_play_status = xQueueCreate(5, sizeof(PlayerStatus));
	pdivoom_voice_handle->voice_noplay_status = xQueueCreate(5, sizeof(VOICE_PLAY_STATUS));
	pdivoom_voice_handle->queue_set_all = xQueueCreateSet(5);
	//把这个信号量加到播放的队列中,会在mediacontrol里面的broadcasting里面对每一个加进来的队列进行push
	EspAudioStatusListenerAdd(pdivoom_voice_handle->voice_play_status);

	xQueueAddToSet(pdivoom_voice_handle->voice_play_status, pdivoom_voice_handle->queue_set_all);
	xQueueAddToSet(pdivoom_voice_handle->voice_noplay_status, pdivoom_voice_handle->queue_set_all);

	pdivoom_voice_handle->task_handle = divoom_system_create_task_prio(divoom_voice_app_task, "VOICE_TASK", pdivoom_voice_handle, DIVOOM_VOICE_STACK, DIVOOM_SYSTEM_TASK_DEFAULT_PRIO);

	divoom_sys_model_crl_register(DIVOOM_MODEL_VOICE_APP, divoom_voice_app_event);
	return;

ERROR_RETURN:
	if(pdivoom_voice_handle->lock)
	{
		divoom_system_destroy_lock(pdivoom_voice_handle->lock);
	}
	if(pdivoom_voice_handle->voice_play_status)
	{
		vQueueDelete(pdivoom_voice_handle->voice_play_status);
	}
	if(pdivoom_voice_handle->voice_noplay_status)
	{
		vQueueDelete(pdivoom_voice_handle->voice_noplay_status);
	}
	if(pdivoom_voice_handle)
	{
		divoom_free(pdivoom_voice_handle);
	}
	return;
}
#endif



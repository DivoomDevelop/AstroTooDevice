#if 1

#ifndef _DIVOOM_VOICE_APP_H_
#define _DIVOOM_VOICE_APP_H_

typedef enum
{
	//遵循player/MediaControl/include/AudioDef.h
	DIVOOM_VOICE_PLAY_UNKNOW,
	//开始播放,对应playing
	DIVOOM_VOICE_PLAY_PLAYING,
	//正常结束,对应finished
	DIVOOM_VOICE_PLAY_OVER,
	//暂停状态,可逆,paused
	DIVOOM_VOICE_PLAY_PAUSE,
	//结束状态,不可逆,执行stop需要把当前的播放信息清除,对应stop
	DIVOOM_VOICE_PLAY_STOP,
	//播放错误,有可能是地址无法播放啊,网络问题啊等等,对应error
	DIVOOM_VOICE_PLAY_ERROR,
	//保留
	DIVOOM_VOICE_PLAU_SEEK,
}VOICE_PLAY_STATUS;

typedef enum
{
	DIVOOM_VOICE_LIVE_STREAM,
	DIVOOM_VOICE_FILE_STREAM,
}VOICE_PLAY_TYPE;

enum
{
	//参数divoom_voice_play_info
	DIVOOM_VOICE_PLAY,
	//暂停,不从队列pop,参数是 divoom_voice_play_info
	//尽量不使用pause
	DIVOOM_VOICE_PAUSE,
	//相当于destroy,从队列pop,然后从队列里找到第一个不为空的,更新cur_active_num,但是不播放,由schedule控制下一个激活的模块进行播放
	//参数是 divoom_voice_play_info
	DIVOOM_VOICE_STOP,
	//接收一个uint8类型的值
	DIVOOM_VOICE_SET_VOLUME,
	DIVOOM_VOICE_ADD_VOLUME,
	DIVOOM_VOICE_SUBTRACT_VOLUME,
	DIVOOM_VOICE_SET_MUTE,
	DIVOOM_VOICE_SEEK,
};

enum
{
    DIVOOM_VOICE_IN_STATUS_PLAY = 1,
    DIVOOM_VOICE_IN_STATUS_PAUSE,
    DIVOOM_VOICE_IN_STATUS_STOP,
    DIVOOM_VOICE_IN_STATUS_NEXT,
    DIVOOM_VOICE_IN_STATUS_PRE,
    DIVOOM_VOICE_IN_STATUS_SAVE_CUR_PLAY,
    DIVOOM_VOICE_IN_STATUS_PLAY_PRE_VOICE,
};

enum
{
    DIVOOM_VOICE_APP_PLAY_EVENT,
};
enum
{
	DIVOOM_VOICE_PLAY_STATUS_NO_EXIST,
	DIVOOM_VOICE_PLAY_STATUS_END,
	DIVOOM_VOICE_PLAY_STATUS_ERR,
	DIVOOM_VOICE_PLAY_STATUS_PLAY,
	DIVOOM_VOICE_PLAY_STATUS_PAUSE,
	DIVOOM_VOICE_PLAY_STATUS_NEXT,
};


//针对文件流有个play_position的位置,标志当前播放的位置,如果是直播流忽略,暂定,可能会改
//增加一个errmsg,看后面激活的APP要不要进行处理:比如闹钟,播放的是错误的url,那么播放一个默认的音乐
//应该只有在radio模块才有可能处理 error_msg
//typedef void (*VOICE_PLAY_STATUS_CB)(VOICE_PLAY_STATUS status, AudioErr error_msg, VOICE_PLAY_TYPE type, uint8 cur_play_index, uint32 play_position);

typedef struct _divoom_voice_play_info_
{
	//model_id 和 event_id 就算是没有音乐要播放也要添加,作为删除和播放的依据
	//增加一个唯一标识符
	uint16 model_id;
	//是什么时间触发的voice,这样每个模块可以有多个voice触发
	uint8 event_id;
	uint32 play_position;
	//播放地址,可以使net, spiffs等等
	char *voice_addr;
	uint16 addr_len;
	bool loop_flag;
	//VOICE_PLAY_STATUS_CB play_cb;
	VOICE_PLAY_TYPE type;
}divoom_voice_play_info, *Pdivoom_voice_play_info;

void divoom_voice_app_init(void);

#endif

#endif


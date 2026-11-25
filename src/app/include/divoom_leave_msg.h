#ifndef _divoom_leave_msg_h_
#define _divoom_leave_msg_h_

typedef struct _divoom_leave_msg_voice_item
{
    char user_name[128];
    char file_name[64];
}divoom_leave_msg_voice_item, *Pdivoom_leave_msg_voice_item;

enum
{
    DIVOOM_LEAVE_MSG_APP_EVENT_NOTICE_PLAY,
    DIVOOM_LEAVE_MSG_APP_EVENT_RUN_PLAY,
};

enum DIVOOM_LEAVE_MSG_COMMAND
{
    DIVOOM_LEAVE_MSG_LIST_EVENT,       //留言更新
    DIVOOM_LEAVE_MSG_PLAY_OVER_EVENT,  //播放完前一条
    DIVOOM_LEAVE_MSG_ADD_EVENT,       //增加
    DIVOOM_LEAVE_MSG_PLAY_EVENT,      //播放最近一条留言，无参数
    DIVOOM_LEAVE_MSG_NOTICE_OVER_EVENT,      //提醒结束
};

void divoom_leave_msg_init(void);
void divoom_leave_msg_main(void);

void divoom_leave_msg_deinit(void);

uint8 divoom_leave_msg_app_schedule_play(uint8 event_id, void *user_data, uint32 resouce_type);


uint8 divoom_leave_msg_app_schedule_pause(uint8 event_id, void *user_data, uint8 active_model, uint8 active_event, uint32 resouce_type);


uint8 divoom_leave_msg_app_schedule_destroy(uint8 event_id, void *user_data, uint32 status, uint32 resouce_type);

uint8 divoom_leave_msg_app_schedule_key_handle(uint8 key_id, uint8 key_type, uint32 resouce_type);

#endif


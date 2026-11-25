#ifndef _divoom_leave_msg_ctrl_h_
#define _divoom_leave_msg_ctrl_h_

enum {
	DIVOOM_LEAVE_MSG_CTRL_EVENT_LOCAL_RECORD,
	DIVOOM_LEAVE_MSG_CTRL_EVENT_STOP_RECORD,
	DIVOOM_LEAVE_MSG_CTRL_EVENT_SET_VOICE_DATA,
	DIVOOM_LEAVE_MSG_CTRL_EVENT_LVGL_EXIT,
	/** App Voice/Upload 下载完成：写 meta + 顶栏提醒 */
	DIVOOM_LEAVE_MSG_CTRL_EVENT_APP_DOWNLOAD_DONE,
};

enum {
	DIVOOM_LEAVE_MSG_CTRL_LOCAL_RECORD_EVENT,
};

typedef enum {
	DIVOOM_LEAVE_MSG_WAIT,
	DIVOOM_LEAVE_MSG_START_RECORD,
	DIVOOM_LEAVE_MSG_STOP_RECORD,
} DIVOOM_LEAVE_MSG_STATUS;

void divoom_leave_msg_ctrl_init(void);
uint8 divoom_leave_msg_ctrl_event(Pdivoom_sys_msg_info msg);
/** 磁盘上仍有可播留言则弹顶栏条（开机进表盘、App 下发共用判定） */
void divoom_leave_msg_ctrl_notice_if_clip_on_disk(void);

uint8 divoom_leave_msg_ctrl_schedule_play_callback(uint8 event_id, void *user_data,
						   uint32 resouce_type);
uint8 divoom_leave_msg_ctrl_schedule_pause_callback(uint8 event_id, void *user_data,
						    uint8 active_model, uint8 active_event,
						    uint32 resouce_type);
uint8 divoom_leave_msg_ctrl_schedule_destroy_callback(uint8 event_id, void *user_data,
						      uint32 status, uint32 resouce_type);

int divoom_leave_msg_ctrl_get_status(void);
int divoom_leave_msg_ctrl_get_duration_sec(void);
void divoom_leave_msg_ctrl_stop_local_record_sync(void);
void divoom_leave_msg_ctrl_abort_local_record(void);
/* flowtoo 式异步 LOCAL_RECORD：UI push 前分配 gen 写入 msg.attr，abort 后迟到消息作废 */
uint8 divoom_leave_msg_ctrl_alloc_record_gen(void);
void divoom_leave_msg_ctrl_clear_saved_clip(void);

#endif

#include "divoom.h"
#include "divoom_leave_msg_ctrl.h"
#include "divoom_leave_msg_record.h"
#include "divoom_local_music_ctrl.h"
#include "divoom_manage_image.h"
#include "divoom_mic_record.h"
#include "divoom_voice_json.h"
#include "divoom_app_schedule.h"
#include "divoom_lvgl_top_leave_msg_notice_menu.h"

#include <string.h>

#define DIVOOM_LEAVE_MSG_CTRL_RECORD_MAX_SEC  60u
#define LEAVE_MSG_DBG_TAG  "LEAVE_MSG_DBG"

typedef struct {
	DIVOOM_LEAVE_MSG_STATUS leave_status;
	uint32 msg_time_sec;
	uint32 record_enc_sec;     /* 0x64 给出的秒；0=尚未结束 */
	uint8 record_from_lvgl;
	uint8 record_cmd_gen;
} divoom_leave_msg_ctrl_ctx_t;

static divoom_leave_msg_ctrl_ctx_t s_leave_msg_ctrl;

static void divoom_leave_msg_ctrl_finish_download(int ret)
{
	divoom_sys_msg_info msg_info = {0};

	if (ret != 1) {
		divoom_print("%s: APP download callback ret=%d, skip\n", LEAVE_MSG_DBG_TAG, ret);
		return;
	}
	msg_info.src_id = DIVOOM_MODEL_LEAVE_MSG;
	msg_info.dest_id = DIVOOM_MODEL_LEAVE_MSG;
	msg_info.msg_id = DIVOOM_LEAVE_MSG_CTRL_EVENT_APP_DOWNLOAD_DONE;
	msg_info.msg_type = 1;
	divoom_sys_model_crl_msg_push(&msg_info);
}

static void divoom_leave_msg_ctrl_refresh_duration_from_file(void)
{
	char path[256];
	const char *use_path = NULL;
	uint32 sec;

	s_leave_msg_ctrl.msg_time_sec = 0;
	if (divoom_leave_msg_record_temp_clip_bytes() >= (int)DIVOOM_LEAVE_MSG_MIN_CLIP_BYTES
	    && divoom_leave_msg_record_clip_path_is_playable(DIVOOM_LEAVE_MSG_RECORD_TMP_MP3)) {
		use_path = DIVOOM_LEAVE_MSG_RECORD_TMP_MP3;
	} else {
		if (!divoom_voice_read_get_filename((uint32)DIVOOM_VOICE_ID_LEAVE_MSG, path))
			return;
		if (!divoom_port_file_exists(path))
			return;
		if (divoom_app_com_get_file_size(path) < (int)DIVOOM_LEAVE_MSG_MIN_CLIP_BYTES)
			return;
		if (!divoom_leave_msg_record_clip_path_is_playable(path))
			return;
		use_path = path;
	}
	sec = divoom_mic_record_file_duration_sec(use_path);
	s_leave_msg_ctrl.msg_time_sec = sec;
}

static void divoom_leave_msg_ctrl_record_done_cb(const char *path, uint32 duration_sec, int result)
{
	(void)path;
	(void)result;
	s_leave_msg_ctrl.record_enc_sec = duration_sec > 0u ? duration_sec : 1u;
	if (s_leave_msg_ctrl.record_enc_sec > DIVOOM_LEAVE_MSG_CTRL_RECORD_MAX_SEC)
		s_leave_msg_ctrl.record_enc_sec = DIVOOM_LEAVE_MSG_CTRL_RECORD_MAX_SEC;
}

static int divoom_leave_msg_ctrl_start_record(void)
{
	s_leave_msg_ctrl.record_enc_sec = 0;
	s_leave_msg_ctrl.msg_time_sec = 0;
	if (divoom_local_music_ctrl_voice_clip_record_start(
		    (char *)DIVOOM_LEAVE_MSG_RECORD_TMP_MP3,
		    DIVOOM_LEAVE_MSG_CTRL_RECORD_MAX_SEC,
		    divoom_leave_msg_ctrl_record_done_cb) != 0)
		return -1;
	s_leave_msg_ctrl.leave_status = DIVOOM_LEAVE_MSG_START_RECORD;
	return 0;
}

static void divoom_leave_msg_ctrl_stop_record_internal(void)
{
	divoom_local_music_ctrl_voice_clip_record_stop_sync();
	s_leave_msg_ctrl.leave_status = DIVOOM_LEAVE_MSG_STOP_RECORD;
	if (s_leave_msg_ctrl.record_enc_sec > 0u)
		s_leave_msg_ctrl.msg_time_sec = s_leave_msg_ctrl.record_enc_sec;
	else
		divoom_leave_msg_ctrl_refresh_duration_from_file();
}

uint8 divoom_leave_msg_ctrl_alloc_record_gen(void)
{
	return ++s_leave_msg_ctrl.record_cmd_gen;
}

static void divoom_leave_msg_ctrl_invalidate_record_cmds(void)
{
	s_leave_msg_ctrl.record_cmd_gen++;
}

void divoom_leave_msg_ctrl_stop_local_record_sync(void)
{
	divoom_app_schedule_item_info event_info = {0};

	divoom_leave_msg_ctrl_stop_record_internal();
	event_info.model_id = DIVOOM_MODEL_LEAVE_MSG;
	event_info.event_id = DIVOOM_LEAVE_MSG_CTRL_LOCAL_RECORD_EVENT;
	event_info.resource_type = DIVOOM_SCHEDULE_VOICE_RESOUCE_TYPE;
	divoom_app_schedule_cancel(&event_info);
	s_leave_msg_ctrl.record_from_lvgl = 0;
	s_leave_msg_ctrl.leave_status = DIVOOM_LEAVE_MSG_WAIT;
}

int divoom_leave_msg_ctrl_get_status(void)
{
	if (s_leave_msg_ctrl.leave_status == DIVOOM_LEAVE_MSG_START_RECORD
	    && !divoom_local_music_ctrl_voice_clip_record_is_active())
		s_leave_msg_ctrl.leave_status = DIVOOM_LEAVE_MSG_STOP_RECORD;
	return (int)s_leave_msg_ctrl.leave_status;
}

int divoom_leave_msg_ctrl_get_duration_sec(void)
{
	if (s_leave_msg_ctrl.msg_time_sec == 0u
	    && s_leave_msg_ctrl.leave_status != DIVOOM_LEAVE_MSG_START_RECORD)
		divoom_leave_msg_ctrl_refresh_duration_from_file();
	return (int)s_leave_msg_ctrl.msg_time_sec;
}

void divoom_leave_msg_ctrl_clear_saved_clip(void)
{
	s_leave_msg_ctrl.msg_time_sec = 0;
	s_leave_msg_ctrl.record_enc_sec = 0;
}

uint8 divoom_leave_msg_ctrl_schedule_pause_callback(uint8 event_id, void *user_data,
						    uint8 active_model, uint8 active_event,
						    uint32 resouce_type)
{
	divoom_sys_msg_info msg_info = {0};

	(void)user_data;
	(void)active_model;
	(void)active_event;
	(void)resouce_type;

	if (event_id == DIVOOM_LEAVE_MSG_CTRL_LOCAL_RECORD_EVENT) {
		msg_info.src_id = DIVOOM_MODEL_LEAVE_MSG;
		msg_info.dest_id = DIVOOM_MODEL_LEAVE_MSG;
		msg_info.msg_type = 1;
		msg_info.msg_id = DIVOOM_LEAVE_MSG_CTRL_EVENT_STOP_RECORD;
		divoom_sys_model_crl_msg_push(&msg_info);
	}
	return 0;
}

uint8 divoom_leave_msg_ctrl_schedule_play_callback(uint8 event_id, void *user_data,
						   uint32 resouce_type)
{
	(void)event_id;
	(void)user_data;
	(void)resouce_type;
	return 1;
}

uint8 divoom_leave_msg_ctrl_schedule_destroy_callback(uint8 event_id, void *user_data,
						      uint32 status, uint32 resouce_type)
{
	(void)event_id;
	(void)user_data;
	(void)status;
	(void)resouce_type;
	divoom_local_music_ctrl_voice_clip_record_stop_sync();
	s_leave_msg_ctrl.leave_status = DIVOOM_LEAVE_MSG_WAIT;
	return 1;
}

uint8 divoom_leave_msg_ctrl_event(Pdivoom_sys_msg_info msg)
{
	divoom_app_schedule_item_info event_info = {0};

	if (!msg)
		return 0;

	switch (msg->msg_id) {
	case DIVOOM_LEAVE_MSG_CTRL_EVENT_SET_VOICE_DATA: {
		Pdivoom_manage_image_item_info pimage_info;
		Pdivoom_voice_json_voice_item_info pvoice_info;

		if (!msg->data)
			break;
		pvoice_info = (Pdivoom_voice_json_voice_item_info)msg->data;
		pimage_info = (Pdivoom_manage_image_item_info)divoom_system_malloc(
			sizeof(divoom_manage_image_item_info));
		if (!pimage_info)
			break;
		if (divoom_local_music_ctrl_voice_clip_record_is_active())
			divoom_leave_msg_ctrl_stop_record_internal();
		strcpy(pimage_info->file_addr, pvoice_info->path);
		pimage_info->image_id = DIVOOM_VOICE_ID_LEAVE_MSG;
		pimage_info->pcallback = divoom_leave_msg_ctrl_finish_download;
		pimage_info->flag = 1;
		divoom_manage_image_del_direct(DIVOOM_MANAGE_IMAGE_LEAVEMSG_FILE,
					     &pimage_info->image_id, 1);
		divoom_manage_image_add_local_font(DIVOOM_MANAGE_IMAGE_LEAVEMSG_FILE, pimage_info, 1);
		divoom_system_free(pimage_info);
		break;
	}
	case DIVOOM_LEAVE_MSG_CTRL_EVENT_LOCAL_RECORD:
		if (msg->attr != 0 && msg->attr != s_leave_msg_ctrl.record_cmd_gen) {
			divoom_print("%s: LOCAL_RECORD stale attr=%u gen=%u\n",
				     LEAVE_MSG_DBG_TAG, (unsigned)msg->attr,
				     (unsigned)s_leave_msg_ctrl.record_cmd_gen);
			break;
		}
		divoom_leave_msg_ctrl_stop_local_record_sync();
		s_leave_msg_ctrl.record_from_lvgl =
			(msg->data && msg->data_len > 0 && msg->data[0] == 1) ? 1u : 0u;
		event_info.model_id = DIVOOM_MODEL_LEAVE_MSG;
		event_info.event_id = DIVOOM_LEAVE_MSG_CTRL_LOCAL_RECORD_EVENT;
		event_info.attr = 0;
		event_info.resource_type = DIVOOM_SCHEDULE_VOICE_RESOUCE_TYPE;
		if (DIVOOM_APP_SCHEDULE_RET_RUN != divoom_app_schedule_push(&event_info)) {
			divoom_app_schedule_cancel(&event_info);
			if (DIVOOM_APP_SCHEDULE_RET_RUN != divoom_app_schedule_push(&event_info)) {
				divoom_print("%s: LOCAL_RECORD schedule_push fail st=%d active=%d\n",
					     LEAVE_MSG_DBG_TAG, s_leave_msg_ctrl.leave_status,
					     divoom_local_music_ctrl_voice_clip_record_is_active());
				divoom_leave_msg_ctrl_abort_local_record();
				break;
			}
		}
		if (divoom_leave_msg_ctrl_start_record() != 0) {
			divoom_print("%s: LOCAL_RECORD start_record fail\n", LEAVE_MSG_DBG_TAG);
			divoom_app_schedule_cancel(&event_info);
			s_leave_msg_ctrl.leave_status = DIVOOM_LEAVE_MSG_WAIT;
		} else {
			divoom_print("%s: LOCAL_RECORD ok tmp=%d active=%d\n", LEAVE_MSG_DBG_TAG,
				     divoom_leave_msg_record_temp_clip_bytes(),
				     divoom_local_music_ctrl_voice_clip_record_is_active());
		}
		break;

	case DIVOOM_LEAVE_MSG_CTRL_EVENT_STOP_RECORD:
		divoom_leave_msg_ctrl_stop_local_record_sync();
		divoom_print("%s: STOP_RECORD st=%d tmp=%d dur=%d\n", LEAVE_MSG_DBG_TAG,
			     s_leave_msg_ctrl.leave_status,
			     divoom_leave_msg_record_temp_clip_bytes(),
			     s_leave_msg_ctrl.msg_time_sec);
		break;

	case DIVOOM_LEAVE_MSG_CTRL_EVENT_LVGL_EXIT:
		divoom_local_music_ctrl_voice_clip_record_stop_sync();
		event_info.model_id = DIVOOM_MODEL_LEAVE_MSG;
		event_info.event_id = DIVOOM_LEAVE_MSG_CTRL_LOCAL_RECORD_EVENT;
		event_info.resource_type = DIVOOM_SCHEDULE_VOICE_RESOUCE_TYPE;
		divoom_app_schedule_cancel(&event_info);
		s_leave_msg_ctrl.leave_status = DIVOOM_LEAVE_MSG_WAIT;
		s_leave_msg_ctrl.record_from_lvgl = 0;
		divoom_print("%s: LVGL_EXIT st=WAIT tmp=%d\n", LEAVE_MSG_DBG_TAG,
			     divoom_leave_msg_record_temp_clip_bytes());
		break;

	case DIVOOM_LEAVE_MSG_CTRL_EVENT_APP_DOWNLOAD_DONE: {
		char path[256];
		uint32 dur;

		divoom_leave_msg_ctrl_refresh_duration_from_file();
		dur = s_leave_msg_ctrl.msg_time_sec;
		if (!divoom_leave_msg_record_get_playback_path(path, sizeof(path))) {
			divoom_print("%s: APP_DOWNLOAD_DONE no playable clip\n", LEAVE_MSG_DBG_TAG);
			break;
		}
		if (dur == 0u)
			dur = 1u;
		divoom_leave_msg_record_write_last_saved_meta(dur);
		divoom_print("%s: APP_DOWNLOAD_DONE meta+notice dur=%u path=%s\n",
			     LEAVE_MSG_DBG_TAG, (unsigned)dur, path);
		/* model 线程调用；notice_add 自行加/解 lvgl 锁（同 social/anniversary） */
		divoom_lvgl_top_leave_msg_notice_init(dur);
		break;
	}

	default:
		break;
	}
	return 1;
}

void divoom_leave_msg_ctrl_abort_local_record(void)
{
	divoom_app_schedule_item_info event_info = {0};

	divoom_leave_msg_ctrl_invalidate_record_cmds();
	divoom_local_music_ctrl_voice_clip_record_stop_sync();
	event_info.model_id = DIVOOM_MODEL_LEAVE_MSG;
	event_info.event_id = DIVOOM_LEAVE_MSG_CTRL_LOCAL_RECORD_EVENT;
	event_info.resource_type = DIVOOM_SCHEDULE_VOICE_RESOUCE_TYPE;
	divoom_app_schedule_cancel(&event_info);
	s_leave_msg_ctrl.leave_status = DIVOOM_LEAVE_MSG_WAIT;
	s_leave_msg_ctrl.record_from_lvgl = 0;
	divoom_print("%s: abort_local_record tmp=%d active=%d\n", LEAVE_MSG_DBG_TAG,
		     divoom_leave_msg_record_temp_clip_bytes(),
		     divoom_local_music_ctrl_voice_clip_record_is_active());
}

void divoom_leave_msg_ctrl_notice_if_clip_on_disk(void)
{
	char path[256];
	uint32 dur = 0;
	int mon = 0;
	int mday = 0;

	if (!divoom_leave_msg_record_get_playback_path(path, sizeof(path)))
		return;
	if (!divoom_leave_msg_record_read_last_saved_meta(&dur, &mon, &mday) || dur == 0u) {
		divoom_leave_msg_ctrl_refresh_duration_from_file();
		dur = s_leave_msg_ctrl.msg_time_sec;
	}
	if (dur == 0u)
		dur = 1u;
	divoom_print("%s: notice_if_clip dur=%u path=%s\n",
		     LEAVE_MSG_DBG_TAG, (unsigned)dur, path);
	divoom_lvgl_top_leave_msg_notice_init(dur);
}

void divoom_leave_msg_ctrl_init(void)
{
	memset(&s_leave_msg_ctrl, 0, sizeof(s_leave_msg_ctrl));
	s_leave_msg_ctrl.leave_status = DIVOOM_LEAVE_MSG_WAIT;
	divoom_sys_model_crl_register(DIVOOM_MODEL_LEAVE_MSG, divoom_leave_msg_ctrl_event);
}

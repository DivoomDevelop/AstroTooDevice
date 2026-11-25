#include "divoom.h"
#include "divoom_app_com.h"
#include "divoom_leave_msg_record.h"
#include "divoom_manage_image.h"
#include "divoom_voice_read.h"
#include "divoom_local_music_ctrl.h"
#include "divoom_music_ctrl.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#define DIVOOM_LEAVE_MSG_META_NAME "leave_msg_last.meta"

static uint8 divoom_leave_msg_record_capture_active;

static int divoom_leave_msg_record_path_playable(const char *path)
{
	return divoom_voice_clip_path_coding_type(path) != DIVOOM_TONE_CODING_NONE;
}

static int divoom_leave_msg_record_on_disk_playable(char *path, size_t path_len)
{
	if (!path || path_len < 4)
		return 0;
	path[0] = '\0';
	if (!divoom_voice_read_get_filename((uint32)DIVOOM_VOICE_ID_LEAVE_MSG, path)
	    || !divoom_port_file_exists(path))
		return 0;
	if (divoom_app_com_get_file_size(path) < DIVOOM_LEAVE_MSG_MIN_CLIP_BYTES)
		return 0;
	if (!divoom_leave_msg_record_path_playable(path)) {
		divoom_print("%s: invalid leave msg on disk: %s\n", __func__, path);
		return 0;
	}
	return 1;
}

void divoom_leave_msg_record_write_last_saved_meta(uint32_t duration_sec)
{
	char dirpath[160];
	char metapath[192];
	time_t t = time(NULL);
	struct tm ltm;
	FILE *fp;

	localtime_r(&t, &ltm);
	divoom_manage_image_download_get_path_name(DIVOOM_MANAGE_IMAGE_LEAVEMSG_FILE, 0, dirpath);
	divoom_app_com_check_and_mk_dir(dirpath);
	snprintf(metapath, sizeof(metapath), "%s/%s", dirpath, DIVOOM_LEAVE_MSG_META_NAME);
	fp = divoom_port_file_open(metapath, "w");
	if (!fp) {
		divoom_print("%s: fopen failed %s\n", __func__, metapath);
		return;
	}
	fprintf(fp, "%u\n%d\n%d\n", (unsigned)duration_sec, ltm.tm_mon + 1, ltm.tm_mday);
	divoom_port_file_close(fp);
}

int divoom_leave_msg_record_read_last_saved_meta(uint32_t *duration_sec, int *mon, int *mday)
{
	char dirpath[160];
	char metapath[192];
	FILE *fp;
	unsigned ds = 0;
	int mo = 0, dd = 0;

	if (!duration_sec || !mon || !mday)
		return 0;
	if (!divoom_voice_read_file_is_exit((uint32)DIVOOM_VOICE_ID_LEAVE_MSG))
		return 0;
	divoom_manage_image_download_get_path_name(DIVOOM_MANAGE_IMAGE_LEAVEMSG_FILE, 0, dirpath);
	snprintf(metapath, sizeof(metapath), "%s/%s", dirpath, DIVOOM_LEAVE_MSG_META_NAME);
	fp = divoom_port_file_open(metapath, "r");
	if (!fp)
		return 0;
	if (fscanf(fp, "%u%d%d", &ds, &mo, &dd) != 3) {
		divoom_port_file_close(fp);
		return 0;
	}
	divoom_port_file_close(fp);
	if (mo < 1 || mo > 12 || dd < 1 || dd > 31)
		return 0;
	*duration_sec = (uint32)ds;
	*mon = mo;
	*mday = dd;
	return 1;
}

void divoom_leave_msg_record_stop_capture(void)
{
	if (!divoom_leave_msg_record_capture_active
	    && !divoom_local_music_ctrl_voice_clip_record_is_active())
		return;
	divoom_local_music_ctrl_voice_clip_record_stop_sync();
	divoom_leave_msg_record_capture_active = 0;
}

int divoom_leave_msg_record_start_capture(void)
{
	divoom_leave_msg_record_stop_capture();
	divoom_app_com_del_file((char *)DIVOOM_LEAVE_MSG_RECORD_TMP_MP3);
	if (divoom_local_music_ctrl_voice_clip_record_start(
		    (char *)DIVOOM_LEAVE_MSG_RECORD_TMP_MP3, 60, NULL) != 0) {
		divoom_print("%s: voice_clip_record_start failed\n", __func__);
		return -1;
	}
	divoom_leave_msg_record_capture_active = 1;
	return 0;
}

int divoom_leave_msg_record_capture_is_active(void)
{
	if (divoom_leave_msg_record_capture_active)
		return 1;
	return divoom_local_music_ctrl_voice_clip_record_is_active();
}

void divoom_leave_msg_record_delete_temp_file(void)
{
	divoom_leave_msg_record_stop_capture();
	divoom_app_com_del_file((char *)DIVOOM_LEAVE_MSG_RECORD_TMP_MP3);
}

int divoom_leave_msg_record_temp_clip_bytes(void)
{
	return divoom_app_com_get_file_size((char *)DIVOOM_LEAVE_MSG_RECORD_TMP_MP3);
}

int divoom_leave_msg_record_commit_to_leave_msg_file(uint32_t duration_sec)
{
	char dirpath[160];
	char dest[256];
	uint32 voice_id;
	int sz;

	divoom_leave_msg_record_stop_capture();
	sz = divoom_leave_msg_record_temp_clip_bytes();
	if (sz < DIVOOM_LEAVE_MSG_MIN_CLIP_BYTES) {
		if (!divoom_leave_msg_record_on_disk_playable(dest, sizeof(dest)))
			return 0;
		if (duration_sec > 0u)
			divoom_leave_msg_record_write_last_saved_meta(duration_sec);
		return 1;
	}
	if (!divoom_leave_msg_record_path_playable(DIVOOM_LEAVE_MSG_RECORD_TMP_MP3)) {
		divoom_print("%s: tmp clip header invalid\n", __func__);
		return 0;
	}

	voice_id = DIVOOM_VOICE_ID_LEAVE_MSG;
	divoom_manage_image_del_direct(DIVOOM_MANAGE_IMAGE_LEAVEMSG_FILE, &voice_id, 1);

	divoom_manage_image_download_get_path_name(DIVOOM_MANAGE_IMAGE_LEAVEMSG_FILE, 0, dirpath);
	divoom_app_com_check_and_mk_dir(dirpath);

	divoom_manage_image_download_get_file_name(DIVOOM_MANAGE_IMAGE_LEAVEMSG_FILE,
						  (uint32)DIVOOM_VOICE_ID_LEAVE_MSG, dest);

	divoom_app_com_copy_file((char *)DIVOOM_LEAVE_MSG_RECORD_TMP_MP3, dest);
	if (!divoom_port_file_exists(dest)) {
		divoom_print("%s: copy failed -> %s\n", __func__, dest);
		return 0;
	}
	if (!divoom_leave_msg_record_path_playable(dest)) {
		divoom_print("%s: committed file header invalid: %s\n", __func__, dest);
		divoom_app_com_del_file(dest);
		return 0;
	}
	divoom_leave_msg_record_write_last_saved_meta(duration_sec);
	divoom_app_com_del_file((char *)DIVOOM_LEAVE_MSG_RECORD_TMP_MP3);
	return 1;
}

int divoom_leave_msg_record_get_playback_path(char *path_buf, size_t path_buf_len)
{
	if (!path_buf || path_buf_len < 4)
		return 0;
	path_buf[0] = '\0';
	/* 方案 A：停录 commit 后只播磁盘 240.bin，不播 tmp */
	if (!divoom_leave_msg_record_on_disk_playable(path_buf, path_buf_len)) {
		divoom_print("LEAVE_MSG_DBG: get_playback_path no valid on_disk clip\n");
		return 0;
	}
	divoom_print("LEAVE_MSG_DBG: get_playback_path on_disk -> %s\n", path_buf);
	return 1;
}

int divoom_leave_msg_record_clip_path_is_playable(const char *path)
{
	return divoom_leave_msg_record_path_playable(path);
}

void divoom_leave_msg_record_delete_saved_on_disk(void)
{
	char dirpath[160];
	char metapath[192];
	uint32 voice_id = DIVOOM_VOICE_ID_LEAVE_MSG;

	divoom_manage_image_del_direct(DIVOOM_MANAGE_IMAGE_LEAVEMSG_FILE, &voice_id, 1);
	divoom_manage_image_download_get_path_name(DIVOOM_MANAGE_IMAGE_LEAVEMSG_FILE, 0, dirpath);
	snprintf(metapath, sizeof(metapath), "%s/%s", dirpath, DIVOOM_LEAVE_MSG_META_NAME);
	divoom_app_com_del_file(metapath);
}

void divoom_leave_msg_record_stop_preview_playback(void)
{
	divoom_local_music_ctrl_stop_tone();
}

int divoom_leave_msg_record_get_play_volume(void)
{
	return (int)divoom_music_ctrl_get_volume();
}

void divoom_leave_msg_record_safe_discard_after_notice_play(void)
{
	if (divoom_local_music_ctrl_get_tone_status()) {
		divoom_print("%s: skip delete while tone playing\n", __func__);
		return;
	}
	divoom_leave_msg_record_delete_temp_file();
	divoom_leave_msg_record_delete_saved_on_disk();
	divoom_print("%s: leave msg clip discarded after notice play\n", __func__);
}

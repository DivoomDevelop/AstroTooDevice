#include "divoom.h"
#include "divoom_menu_control.h"
#include "divoom_http_client.h"

#include <sys/time.h>
#include <string.h>
#include <strings.h>
typedef struct _divoom_tf_music_ctrl
{
    int mode;
    int play_order;   /* 0=全部 1=收藏 2+=一级目录索引 */
    int total;
    int cur_play_id;
    int cur_folder_id;
    int cur_gord;
    uint32 generation;
    int id_base; /* JL 0/1-based；wire = db_id-1+id_base */
    char music_name[256];
    char cur_path[256];
    char cur_root[DIVOOM_TF_MUSIC_ROOT_NAME_LEN];
    divoom_tf_music_item cur_item;
    Pdivoom_tf_music_item pcur_play_item;
    char disp_name[256];
}divoom_tf_music_ctrl, *Pdivoom_tf_music_ctrl;

static Pdivoom_tf_music_ctrl pdivoom_tf_music_handle = NULL;
static char s_tf_root_name_buf[DIVOOM_TF_MUSIC_ROOT_NAME_LEN];

char *divoom_tf_music_get_disp_name(const char *path)
{
	const char *p;
	const char *last;

	if (path == NULL || path[0] == '\0')
		return (char *)"";
	last = path;
	for (p = path; *p != '\0'; p++)
	{
		if ((*p == '/' || *p == '\\') && p[1] != '\0')
			last = p + 1;
	}
	return (char *)last;
}

static void divoom_tf_music_refresh_cur_from_id(Pdivoom_tf_music_ctrl phandle, int track_id)
{
	char *base;
	divoom_tf_folder_info finfo;

	if (phandle == NULL)
		return;

	memset(&phandle->cur_item, 0, sizeof(phandle->cur_item));
	phandle->cur_path[0] = '\0';
	phandle->cur_root[0] = '\0';
	phandle->music_name[0] = '\0';
	phandle->disp_name[0] = '\0';
	phandle->cur_play_id = 0;
	phandle->cur_folder_id = 0;
	phandle->cur_gord = 0;
	phandle->pcur_play_item = NULL;

	if (track_id <= 0)
		return;
	if (!divoom_database_tf_track_path(track_id, phandle->cur_path, (int)sizeof(phandle->cur_path)))
		return;

	phandle->cur_play_id = track_id;
	phandle->cur_item.id = track_id;
	phandle->cur_item.like = 0;
	phandle->cur_item.music_name = phandle->cur_path;
	{
		const char *slash = strchr(phandle->cur_path, '/');
		int n = slash ? (int)(slash - phandle->cur_path) : 0;

		if (n > 0 && n < (int)sizeof(phandle->cur_root)) {
			memcpy(phandle->cur_root, phandle->cur_path, (size_t)n);
			phandle->cur_root[n] = '\0';
			phandle->cur_item.root_name = phandle->cur_root;
		}
	}
	if (divoom_database_tf_folder_info(0, &finfo))
		(void)finfo;
	{
		int row = divoom_database_tf_track_row(0, track_id);

		if (row >= 0)
			phandle->cur_gord = row;
	}
	phandle->pcur_play_item = &phandle->cur_item;
#if DIVOOM_SHOW_TF_NAND
	{
		const char *rel = phandle->cur_path;

		while (*rel == '/' || *rel == '\\')
			rel++;
		snprintf(phandle->music_name, sizeof(phandle->music_name),
			 "file://%s/%s", DIVOOM_TF_MUSIC_NAND_DIR, rel);
	}
#else
	snprintf(phandle->music_name, sizeof(phandle->music_name), "file://%s", phandle->cur_path);
#endif
	base = divoom_tf_music_get_disp_name(phandle->cur_path);
	if (base && base[0]) {
		strncpy(phandle->disp_name, base, sizeof(phandle->disp_name) - 1);
		phandle->disp_name[sizeof(phandle->disp_name) - 1] = '\0';
	}
}

static void divoom_tf_music_set_cur_play_item(Pdivoom_tf_music_ctrl phandle, Pdivoom_tf_music_item pitem)
{
	if (phandle == NULL)
		return;
	if (pitem == NULL)
		divoom_tf_music_refresh_cur_from_id(phandle, 0);
	else
		divoom_tf_music_refresh_cur_from_id(phandle, pitem->id);
}

static void divoom_tf_music_start_play_saved(void);
int divoom_tf_music_play_get_next(char *music_name, int status);
static Pdivoom_tf_music_item divoom_tf_music_resolve_item_by_id(int music_id);

#ifdef DIVOOM_SIMULATOR
#include <dirent.h>
#include <strings.h>

#define DIVOOM_TF_SKIP_RING 16
static int s_tf_skip_ids[DIVOOM_TF_SKIP_RING];
static uint8 s_tf_skip_n;
static uint8 s_tf_skip_i;

static int divoom_tf_music_skip_has(int track_id)
{
	int i;

	for (i = 0; i < (int)s_tf_skip_n && i < DIVOOM_TF_SKIP_RING; i++) {
		if (s_tf_skip_ids[i] == track_id)
			return 1;
	}
	return 0;
}

static void divoom_tf_music_skip_add(int track_id)
{
	if (track_id <= 0)
		return;
	if (divoom_tf_music_skip_has(track_id))
		return;
	s_tf_skip_ids[s_tf_skip_i % DIVOOM_TF_SKIP_RING] = track_id;
	s_tf_skip_i++;
	if (s_tf_skip_n < DIVOOM_TF_SKIP_RING)
		s_tf_skip_n++;
}

static void divoom_tf_music_sim_play_file_for_id(int music_id)
{
    char play_url[280];
    int vol;

    if (pdivoom_tf_music_handle == NULL)
        return;
    divoom_tf_music_refresh_cur_from_id(pdivoom_tf_music_handle, music_id);
    if (pdivoom_tf_music_handle->cur_path[0] == '\0')
    {
        divoom_print("[sim_tf] skip music_id=%d (no track)\n", music_id);
        return;
    }

    vol = (int)divoom_tf_music_get_volume();
    if (vol < 0)
        vol = 0;
    if (vol > 100)
        vol = 100;

    snprintf(play_url, sizeof(play_url), "file://%s", pdivoom_tf_music_handle->cur_path);
    divoom_print("[sim_tf] music_id=%d vol=%d path=%s\n", music_id, vol, pdivoom_tf_music_handle->cur_path);
    divoom_local_music_ctrl_stop_tone();
    divoom_local_music_ctrl_music_play(play_url, (uint8)vol, divoom_tf_music_play_get_next);
    if (divoom_music_ctrl_get_mode() == DIVOOM_MUSIC_CTRL_MODE_LOCAL)
        divoom_music_ctrl_set_playing_flag(1);
}
#endif /* DIVOOM_SIMULATOR */

int divoom_tf_music_compare_music_id(void *item1, void *item2);

void divoom_tf_music_start_free_music_item(void *item)
{
    Pdivoom_tf_music_item pitem = (Pdivoom_tf_music_item)item;

    if (pitem == NULL)
        return;
    divoom_system_free(pitem->music_name);
    divoom_system_free(pitem->root_name);
    divoom_system_free(pitem);
}

void divoom_tf_music_start_free_like_music_item(void *item)
{
#if 0
	Pdivoom_tf_music_item pitem = (Pdivoom_tf_music_item)item;

    if (pitem == NULL)
        return;
    divoom_system_free(pitem->music_name);
    divoom_system_free(pitem->root_name);
    divoom_system_free(pitem);
#endif
}

#ifdef DIVOOM_TIMES_VIBE
static void divoom_tf_music_mark_play_started(void);
#endif
void divoom_tf_music_send_play_music_id(int music_id)
{
	int wire;
	int id_base;

#if DIVOOM_SHOW_TF_NAND
	if (music_id <= 0)
		return;
	divoom_tf_music_refresh_cur_from_id(pdivoom_tf_music_handle, music_id);
	if (!pdivoom_tf_music_handle || pdivoom_tf_music_handle->music_name[0] == '\0')
		return;
#ifdef DIVOOM_TIMES_VIBE
	divoom_tf_music_mark_play_started();
#endif
	divoom_local_music_ctrl_stop_music();
	divoom_local_music_ctrl_music_play(pdivoom_tf_music_handle->music_name,
					   divoom_tf_music_get_volume(),
					   divoom_tf_music_play_get_next);
	if (divoom_music_ctrl_get_mode() == DIVOOM_MUSIC_CTRL_MODE_LOCAL)
		divoom_music_ctrl_set_playing_flag(1);
	return;
#endif

#ifdef DIVOOM_SIMULATOR
    if (music_id <= 0)
        return;
    divoom_tf_music_sim_play_file_for_id(music_id);
    return;
#endif
#ifdef DIVOOM_TIMES_VIBE
	uint8 cmd_data[16];

	if (music_id <= 0)
		return;

	id_base = 1;
	if (pdivoom_tf_music_handle && pdivoom_tf_music_handle->id_base >= 0)
		id_base = pdivoom_tf_music_handle->id_base;
	wire = music_id - 1 + id_base;
	if (wire < 0)
		wire = 0;

	if (!divoom_music_ctrl_tf_jl_ready_for_play())
	{
		divoom_print("tf music: defer db_id %d wire %d (wait jl TF/0x5e)\n", music_id, wire);
		return;
	}

	cmd_data[0] = 1;
	cmd_data[1] = wire & 0xff;
	cmd_data[2] = (wire >> 8) & 0xff;

	divoom_tf_music_mark_play_started();
	divoom_btcore_uart_cmd_send(DM_BTCORE_SD_MUSIC_PLAYED_ID, cmd_data, 3);
	divoom_music_ctrl_jl_tf_mark_ready();
#endif
}

uint32 divoom_tf_music_get_music_name_checksum(uint32 checksum, char *music_name)
{
	int len;

	len = strlen(music_name);

	for (int j = 0; j < len; j++)
	{
        checksum ^= (uint32_t)(unsigned char)music_name[j];
    }

	return checksum;
}

uint32 divoom_tf_music_get_all_music_name_checksum(void)
{
	uint32 checksum = 0u;

	(void)divoom_database_tf_meta_get_u32("checksum", &checksum);
	return checksum;
}



#ifdef DIVOOM_TIMES_VIBE
/* TIMES_VIBE：TF 挂载根为 /。/a.mp3 无子目录；/MUSIC/a.mp3 根目录为 MUSIC */
static int divoom_tf_music_parse_root_name(const char *music_name, char *root_buf, int root_buf_size)
{
	const char *p;
	const char *seg;
	int len;

	if (music_name == NULL || root_buf == NULL || root_buf_size <= 0)
		return 0;

	root_buf[0] = '\0';
	p = music_name;
	while (*p == '/')
		p++;
	if (*p == '\0')
		return 0;

	seg = p;
	while (*p && *p != '/')
		p++;
	len = (int)(p - seg);
	if (len <= 0)
		return 0;

	while (*p == '/')
		p++;
	if (*p == '\0')
		return 0;

	if (len >= root_buf_size)
		len = root_buf_size - 1;
	memcpy(root_buf, seg, (size_t)len);
	root_buf[len] = '\0';
	return 1;
}
#else
/* Path A：从路径中定位 music 后的第一级目录；文件直接在 music 下则不算根目录 */
#define DIVOOM_TF_MUSIC_DIR_NAME    "music"
static int divoom_tf_music_parse_root_name(const char *music_name, char *root_buf, int root_buf_size)
{
	const char *p = music_name;
	const char *seg_start;
	int len;

	if (music_name == NULL || root_buf == NULL || root_buf_size <= 0)
		return 0;

	root_buf[0] = '\0';
	while (*p)
	{
		while (*p == '/')
			p++;
		if (!*p)
			break;
		seg_start = p;
		while (*p && *p != '/')
			p++;
		len = (int)(p - seg_start);
		if (len == (int)strlen(DIVOOM_TF_MUSIC_DIR_NAME) &&
			strncmp(seg_start, DIVOOM_TF_MUSIC_DIR_NAME, (size_t)len) == 0)
		{
			while (*p == '/')
				p++;
			if (!*p)
				return 0;
			seg_start = p;
			while (*p && *p != '/')
				p++;
			if (*p != '/')
				return 0;
			len = (int)(p - seg_start);
			if (len >= root_buf_size)
				len = root_buf_size - 1;
			memcpy(root_buf, seg_start, (size_t)len);
			root_buf[len] = '\0';
			return 1;
		}
	}
	return 0;
}
#endif

/* 当前歌单同步来源，供 begin/store/commit 日志关联 */
static const char *s_tf_sync_reason = "unset";

void divoom_tf_music_sync_set_reason(const char *reason)
{
	s_tf_sync_reason = (reason && reason[0]) ? reason : "unset";
}

void divoom_tf_music_sync_free_tmp_items(void)
{
}

void divoom_tf_music_sync_begin(const uint8 *cmd_data, int cmd_len)
{
	(void)cmd_data;
	(void)cmd_len;
	divoom_print("tf music uart sync ignored (use songlist.bin) reason=%s\n", s_tf_sync_reason);
}

int divoom_tf_music_sync_store_item(const uint8 *cmd_data, int cmd_len)
{
	(void)cmd_data;
	(void)cmd_len;
	return 0;
}

int divoom_tf_music_sync_is_complete(void)
{
	return 0;
}

#ifdef DIVOOM_TIMES_VIBE
void divoom_tf_music_request_ui_hotplug_reload(void);
#endif

void divoom_tf_music_free_music_info_list(Pdivoom_tf_music_item list, int count)
{
	int i;

	if (list == NULL || count <= 0)
		return;

	for (i = 0; i < count; i++)
	{
		if (list[i].music_name != NULL)
			divoom_system_free(list[i].music_name);
		if (list[i].root_name != NULL)
			divoom_system_free(list[i].root_name);
	}
	divoom_system_free(list);
}

void divoom_tf_music_sync_commit(void)
{
}

#ifdef DIVOOM_TIMES_VIBE
#define DIVOOM_TF_MUSIC_SONGLIST_PATH  DIVOOM_SD_SYSTEM_DIR"/tf_songlist.bin"
#define DIVOOM_TF_MUSIC_SONGLIST_STAMP_PATH  DIVOOM_SD_SYSTEM_DIR"/tf_songlist.stamp"
/* 歌单文件头 128 字节：0x00 uint32 小端总曲数，0x04 uint32 小端时间戳，0x08~0x7F 填 0；其后为记录区 */
#define DIVOOM_TF_MUSIC_SONGLIST_HEADER_SIZE  128

static void divoom_tf_music_send_songlist_request(void);
static void divoom_tf_music_send_scan_music_list_internal(void);
static int divoom_tf_music_ctrl_init_if_needed(void);
static void divoom_tf_music_on_songlist_file_saved(const char *path, int ok);
static void divoom_tf_music_try_play_saved_if_jl_ready(void);
#ifdef DIVOOM_SIMULATOR
static void divoom_tf_music_sim_scan_start_async(void);
static void divoom_tf_music_sim_poll_scan_done(void);
static void divoom_tf_music_sim_boot_scan_once(void);
int divoom_tf_music_sim_is_scanning(void);
#endif

static int s_songlist_refresh_wait;
/* UI 点刷新：结束后恢复播放；不改变「有变化才入库」 */
static int s_tf_user_scan_replay;
static int s_tf_refresh_got_file;
/* 歌单文件刚载入：下一帧 set_music_id 即使曲目 id 未变也重发 0x0e（刷新后 JL 可能已停播） */
static int s_tf_songlist_force_replay;
/* 0x5e 当时库空/无有效曲：歌单落地后补一次。插卡正常路径仍只走 0x5e/defer，避免播两次 */
static int s_tf_need_catchup_play;
/* JL 连续出错结束则停止自动切歌，避免整表连跳 */
#define DIVOOM_TF_PLAY_MAX_FAST_FAIL      3u
#define DIVOOM_TF_PLAY_NEXT_MIN_GAP_MS    500u
static uint32_t s_tf_play_start_ms;
static uint32_t s_tf_play_last_next_ms;
static uint8 s_tf_play_fast_fail_count;
static uint8 s_tf_play_skip_blocked;
/* 1=用户上一首触发的失败连跳，往前找能播的；正常听完/点播/下一首都往后 */
static uint8 s_tf_fail_seek_prev;

static void divoom_tf_music_reset_play_fail_guard(void)
{
	s_tf_play_start_ms = 0;
	s_tf_play_last_next_ms = 0;
	s_tf_play_fast_fail_count = 0;
	s_tf_play_skip_blocked = 0;
	s_tf_fail_seek_prev = 0;
}

static void divoom_tf_music_mark_play_started(void)
{
	s_tf_play_start_ms = divoom_system_get_tickms();
}
static uint32_t s_songlist_refresh_start_ms;
static int s_songlist_transfer_active;
static uint8 s_tf_wait_jl_songlist;
static uint32_t s_tf_hotplug_insert_ms;
static uint32_t s_tf_jl_5e_defer_ms;
static uint8 s_tf_play_via_jl_5e_pending = 0;

#define DIVOOM_TF_HOTPLUG_SONGLIST_PLAY_GRACE_MS  300u
static divoom_tf_music_songlist_refresh_cb s_songlist_refresh_cb;
static void *s_songlist_refresh_user;

/* 点刷新后卡死才拆窗：从按下起算，传输中也计时。不当完成、不续播 */
#define DIVOOM_TF_MUSIC_SONGLIST_REFRESH_WATCHDOG_MS  (5u * 60u * 1000u)

static uint32_t divoom_tf_music_refresh_now_ms(void)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);
	return (uint32_t)((tv.tv_sec * 1000UL) + (tv.tv_usec / 1000));
}

static void divoom_tf_music_finish_songlist_refresh(int ok)
{
	if (!s_songlist_refresh_wait)
		return;

	s_songlist_refresh_wait = 0;
	s_songlist_refresh_start_ms = 0;
	s_songlist_transfer_active = 0;
	s_tf_refresh_got_file = 0;
	if (s_songlist_refresh_cb)
		s_songlist_refresh_cb(s_songlist_refresh_user,
				      ok ? DIVOOM_TF_MUSIC_SONGLIST_REFRESH_DONE :
					   DIVOOM_TF_MUSIC_SONGLIST_REFRESH_FAIL);
}

void divoom_tf_music_set_songlist_refresh_callback(divoom_tf_music_songlist_refresh_cb cb,
						   void *user)
{
	s_songlist_refresh_cb = cb;
	s_songlist_refresh_user = user;
}

void divoom_tf_music_on_songlist_info_replied(void)
{
	/* 刷新弹窗只等文件落地，不再用 0x42 空闲当完成 */
}

void divoom_tf_music_on_songlist_transfer_begin(void)
{
	if (!s_songlist_refresh_wait)
		return;

	s_songlist_transfer_active = 1;
}

void divoom_tf_music_on_scan_music_list_status(uint8_t status)
{
	if (status == DIVOOM_BTCORE_SCAN_MUSIC_LIST_STAT_START)
	{
		if (!s_songlist_refresh_wait)
		{
			divoom_tf_music_sync_prepare();
			s_songlist_refresh_wait = 1;
			s_songlist_refresh_start_ms = divoom_tf_music_refresh_now_ms();
			s_songlist_transfer_active = 0;
		}
		return;
	}

	if (status == DIVOOM_BTCORE_SCAN_MUSIC_LIST_STAT_END)
	{
		if (s_songlist_refresh_wait && !s_tf_user_scan_replay
		    && !s_songlist_transfer_active)
			divoom_tf_music_finish_songlist_refresh(1);
	}
}

/* 点刷新：文件已处理（写库或确认相同）后关弹窗并续播 */
static void divoom_tf_music_user_refresh_complete_after_file(void)
{
	if (!s_songlist_refresh_wait)
		return;

	divoom_tf_music_finish_songlist_refresh(1);
	if (s_tf_user_scan_replay
	    && divoom_music_ctrl_get_mode() == DIVOOM_MUSIC_CTRL_MODE_LOCAL)
		divoom_tf_music_play_saved_force_replay();
	s_tf_user_scan_replay = 0;
}

/* 5 分钟卡死：只拆窗，不当刷新成功 */
static void divoom_tf_music_user_refresh_abort(void)
{
	if (!s_songlist_refresh_wait)
		return;

	s_tf_user_scan_replay = 0;
	divoom_tf_music_finish_songlist_refresh(0);
}

void divoom_tf_music_poll_songlist_refresh(void)
{
#ifdef DIVOOM_SIMULATOR
	divoom_tf_music_sim_poll_scan_done();
#endif

	if (!s_songlist_refresh_wait)
		return;

	{
		uint32_t now;

		now = divoom_tf_music_refresh_now_ms();

		if (s_tf_refresh_got_file)
		{
			divoom_tf_music_user_refresh_complete_after_file();
			return;
		}

		if (s_songlist_refresh_start_ms != 0u &&
		    now - s_songlist_refresh_start_ms >= DIVOOM_TF_MUSIC_SONGLIST_REFRESH_WATCHDOG_MS)
			divoom_tf_music_user_refresh_abort();
	}
}

int divoom_tf_music_is_songlist_refreshing(void)
{
#ifdef DIVOOM_SIMULATOR
	if (divoom_tf_music_sim_is_scanning())
		return 1;
#endif
	return s_songlist_refresh_wait ? 1 : 0;
}

static int divoom_tf_music_get_songlist_file_total(const uint8_t *buf, int len)
{
	uint32_t total;

	if (!buf || len < 4)
		return 0;

	total = (uint32_t)btcore_get_le32(buf);
	if (total == 0u || total > 65535u)
		return 0;
	return (int)total;
}

static int divoom_tf_music_read_songlist_file_header(const char *path, int *total, uint32_t *timestamp)
{
	FILE *fp;
	uint8_t hdr[8];

	if (total)
		*total = 0;
	if (timestamp)
		*timestamp = 0u;
	if (!path || !path[0])
		return 0;

	fp = divoom_port_file_open(path, "rb");
	if (!fp)
		return 0;
	if (divoom_port_file_read(hdr, 1, sizeof(hdr), fp) < sizeof(hdr))
	{
		divoom_port_file_close(fp);
		return 0;
	}
	divoom_port_file_close(fp);

	if (total)
		*total = divoom_tf_music_get_songlist_file_total(hdr, (int)sizeof(hdr));
	if (timestamp)
		*timestamp = (uint32_t)btcore_get_le32(hdr + 4);
	return 1;
}

static int divoom_tf_music_read_applied_songlist_stamp(uint32_t *timestamp)
{
	uint32 stamp = 0u;

	if (!timestamp)
		return 0;
	if (!divoom_database_tf_meta_get_u32("songlist_ts", &stamp) || stamp == 0u)
		return 0;
	*timestamp = stamp;
	return 1;
}

static int divoom_tf_music_songlist_cache_hit(const char **miss_reason)
{
	int file_total = 0;
	int db_total = 0;
	uint32_t file_ts = 0u;
	uint32_t applied_ts = 0u;

	if (miss_reason)
		*miss_reason = NULL;

	if (!divoom_tf_music_read_songlist_file_header(DIVOOM_TF_MUSIC_SONGLIST_PATH,
			&file_total, &file_ts))
	{
		if (miss_reason)
			*miss_reason = "songlist file missing or bad header";
		return 0;
	}
	if (file_total <= 0 || file_ts == 0u)
	{
		if (miss_reason)
			*miss_reason = "songlist file total/ts invalid";
		return 0;
	}
	if (!divoom_tf_music_read_applied_songlist_stamp(&applied_ts))
	{
		if (miss_reason)
			*miss_reason = "applied stamp missing";
		return 0;
	}
	if (applied_ts != file_ts)
	{
		if (miss_reason)
			*miss_reason = "stamp mismatch (file updated, need reload)";
		divoom_print("tf music cache miss: file_ts=%u applied_ts=%u\n",
			(unsigned)file_ts, (unsigned)applied_ts);
		return 0;
	}

	db_total = divoom_database_tf_total();
	if (db_total != file_total)
	{
		if (miss_reason)
			*miss_reason = "tf_track count mismatch";
		divoom_print("tf music cache miss: db=%d file_total=%d ts=%u\n",
			db_total, file_total, (unsigned)file_ts);
		return 0;
	}

	if (pdivoom_tf_music_handle)
		pdivoom_tf_music_handle->total = db_total;
	divoom_print("tf music cache hit: total=%d ts=%u (skip parse/store)\n", file_total, file_ts);
	return 1;
}

#define DIVOOM_TF_FNV_OFF 2166136261u
#define DIVOOM_TF_FNV_PRIME 16777619u
#define DIVOOM_TF_SONGLIST_CHUNK 4096
static uint32 divoom_tf_music_fnv1a_update(uint32 h, const uint8 *p, int n)
{
	int i;

	for (i = 0; i < n; i++) {
		h ^= (uint32)p[i];
		h *= DIVOOM_TF_FNV_PRIME;
	}
	return h;
}

static int divoom_tf_music_songlist_next_record(FILE *fp, int *jl_id, char *name, int name_cap)
{
	uint8 hdr[4];
	int name_len;

	if (!fp || !jl_id || !name || name_cap <= 1)
		return 0;
	if (divoom_port_file_read(hdr, 1, 4, fp) != 4)
		return 0;
	*jl_id = (int)hdr[0] | ((int)hdr[1] << 8);
	name_len = (int)hdr[2] | ((int)hdr[3] << 8);
	if (name_len <= 0)
		return 0;
	if (name_len >= name_cap)
		name_len = name_cap - 1;
	if (divoom_port_file_read(name, 1, (size_t)name_len, fp) != (size_t)name_len)
		return 0;
	name[name_len] = '\0';
	return 1;
}

/* dst_sz 含 '\0'。src_len 为源字节数，可大于 dst。 */
static void divoom_tf_music_utf8_cap(char *dst, size_t dst_sz, const char *src, size_t src_len)
{
	size_t n;
	size_t n0;

	if (!dst || dst_sz == 0)
		return;
	if (!src || src_len == 0) {
		dst[0] = '\0';
		return;
	}
	n0 = src_len;
	if (n0 >= dst_sz)
		n0 = dst_sz - 1;
	n = n0;
	while (n > 0 && ((unsigned char)src[n - 1] & 0xC0) == 0x80)
		n--;
	if (n > 0 && ((unsigned char)src[n - 1] & 0x80)) {
		unsigned char c = (unsigned char)src[n - 1];
		size_t need = 1;

		if ((c & 0xE0) == 0xC0)
			need = 2;
		else if ((c & 0xF0) == 0xE0)
			need = 3;
		else if ((c & 0xF8) == 0xF0)
			need = 4;
		if (n - 1 + need > n0)
			n--;
	}
	memcpy(dst, src, n);
	dst[n] = '\0';
}

/* 超长歌名：保留前面 + 后缀（含 '.'），中间丢掉。无合理后缀则只截前面。 */
#define DIVOOM_TF_MUSIC_EXT_MAX  8
static void divoom_tf_music_utf8_cap_keep_ext(char *dst, size_t dst_sz, const char *src, size_t src_len)
{
	const char *dot;
	size_t i;
	size_t ext_len;
	size_t prefix_room;
	size_t n;

	if (!dst || dst_sz == 0)
		return;
	if (!src || src_len == 0) {
		dst[0] = '\0';
		return;
	}
	if (src_len < dst_sz) {
		memcpy(dst, src, src_len);
		dst[src_len] = '\0';
		return;
	}
	dot = NULL;
	for (i = src_len; i > 0; i--) {
		if (src[i - 1] == '.') {
			dot = src + (i - 1);
			break;
		}
	}
	ext_len = dot ? (size_t)(src + src_len - dot) : 0;
	if (!dot || dot == src || ext_len < 2 || ext_len > DIVOOM_TF_MUSIC_EXT_MAX) {
		divoom_tf_music_utf8_cap(dst, dst_sz, src, src_len);
		return;
	}
	prefix_room = dst_sz - 1 - ext_len;
	divoom_tf_music_utf8_cap(dst, prefix_room + 1, src, (size_t)(dot - src));
	n = strlen(dst);
	memcpy(dst + n, dot, ext_len);
	dst[n + ext_len] = '\0';
}

static int divoom_tf_music_apply_track_path(int db_id, const char *path)
{
	int parent = 0;
	int depth = 0;
	int folder_id;
	char accum[DIVOOM_TF_PATH_MAX];
	char seg[DIVOOM_TF_NAME_MAX];
	const char *p;
	const char *start;
	size_t seglen;
	int hit_sep;

	if (!path || !path[0] || db_id <= 0)
		return 0;
	p = path;
	while (*p == '/' || *p == '\\')
		p++;
	accum[0] = '\0';
	while (*p) {
		start = p;
		while (*p && *p != '/' && *p != '\\')
			p++;
		seglen = (size_t)(p - start);
		hit_sep = (*p == '/' || *p == '\\') ? 1 : 0;
		while (*p == '/' || *p == '\\')
			p++;
		if (seglen == 0)
			continue;
		if (hit_sep) {
			size_t used;

			divoom_tf_music_utf8_cap(seg, sizeof(seg), start, seglen);
			if (!seg[0])
				continue;
			used = strlen(accum);
			if (used + strlen(seg) + 2u >= sizeof(accum))
				return 0;
			snprintf(accum + used, sizeof(accum) - used, "%s/", seg);
			if (!divoom_database_tf_ensure_folder(parent, seg, accum, depth + 1, &folder_id))
				return 0;
			parent = folder_id;
			depth++;
		} else {
			divoom_tf_music_utf8_cap_keep_ext(seg, sizeof(seg), start, seglen);
			if (!seg[0])
				return 0;
			return divoom_database_tf_add_track(db_id, parent, seg);
		}
	}
	return 0;
}

#if DIVOOM_SHOW_TF_NAND
#define DIVOOM_TF_NAND_SCAN_MAX_FILES  4096
#define DIVOOM_TF_NAND_SCAN_MAX_DEPTH  16
#define DIVOOM_SHOW_TF_META_FP         "nand_fp"
#define DIVOOM_SHOW_TF_META_TOTAL      "nand_total"

static void divoom_tf_music_nand_path_join(char *out, int out_sz, const char *dir, const char *name)
{
	if (!out || out_sz <= 0)
		return;
	if (!name || !name[0]) {
		if (dir && dir[0])
			snprintf(out, (size_t)out_sz, "%s", dir);
		else
			out[0] = '\0';
		return;
	}
	if (!dir || !dir[0]) {
		snprintf(out, (size_t)out_sz, "%s", name);
		return;
	}
	if (dir[strlen(dir) - 1] == '/')
		snprintf(out, (size_t)out_sz, "%s%s", dir, name);
	else
		snprintf(out, (size_t)out_sz, "%s/%s", dir, name);
}

static uint32 divoom_tf_music_nand_file_fp(const char *rel, long size, long mtime)
{
	uint32 h = DIVOOM_TF_FNV_OFF;

	if (!rel)
		rel = "";
	h = divoom_tf_music_fnv1a_update(h, (const uint8 *)rel, (int)strlen(rel));
	h = divoom_tf_music_fnv1a_update(h, (const uint8 *)&size, (int)sizeof(size));
	h = divoom_tf_music_fnv1a_update(h, (const uint8 *)&mtime, (int)sizeof(mtime));
	return h;
}

static void divoom_tf_music_nand_scan_dir_recursive(Pdivoom_tf_music_ctrl phandle,
	const char *host_dir, const char *dev_rel, int depth, int *seeded,
	uint32 *fp, int apply)
{
	void *dir;
	const divoom_port_dirent_t *ent;
	char child_host[512];
	char child_dev[DIVOOM_TF_PATH_MAX];
	const char *ext;

	(void)phandle;
	if (!host_dir || !dev_rel || !seeded || *seeded >= DIVOOM_TF_NAND_SCAN_MAX_FILES)
		return;
	if (depth > DIVOOM_TF_NAND_SCAN_MAX_DEPTH)
		return;
	dir = divoom_port_dir_open(host_dir);
	if (!dir) {
		if (depth == 0)
			divoom_print("[show_tf] opendir %s failed\n", host_dir);
		return;
	}
	while ((ent = divoom_port_dir_read(dir)) != NULL) {
		if (!ent->name || ent->name[0] == '.')
			continue;
		if ((int)strlen(ent->name) >= (int)sizeof(child_host) - 32)
			continue;
		divoom_tf_music_nand_path_join(child_host, (int)sizeof(child_host), host_dir, ent->name);
		divoom_tf_music_nand_path_join(child_dev, (int)sizeof(child_dev), dev_rel, ent->name);
		if (ent->is_dir) {
			divoom_tf_music_nand_scan_dir_recursive(phandle, child_host, child_dev,
				depth + 1, seeded, fp, apply);
			continue;
		}
		ext = strrchr(ent->name, '.');
		if (ext != NULL && strcasecmp(ext, ".mp3") == 0) {
			const char *rel = child_dev[0] ? child_dev : ent->name;

			if (fp) {
				divoom_port_stat_t st;

				memset(&st, 0, sizeof(st));
				if (divoom_port_stat(child_host, &st) != 0) {
					st.size = 0;
					st.mtime = 0;
				}
				*fp ^= divoom_tf_music_nand_file_fp(rel, st.size, st.mtime);
			}
			if (!apply) {
				(*seeded)++;
				if (*seeded >= DIVOOM_TF_NAND_SCAN_MAX_FILES)
					break;
				continue;
			}
			if (divoom_tf_music_apply_track_path(*seeded + 1, rel))
				(*seeded)++;
			if (*seeded >= DIVOOM_TF_NAND_SCAN_MAX_FILES)
				break;
		}
	}
	divoom_port_dir_close(dir);
}

int divoom_tf_music_scan_root_dir(const char *root)
{
	Pdivoom_tf_music_ctrl phandle;
	int seen = 0;
	int seeded = 0;
	uint32 fp = 0;
	uint32 old_fp = 0;
	uint32 old_total = 0;
	int db_total;
	const char *scan_root = root ? root : DIVOOM_TF_MUSIC_NAND_DIR;

	divoom_app_com_mk_dir(scan_root);
	if (!divoom_tf_music_ctrl_init_if_needed())
		return 0;
	phandle = pdivoom_tf_music_handle;
	if (!phandle)
		return 0;
	divoom_tf_music_nand_scan_dir_recursive(phandle, scan_root, "", 0, &seen, &fp, 0);
	db_total = divoom_database_tf_total();
	if (divoom_database_tf_meta_get_u32(DIVOOM_SHOW_TF_META_FP, &old_fp)
	    && divoom_database_tf_meta_get_u32(DIVOOM_SHOW_TF_META_TOTAL, &old_total)
	    && old_fp == fp && old_total == (uint32)seen && db_total == seen) {
		phandle->total = db_total;
		phandle->id_base = 1;
		divoom_print("[show_tf] scan skip %s fp=%u tracks=%d\n",
			     scan_root, (unsigned)fp, seen);
		return seen;
	}
	divoom_database_tf_rebuild_begin();
	divoom_tf_music_nand_scan_dir_recursive(phandle, scan_root, "", 0, &seeded, NULL, 1);
	divoom_database_tf_rebuild_end();
	divoom_database_tf_meta_set_u32(DIVOOM_SHOW_TF_META_FP, fp);
	divoom_database_tf_meta_set_u32(DIVOOM_SHOW_TF_META_TOTAL, (uint32)seen);
	phandle->total = divoom_database_tf_total();
	phandle->id_base = 1;
	divoom_print("[show_tf] scan %s tracks=%d total=%d fp=%u\n",
		     scan_root, seeded, phandle->total, (unsigned)fp);
	return seeded;
}

#define DIVOOM_SHOW_TF_NET_DIR     "AstroToo/showmode/tf_music/"
#define DIVOOM_SHOW_TF_CRC_NAME    "tf_music.crc"
#define DIVOOM_SHOW_TF_FETCH_MAX   16

typedef struct _divoom_show_tf_fetch_item
{
	char name[96];
	uint32 crc;
	uint64_t size;
} divoom_show_tf_fetch_item;

static volatile int s_show_tf_fetch_running;
static volatile int s_show_tf_fetch_cancel;

static int divoom_show_tf_hex4(char c, uint32 *digit)
{
	if (c >= '0' && c <= '9')
	{
		*digit = (uint32)(c - '0');
		return 1;
	}
	if (c >= 'a' && c <= 'f')
	{
		*digit = (uint32)(c - 'a' + 10);
		return 1;
	}
	if (c >= 'A' && c <= 'F')
	{
		*digit = (uint32)(c - 'A' + 10);
		return 1;
	}
	return 0;
}

static int divoom_show_tf_encode_name(const char *name, char *out, int out_sz)
{
	int n;
	const char *dir = DIVOOM_SHOW_TF_NET_DIR;

	n = snprintf(out, (size_t)out_sz, "%s", dir);
	if (n <= 0 || n >= out_sz)
		return 0;
	for (; *name; name++)
	{
		unsigned char c = (unsigned char)*name;

		if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
		    (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-')
		{
			if (n + 1 >= out_sz)
				return 0;
			out[n++] = (char)c;
		}
		else
		{
			if (n + 3 >= out_sz)
				return 0;
			n += snprintf(out + n, (size_t)(out_sz - n), "%%%02X", c);
		}
	}
	out[n] = '\0';
	return 1;
}

static int divoom_show_tf_name_ok(const char *name)
{
	const char *p;

	if (!name || !name[0] || strlen(name) >= 96)
		return 0;
	for (p = name; *p; p++)
	{
		if (*p == '/' || *p == '\\' || *p == '\r' || *p == '\n')
			return 0;
	}
	return 1;
}

static int divoom_show_tf_parse_crc(char *text, divoom_show_tf_fetch_item *items, int *count)
{
	char *p = text;

	*count = 0;
	while (p && *p)
	{
		char *line = p;
		char *nl = strchr(p, '\n');
		char *name;
		uint32 crc = 0;
		uint64_t size = 0;
		int i;

		if (nl)
		{
			*nl = '\0';
			p = nl + 1;
		}
		else
		{
			p += strlen(p);
		}
		if (line[0] == '\0' || line[0] == '#')
			continue;
		if (line[0] && line[strlen(line) - 1] == '\r')
			line[strlen(line) - 1] = '\0';
		for (i = 0; i < 8; i++)
		{
			uint32 digit;

			if (!divoom_show_tf_hex4(line[i], &digit))
				return 0;
			crc = (crc << 4) | digit;
		}
		if (line[8] != ' ')
			return 0;
		name = line + 9;
		while (*name >= '0' && *name <= '9')
		{
			size = size * 10u + (uint64_t)(*name - '0');
			name++;
		}
		if (name == line + 9 || *name != ' ')
			return 0;
		name++;
		if (!divoom_show_tf_name_ok(name) || *count >= DIVOOM_SHOW_TF_FETCH_MAX)
			return 0;
		items[*count].crc = crc;
		items[*count].size = size;
		strncpy(items[*count].name, name, sizeof(items[*count].name) - 1);
		items[*count].name[sizeof(items[*count].name) - 1] = '\0';
		(*count)++;
	}
	return *count > 0;
}

static int divoom_show_tf_file_matches(const char *path, uint64_t size, uint32 crc)
{
	uint64_t actual_size = 0;
	uint32 actual_crc = 0;

	if (divoom_port_file_crc32(path, &actual_size, &actual_crc) != 0)
		return 0;
	return actual_size == size && actual_crc == crc;
}

static int divoom_show_tf_download_one(const char *remote, const char *temp,
	const char *final_path, uint64_t size, uint32 crc)
{
	if (divoom_show_tf_file_matches(final_path, size, crc))
		return 1;
	divoom_port_remove_file(temp);
	if (divoom_http_client_down_file((char *)temp, (char *)remote, 1, NULL) != 1)
	{
		divoom_port_remove_file(temp);
		return 0;
	}
	if (!divoom_show_tf_file_matches(temp, size, crc))
	{
		divoom_print("[show_tf] crc mismatch %s\n", final_path);
		divoom_port_remove_file(temp);
		return 0;
	}
	divoom_port_remove_file(final_path);
	if (divoom_port_rename_file(temp, final_path) != 0)
	{
		divoom_port_remove_file(temp);
		return 0;
	}
	return 1;
}

static void *divoom_show_tf_fetch_thread(void *param)
{
	char *text = NULL;
	int text_len = 0;
	int count = 0;
	int i;
	int ok = 0;
	char remote[320];
	char crc_final[160];
	char crc_temp[180];
	divoom_show_tf_fetch_item items[DIVOOM_SHOW_TF_FETCH_MAX];

	(void)param;
	snprintf(crc_final, sizeof(crc_final), "%s/%s",
		DIVOOM_TF_MUSIC_NAND_DIR, DIVOOM_SHOW_TF_CRC_NAME);
	snprintf(crc_temp, sizeof(crc_temp), "%s.divoom_new", crc_final);
	divoom_app_com_mk_dir(DIVOOM_TF_MUSIC_NAND_DIR);
	if (divoom_port_file_exists(crc_final))
	{
		ok = 1;
		goto done;
	}
	if (!divoom_show_tf_encode_name(DIVOOM_SHOW_TF_CRC_NAME, remote, (int)sizeof(remote)))
		goto done;
	divoom_print("[show_tf] fetch crc %s\n", remote);
	divoom_port_remove_file(crc_temp);
	if (s_show_tf_fetch_cancel ||
	    divoom_http_client_down_file(crc_temp, remote, 1, NULL) != 1)
	{
		divoom_port_remove_file(crc_temp);
		goto done;
	}
	if (s_show_tf_fetch_cancel)
	{
		divoom_port_remove_file(crc_temp);
		goto done;
	}
	text = divoom_app_com_read_file(crc_temp, &text_len);
	if (!text || !divoom_show_tf_parse_crc(text, items, &count))
	{
		divoom_print("[show_tf] crc list invalid\n");
		divoom_port_remove_file(crc_temp);
		goto done;
	}
	for (i = 0; i < count; i++)
	{
		char final_path[220];
		char temp_path[240];

		if (s_show_tf_fetch_cancel)
		{
			divoom_port_remove_file(crc_temp);
			goto done;
		}
		snprintf(final_path, sizeof(final_path), "%s/%s",
			DIVOOM_TF_MUSIC_NAND_DIR, items[i].name);
		snprintf(temp_path, sizeof(temp_path), "%s.divoom_new", final_path);
		if (!divoom_show_tf_encode_name(items[i].name, remote, (int)sizeof(remote)) ||
		    !divoom_show_tf_download_one(remote, temp_path, final_path,
			items[i].size, items[i].crc))
		{
			divoom_print("[show_tf] fetch failed %s\n", items[i].name);
			divoom_port_remove_file(crc_temp);
			goto done;
		}
	}
	divoom_port_remove_file(crc_final);
	if (divoom_port_rename_file(crc_temp, crc_final) != 0)
	{
		divoom_port_remove_file(crc_temp);
		goto done;
	}
	divoom_tf_music_scan_root_dir(DIVOOM_TF_MUSIC_NAND_DIR);
	divoom_print("[show_tf] fetch done tracks=%d\n", count);
	ok = 1;
done:
	if (text)
		divoom_system_free(text);
	if (!ok)
		divoom_print("[show_tf] fetch incomplete, crc not committed\n");
	s_show_tf_fetch_running = 0;
	return NULL;
}

void divoom_show_tf_music_fetch_cancel(void)
{
	int i;

	s_show_tf_fetch_cancel = 1;
	for (i = 0; i < 100 && s_show_tf_fetch_running; i++)
		divoom_system_delay_ms(100);
	if (s_show_tf_fetch_running)
		divoom_print("[show_tf] fetch still running at format\n");
}

void divoom_show_tf_music_fetch_if_needed(void)
{
	char crc_final[160];
	void *task;

	if (s_show_tf_fetch_cancel)
		return;
	snprintf(crc_final, sizeof(crc_final), "%s/%s",
		DIVOOM_TF_MUSIC_NAND_DIR, DIVOOM_SHOW_TF_CRC_NAME);
	if (divoom_port_file_exists(crc_final) || s_show_tf_fetch_running)
		return;
	s_show_tf_fetch_running = 1;
	task = divoom_system_create_task(divoom_show_tf_fetch_thread, "show_tf_fetch",
		NULL, DIVOOM_TASK_STACK_24K);
	if (!task)
		s_show_tf_fetch_running = 0;
}
#endif

static int divoom_tf_music_stream_apply_songlist(FILE *fp, int item_count, uint32 file_ts, uint32 fnv, int id_base)
{
	int parsed = 0;
	int slot;
	int jl_id;
	char name[256];

	if (!divoom_database_tf_rebuild_begin())
		return 0;
	if (divoom_port_file_seek(fp, DIVOOM_TF_MUSIC_SONGLIST_HEADER_SIZE, SEEK_SET) != 0) {
		divoom_database_tf_rebuild_end();
		return 0;
	}
	for (slot = 0; slot < item_count; slot++) {
		if (!divoom_tf_music_songlist_next_record(fp, &jl_id, name, (int)sizeof(name)))
			break;
		if (divoom_tf_music_apply_track_path(slot + 1, name))
			parsed++;
	}
	if (!divoom_database_tf_rebuild_end())
		return 0;
	divoom_database_tf_meta_set_u32("checksum", fnv);
	divoom_database_tf_meta_set_u32("total", (uint32)parsed);
	divoom_database_tf_meta_set_u32("songlist_ts", file_ts);
	divoom_database_tf_meta_set_u32("id_base", (uint32)id_base);
	if (pdivoom_tf_music_handle) {
		pdivoom_tf_music_handle->id_base = id_base;
		pdivoom_tf_music_handle->total = parsed;
		(void)divoom_database_tf_meta_get_u32("generation", &pdivoom_tf_music_handle->generation);
	}
	return parsed;
}


static void divoom_tf_music_after_songlist_ready(void)
{
	/* 歌单只换表。热插拔起播只认 0x5e；勿在此发 0x0e，也勿清 5e pending。 */
	s_tf_wait_jl_songlist = 0;
	s_tf_hotplug_insert_ms = 0;
	s_tf_jl_5e_defer_ms = 0;
	s_tf_songlist_force_replay = 1;
}

static void divoom_tf_music_load_songlist_file(const char *path)
{
#if DIVOOM_SHOW_TF_NAND
	(void)path;
#else
	FILE *fp;
	uint8 hdr[8];
	uint8 chunk[DIVOOM_TF_SONGLIST_CHUNK];
	int item_count;
	uint32 file_ts;
	uint32 fnv;
	uint32 old_fnv = 0u;
	uint32 old_ts = 0u;
	uint32 old_base = 0u;
	int id_base = 1;
	int first_id;
	int loaded;
	int n;

	if (!path || !path[0]) {
		divoom_print("tf music load songlist skip: empty path reason=%s\n", s_tf_sync_reason);
		return;
	}
	fp = divoom_port_file_open(path, "rb");
	if (!fp) {
		divoom_print("tf music load songlist open fail path=%s reason=%s\n",
			path, s_tf_sync_reason);
		return;
	}
	if (divoom_port_file_read(hdr, 1, sizeof(hdr), fp) != sizeof(hdr)) {
		divoom_port_file_close(fp);
		return;
	}
	item_count = divoom_tf_music_get_songlist_file_total(hdr, (int)sizeof(hdr));
	file_ts = (uint32)btcore_get_le32(hdr + 4);
	if (item_count <= 0) {
		divoom_port_file_close(fp);
		return;
	}
	if (divoom_port_file_seek(fp, DIVOOM_TF_MUSIC_SONGLIST_HEADER_SIZE, SEEK_SET) != 0) {
		divoom_port_file_close(fp);
		return;
	}

	fnv = DIVOOM_TF_FNV_OFF;
	fnv = divoom_tf_music_fnv1a_update(fnv, hdr, (int)sizeof(hdr));
	first_id = -1;
	while ((n = (int)divoom_port_file_read(chunk, 1, sizeof(chunk), fp)) > 0) {
		if (first_id < 0 && n >= 2)
			first_id = (int)chunk[0] | ((int)chunk[1] << 8);
		fnv = divoom_tf_music_fnv1a_update(fnv, chunk, n);
	}
	id_base = (first_id == 0) ? 0 : 1;

	(void)divoom_database_tf_meta_get_u32("checksum", &old_fnv);
	(void)divoom_database_tf_meta_get_u32("songlist_ts", &old_ts);
	(void)divoom_database_tf_meta_get_u32("id_base", &old_base);
	if (old_fnv == fnv && old_ts == file_ts && (int)old_base == id_base
	    && divoom_database_tf_total() == item_count) {
		divoom_print("tf music load skip same fnv ts=%u total=%d reason=%s\n",
			(unsigned)file_ts, item_count, s_tf_sync_reason);
		if (pdivoom_tf_music_handle) {
			pdivoom_tf_music_handle->id_base = id_base;
			pdivoom_tf_music_handle->total = item_count;
		}
		divoom_port_file_close(fp);
		divoom_tf_music_after_songlist_ready();
		divoom_tf_music_try_play_saved_if_jl_ready();
		return;
	}

	loaded = divoom_tf_music_stream_apply_songlist(fp, item_count, file_ts, fnv, id_base);
	divoom_port_file_close(fp);
	divoom_print("tf music load songlist file %s items=%d reason=%s\n", path, loaded, s_tf_sync_reason);
	if (loaded > 0) {
		divoom_tf_music_after_songlist_ready();
		divoom_tf_music_request_ui_hotplug_reload();
		divoom_tf_music_try_play_saved_if_jl_ready();
	}
#endif
}

static int divoom_tf_music_ctrl_init_if_needed(void)
{
	uint32 id_base = 1u;

	if (pdivoom_tf_music_handle != NULL)
		return 1;

	pdivoom_tf_music_handle = divoom_system_malloc(sizeof(divoom_tf_music_ctrl));
	if (pdivoom_tf_music_handle == NULL)
		return 0;
	memset(pdivoom_tf_music_handle, 0, sizeof(divoom_tf_music_ctrl));
	pdivoom_tf_music_handle->mode = divoom_main_set_tf_music_mode(-1);
	pdivoom_tf_music_handle->play_order = divoom_main_set_tf_music_order(-1);
	pdivoom_tf_music_handle->id_base = 1;
	if (divoom_database_tf_meta_get_u32("id_base", &id_base))
		pdivoom_tf_music_handle->id_base = (int)id_base;
	(void)divoom_database_tf_meta_get_u32("generation", &pdivoom_tf_music_handle->generation);
	pdivoom_tf_music_handle->total = divoom_database_tf_total();
	return 1;
}

void divoom_tf_music_sync_prepare(void)
{
	static int s_songlist_cb_registered = 0;

	if (!s_songlist_cb_registered)
	{
		divoom_btcore_file_rx_set_saved_callback(divoom_tf_music_on_songlist_file_saved);
		s_songlist_cb_registered = 1;
	}
	divoom_tf_music_ctrl_init_if_needed();
}

static void divoom_tf_music_on_songlist_file_saved(const char *path, int ok)
{
#if DIVOOM_SHOW_TF_NAND
	(void)path;
	(void)ok;
	return;
#endif
	if (!ok || !path)
	{
		if (s_songlist_refresh_wait)
		{
			s_tf_user_scan_replay = 0;
			divoom_tf_music_finish_songlist_refresh(0);
		}
		return;
	}

	divoom_tf_music_sync_set_reason("jl_songlist_file_rx");
	divoom_tf_music_sync_prepare();
	divoom_tf_music_load_songlist_file(path);
	if (s_songlist_refresh_wait)
	{
		s_songlist_transfer_active = 0;
		s_tf_refresh_got_file = 1;
		divoom_tf_music_user_refresh_complete_after_file();
	}
}

static void divoom_tf_music_send_songlist_request(void)
{
	const char *path = DIVOOM_TF_MUSIC_SONGLIST_PATH;
	uint16_t crc;

	/* 新协议由 JL 发 0x42 查询，T113 在 on_songlist_reply 中应答；此处仅预置保存路径 */
	crc = divoom_btcore_file_rx_crc16_file(path);
	divoom_btcore_file_rx_set_save_path(path, crc);
}

static void divoom_tf_music_send_scan_music_list_internal(void)
{
	if (!divoom_btcore_uart_cmd_is_ready())
	{
		divoom_tf_music_finish_songlist_refresh(0);
		return;
	}

	divoom_tf_music_sync_prepare();
	divoom_tf_music_send_songlist_request();
	s_tf_user_scan_replay = 1;
	s_tf_refresh_got_file = 0;
	s_songlist_refresh_wait = 1;
	s_songlist_refresh_start_ms = divoom_tf_music_refresh_now_ms();
	s_songlist_transfer_active = 0;
	if (divoom_btcore_uart_cmd_send_scan_music_list() != 0)
	{
		divoom_print("tf music scan music list send fail\n");
		s_tf_user_scan_replay = 0;
		divoom_tf_music_finish_songlist_refresh(0);
	}
}

void divoom_tf_music_send_scan_music_list(void)
{
#if DIVOOM_SHOW_TF_NAND
	divoom_tf_music_sync_prepare();
	s_tf_user_scan_replay = 1;
	s_tf_refresh_got_file = 0;
	s_songlist_refresh_wait = 1;
	s_songlist_refresh_start_ms = divoom_tf_music_refresh_now_ms();
	s_songlist_transfer_active = 0;
	divoom_tf_music_scan_root_dir(DIVOOM_TF_MUSIC_NAND_DIR);
	s_tf_refresh_got_file = 1;
	divoom_tf_music_user_refresh_complete_after_file();
	return;
#endif
#ifdef DIVOOM_SIMULATOR
	divoom_tf_music_sync_prepare();
	s_tf_user_scan_replay = 1;
	s_tf_refresh_got_file = 0;
	s_songlist_refresh_wait = 1;
	s_songlist_refresh_start_ms = divoom_tf_music_refresh_now_ms();
	s_songlist_transfer_active = 0;
	divoom_tf_music_sim_scan_start_async();
	return;
#endif
	divoom_tf_music_send_scan_music_list_internal();

}
#endif

void divoom_tf_music_get_music_list_callback(void *handle, int id, int like, char *file_name, char *root_name)
{
	(void)handle;
	(void)id;
	(void)like;
	(void)file_name;
	(void)root_name;
}

#ifdef DIVOOM_SIMULATOR
#include <dirent.h>
#include <strings.h>

#define DIVOOM_TF_SIM_SCAN_MAX_FILES  4096
#define DIVOOM_TF_SIM_SCAN_MAX_DEPTH  16

static volatile int s_sim_tf_scan_running;
static volatile int s_sim_tf_scan_ui_notify;

static void divoom_tf_music_sim_scan_start_async(void);
static void divoom_tf_music_sim_poll_scan_done(void);
static void divoom_tf_music_sim_boot_scan_once(void);
static int divoom_tf_music_sim_scan_tf_dir(void);

static void divoom_tf_music_sim_host_path_join(char *out, int out_sz, const char *dir, const char *name)
{
    size_t len;

    if (out == NULL || out_sz <= 0)
    {
        return;
    }
    if (name == NULL || name[0] == '\0')
    {
        if (dir != NULL && dir[0] != '\0')
        {
            snprintf(out, (size_t)out_sz, "%s", dir);
        }
        else
        {
            out[0] = '\0';
        }
        return;
    }
    if (dir == NULL || dir[0] == '\0')
    {
        snprintf(out, (size_t)out_sz, "%s", name);
        return;
    }
    len = strlen(dir);
    if (dir[len - 1] == '/' || dir[len - 1] == '\\')
    {
        snprintf(out, (size_t)out_sz, "%s%s", dir, name);
    }
    else
    {
        snprintf(out, (size_t)out_sz, "%s\\%s", dir, name);
    }
}

static void divoom_tf_music_sim_dev_path_join(char *out, int out_sz, const char *rel, const char *name)
{
    if (out == NULL || out_sz <= 0)
    {
        return;
    }
    if (name == NULL || name[0] == '\0')
    {
        if (rel != NULL && rel[0] != '\0')
        {
            snprintf(out, (size_t)out_sz, "%s", rel);
        }
        else
        {
            out[0] = '\0';
        }
        return;
    }
    if (rel == NULL || rel[0] == '\0')
    {
        snprintf(out, (size_t)out_sz, "/%s", name);
        return;
    }
    if (rel[0] == '/')
    {
        snprintf(out, (size_t)out_sz, "%s/%s", rel, name);
    }
    else
    {
        snprintf(out, (size_t)out_sz, "/%s/%s", rel, name);
    }
}

static void divoom_tf_music_sim_scan_host_dir_recursive(Pdivoom_tf_music_ctrl phandle,
    const char *host_dir, const char *dev_rel, int depth, int *seeded)
{
    DIR *dir;
    const divoom_port_dirent_t *ent;
    char child_host[512];
    char child_dev[512];
    char root_buf[DIVOOM_TF_MUSIC_ROOT_NAME_LEN];
    const char *root_name;
    int record_id;

    if (host_dir == NULL || dev_rel == NULL || seeded == NULL || *seeded >= DIVOOM_TF_SIM_SCAN_MAX_FILES)
    {
        return;
    }
    if (depth > DIVOOM_TF_SIM_SCAN_MAX_DEPTH)
    {
        return;
    }

    dir = divoom_port_dir_open(host_dir);
    if (dir == NULL)
    {
        if (depth == 0)
        {
            divoom_print("[sim_tf] opendir %s failed\n", host_dir);
        }
        return;
    }

    while ((ent = divoom_port_dir_read(dir)) != NULL)
    {
        const char *ext;

        if (ent->name[0] == '.')
        {
            continue;
        }
        if ((int)strlen(ent->name) >= (int)sizeof(child_host) - 32)
        {
            continue;
        }

        divoom_tf_music_sim_host_path_join(child_host, (int)sizeof(child_host), host_dir, ent->name);
        divoom_tf_music_sim_dev_path_join(child_dev, (int)sizeof(child_dev), dev_rel, ent->name);

        if (ent->is_dir)
        {
            divoom_tf_music_sim_scan_host_dir_recursive(phandle, child_host, child_dev, depth + 1, seeded);
            continue;
        }

        ext = strrchr(ent->name, '.');
        if (ext != NULL && strcasecmp(ext, ".mp3") == 0)
        {
            root_name = "";
            if (divoom_tf_music_parse_root_name(child_dev, root_buf, (int)sizeof(root_buf)))
            {
                root_name = root_buf;
            }
            record_id = *seeded + 1;
            if (divoom_tf_music_apply_track_path(record_id, child_dev))
            {
                (void)phandle;
                (void)root_name;
            }
            (*seeded)++;
            if (*seeded >= DIVOOM_TF_SIM_SCAN_MAX_FILES)
            {
                break;
            }
            continue;
        }

        if (!ent->is_dir)
        {
            continue;
        }

        /* 部分 FindFirstFile 结果 d_type 不可靠：再尝试当作目录递归 */
        {
            DIR *probe = divoom_port_dir_open(child_host);
            if (probe != NULL)
            {
                divoom_port_dir_close(probe);
                divoom_tf_music_sim_scan_host_dir_recursive(phandle, child_host, child_dev, depth + 1, seeded);
            }
        }
    }
    divoom_port_dir_close(dir);
}

static int divoom_tf_music_sim_scan_tf_dir(void)
{
    Pdivoom_tf_music_ctrl phandle = pdivoom_tf_music_handle;
    char host_dir[512];
    int seeded = 0;

    if (!divoom_tf_music_ctrl_init_if_needed())
    {
        return 0;
    }
    phandle = pdivoom_tf_music_handle;
    if (phandle == NULL)
    {
        return 0;
    }

    {
        const char *resolved = divoom_port_path_resolve("/tf");

        if (resolved == NULL || resolved[0] == '\0')
        {
            return 0;
        }
        strncpy(host_dir, resolved, sizeof(host_dir) - 1U);
        host_dir[sizeof(host_dir) - 1U] = '\0';
    }
    divoom_print("[sim_tf] scan host dir: %s\n", host_dir);

    divoom_database_tf_rebuild_begin();
    divoom_tf_music_sim_scan_host_dir_recursive(phandle, host_dir, "", 0, &seeded);
    divoom_database_tf_rebuild_end();

    phandle->total = divoom_database_tf_total();
    phandle->id_base = 1;
    if (seeded > 0)
    {
        divoom_print("[sim_tf] scan refreshed %d tracks from /tf (queue=%d)\n",
            seeded, phandle->total);
    }
    else
    {
        divoom_print("[sim_tf] no mp3 found under /tf (host=%s)\n", host_dir);
    }
    {
        const char *play_env;

        play_env = getenv("DIVOOM_SIM_TF_PLAY_ID");
        if (play_env != NULL && play_env[0] != '\0')
        {
            int play_id = atoi(play_env);
            if (play_id > 0)
            {
                divoom_print("[sim_tf] smoke auto-play id=%d\n", play_id);
                divoom_tf_music_sim_play_file_for_id(play_id);
            }
        }
    }
    return seeded;
}

static void *divoom_tf_music_sim_scan_task(void *param)
{
    (void)param;
    divoom_tf_music_sim_scan_tf_dir();
    s_sim_tf_scan_running = 0;
    s_sim_tf_scan_ui_notify = 1;
    return NULL;
}

static void divoom_tf_music_sim_scan_start_async(void)
{
    if (s_sim_tf_scan_running)
    {
        return;
    }
    s_sim_tf_scan_running = 1;
    s_sim_tf_scan_ui_notify = 0;
    divoom_system_create_task(divoom_tf_music_sim_scan_task, "tf_sim_scan", NULL, DIVOOM_TASK_STACK_TF_SIM_SCAN);
}

static void divoom_tf_music_sim_poll_scan_done(void)
{
    if (!s_sim_tf_scan_ui_notify)
    {
        return;
    }
    s_sim_tf_scan_ui_notify = 0;
    if (s_songlist_refresh_wait)
    {
        s_tf_refresh_got_file = 1;
        divoom_tf_music_user_refresh_complete_after_file();
    }
}

static void divoom_tf_music_sim_boot_scan_once(void)
{
    static int s_boot_scan_started;

    if (s_boot_scan_started)
    {
        return;
    }
    s_boot_scan_started = 1;
    if (divoom_database_manage_get_local_music_count() > 0)
    {
        return;
    }
    divoom_tf_music_sim_scan_start_async();
}

void divoom_tf_music_sim_request_catalog(void)
{
    divoom_tf_music_sync_prepare();
    if (divoom_tf_music_sim_is_scanning())
    {
        return;
    }
    if (divoom_database_manage_get_local_music_count() > 0)
    {
        return;
    }
    divoom_tf_music_sim_scan_start_async();
}

int divoom_tf_music_sim_is_scanning(void)
{
    return s_sim_tf_scan_running ? 1 : 0;
}
#endif /* DIVOOM_SIMULATOR */

static int divoom_tf_music_pick_next_id(int cur_id, int mode)
{
	int next = cur_id;
	int max_id;

	max_id = divoom_database_tf_max_id();
	if (max_id <= 0)
		return 0;
	switch (mode) {
	case DIVOOM_FT_MUSIC_MODE_ONE:
		return cur_id > 0 ? cur_id : divoom_database_tf_min_id();
	case DIVOOM_FT_MUSIC_MODE_RANDOM:
		next = divoom_database_tf_random_id((rand() % max_id) + 1);
		break;
	default:
		next = divoom_database_tf_next_id(cur_id > 0 ? cur_id : 0);
		break;
	}
#ifdef DIVOOM_SIMULATOR
	{
		int tries;

		for (tries = 0; tries < DIVOOM_TF_SKIP_RING + 1 && next > 0; tries++) {
			if (!divoom_tf_music_skip_has(next))
				break;
			next = divoom_database_tf_next_id(next);
		}
	}
#endif
	return next;
}

static int divoom_tf_music_pick_prev_id(int cur_id, int mode)
{
	if (mode == DIVOOM_FT_MUSIC_MODE_RANDOM)
		return divoom_tf_music_pick_next_id(cur_id, DIVOOM_FT_MUSIC_MODE_RANDOM);
	return divoom_database_tf_prev_id(cur_id > 0 ? cur_id : 0);
}

int divoom_tf_music_play_get_next(char *music_name, int status)
{
	int next;

	if (pdivoom_tf_music_handle == NULL)
		return 0;
	if (status == 0) {
#ifdef DIVOOM_SIMULATOR
		divoom_tf_music_skip_add(pdivoom_tf_music_handle->cur_play_id);
#endif
	}
	pdivoom_tf_music_handle->total = divoom_database_tf_total();
	if (pdivoom_tf_music_handle->total <= 0)
		return 0;
	next = divoom_tf_music_pick_next_id(pdivoom_tf_music_handle->cur_play_id,
		pdivoom_tf_music_handle->mode);
	if (next <= 0)
		return 0;
	divoom_tf_music_refresh_cur_from_id(pdivoom_tf_music_handle, next);
	if (music_name) {
#if DIVOOM_SHOW_TF_NAND
		snprintf(music_name, 256, "%s", pdivoom_tf_music_handle->music_name);
#else
		snprintf(music_name, 256, "file://%s", pdivoom_tf_music_handle->cur_path);
#endif
	}
	if (pdivoom_tf_music_handle->mode != DIVOOM_FT_MUSIC_MODE_ONE)
		divoom_main_set_tf_music_id(next);
	return pdivoom_tf_music_handle->cur_path[0] ? 1 : 0;
}

int divoom_tf_music_play_finished_send_play_next(int jl_err)
{
	int next;
	uint32_t now_ms;
	uint32_t elapsed_ms;

	if(pdivoom_tf_music_handle == NULL)
    {
        return 0;
    }

#ifdef DIVOOM_TIMES_VIBE
	/* JL：0=NULL 1=SUCC 正常切；5=DECODE_FAIL 按方向跳过；其余先不自动切 */
	now_ms = divoom_system_get_tickms();
	elapsed_ms = (s_tf_play_start_ms != 0u) ? (now_ms - s_tf_play_start_ms) : 0u;

	if (jl_err == 0 || jl_err == 1)
	{
		s_tf_play_skip_blocked = 0;
		s_tf_play_fast_fail_count = 0;
		s_tf_fail_seek_prev = 0;
		if (s_tf_play_last_next_ms != 0u
		    && (now_ms - s_tf_play_last_next_ms) < DIVOOM_TF_PLAY_NEXT_MIN_GAP_MS)
			return 0;
	}
	else if (jl_err == 5)
	{
		if (s_tf_play_skip_blocked)
		{
			divoom_print("tf music: auto skip blocked after repeated fast finish\n");
			return 0;
		}
		s_tf_play_fast_fail_count++;
		divoom_print("tf music: play err=%d %ums (%u/%u)%s\n",
			     jl_err,
			     (unsigned)elapsed_ms,
			     (unsigned)s_tf_play_fast_fail_count,
			     (unsigned)DIVOOM_TF_PLAY_MAX_FAST_FAIL,
			     s_tf_fail_seek_prev ? " seek prev" : "");
		if (s_tf_play_fast_fail_count >= DIVOOM_TF_PLAY_MAX_FAST_FAIL)
		{
			s_tf_play_skip_blocked = 1;
			divoom_print("tf music: stop auto skip, JL play failed repeatedly\n");
			return 0;
		}
	}
	else
	{
		divoom_print("tf music: finish err=%d ignored, no auto skip\n", jl_err);
		return 0;
	}
#else
	(void)jl_err;
#endif

	pdivoom_tf_music_handle->total = divoom_database_tf_total();
	if (pdivoom_tf_music_handle->total <= 0)
		return 0;
#ifdef DIVOOM_TIMES_VIBE
	if (s_tf_fail_seek_prev)
		next = divoom_tf_music_pick_prev_id(pdivoom_tf_music_handle->cur_play_id,
			pdivoom_tf_music_handle->mode);
	else
		next = divoom_tf_music_pick_next_id(pdivoom_tf_music_handle->cur_play_id,
			pdivoom_tf_music_handle->mode);
#else
	next = divoom_tf_music_pick_next_id(pdivoom_tf_music_handle->cur_play_id,
		pdivoom_tf_music_handle->mode);
#endif
	if (next <= 0)
		return 0;
	divoom_tf_music_refresh_cur_from_id(pdivoom_tf_music_handle, next);
	if (pdivoom_tf_music_handle->mode != DIVOOM_FT_MUSIC_MODE_ONE)
		divoom_main_set_tf_music_id(next);
	divoom_print("music_name: %s\n", pdivoom_tf_music_handle->cur_path);
	divoom_tf_music_send_play_music_id(next);
#ifdef DIVOOM_TIMES_VIBE
	s_tf_play_last_next_ms = divoom_system_get_tickms();
#endif
	return 1;
}

void *divoom_tf_music_get_active_queue(void)
{
	return NULL;
}

static Pdivoom_tf_music_item divoom_tf_music_resolve_item_by_id(int music_id)
{
	if (pdivoom_tf_music_handle == NULL)
		return NULL;
	divoom_tf_music_refresh_cur_from_id(pdivoom_tf_music_handle, music_id);
	if (pdivoom_tf_music_handle->cur_play_id <= 0)
		return NULL;
	return pdivoom_tf_music_handle->pcur_play_item;
}

/* 仅更新内存中的当前曲目，不通知 JL/本地播放器 */
static void divoom_tf_music_apply_music_id(int music_id)
{
    Pdivoom_tf_music_item pitem;

    pitem = divoom_tf_music_resolve_item_by_id(music_id);
    if (pitem == NULL)
    {
        return;
    }

    divoom_tf_music_set_cur_play_item(pdivoom_tf_music_handle, pitem);
}

void divoom_tf_music_set_music_id(int music_id)
{
    Pdivoom_tf_music_item pitem;
    int cur_id = 0;

    if (pdivoom_tf_music_handle == NULL)
        return;

    /* resolve 会刷新 pcur，必须先记下当前曲，否则同 id 判断永远成立、0x0e 发不出去 */
    cur_id = pdivoom_tf_music_handle->cur_play_id;

    pitem = divoom_tf_music_resolve_item_by_id(music_id);
    if (pitem == NULL || pitem->music_name == NULL || pitem->music_name[0] == '\0')
    {
#ifdef DIVOOM_TIMES_VIBE
        s_tf_songlist_force_replay = 0;
#endif
        return;
    }

    divoom_tf_music_set_cur_play_item(pdivoom_tf_music_handle, pitem);

#ifdef DIVOOM_TIMES_VIBE
    if (cur_id == pitem->id && !s_tf_songlist_force_replay)
    {
        return;
    }
    s_tf_songlist_force_replay = 0;
#else
    if (cur_id == pitem->id)
    {
        return;
    }
#endif

    divoom_print("music_name: %s\n", pdivoom_tf_music_handle->music_name);

#ifdef DIVOOM_TIMES_VIBE
	s_tf_play_skip_blocked = 0;
	s_tf_play_fast_fail_count = 0;
	s_tf_fail_seek_prev = 0;
    divoom_tf_music_send_play_music_id(pitem->id);
    if (divoom_music_ctrl_get_mode() == DIVOOM_MUSIC_CTRL_MODE_LOCAL)
        divoom_music_ctrl_set_playing_flag(1);
#else
    divoom_local_music_ctrl_music_play(pdivoom_tf_music_handle->music_name, divoom_tf_music_get_volume(), divoom_tf_music_play_get_next);
#endif
}

void divoom_tf_music_send_get_music_count(void)
{
#ifdef DIVOOM_TIMES_VIBE
	/* JL 会 0x42 查询歌单；0x4d 扫描由 UI 手动触发，不在此发送 */
	divoom_tf_music_sync_prepare();
	divoom_tf_music_send_songlist_request();
#endif
}

static void *divoom_tf_music_bootstrap_thread(void *param)
{
	(void)param;

#if DIVOOM_SHOW_TF_NAND
	divoom_tf_music_scan_root_dir(DIVOOM_TF_MUSIC_NAND_DIR);
	return NULL;
#endif
	/* 后台：读 DB 歌单 + 必要时解析 bin，不阻塞开机视频 */
	divoom_tf_music_sync_prepare();
	divoom_tf_music_sync_set_reason("boot_bootstrap");
	if (!divoom_tf_music_songlist_cache_hit(NULL))
		divoom_tf_music_load_songlist_file(DIVOOM_TF_MUSIC_SONGLIST_PATH);
	divoom_tf_music_send_songlist_request();
	return NULL;
}

void divoom_tf_music_bootstrap_at_boot(void)
{
#if DIVOOM_SHOW_TF_NAND
	divoom_system_create_task(divoom_tf_music_bootstrap_thread, "tf_bootstrap", NULL, DIVOOM_TASK_STACK_TF_BOOTSTRAP);
	return;
#endif
#ifdef DIVOOM_TIMES_VIBE
	divoom_system_create_task(divoom_tf_music_bootstrap_thread, "tf_bootstrap", NULL, DIVOOM_TASK_STACK_TF_BOOTSTRAP);
#endif
}

static void divoom_tf_music_start_play_saved(void)
{
	int music_id;

	if (pdivoom_tf_music_handle == NULL)
		return;

	pdivoom_tf_music_handle->play_order = divoom_main_set_tf_music_order(-1);
	pdivoom_tf_music_handle->total = divoom_database_tf_total();
	if (pdivoom_tf_music_handle->total <= 0)
	{
		s_tf_songlist_force_replay = 1;
		s_tf_need_catchup_play = 1;
		divoom_print("tf music: defer play, songlist not ready\n");
		return;
	}

	music_id = divoom_main_set_tf_music_id(-1);
	{
		char path[DIVOOM_TF_PATH_MAX];

		if (music_id <= 0
		    || !divoom_database_tf_track_path(music_id, path, (int)sizeof(path)))
			music_id = divoom_tf_music_get_first_music_id();
	}
	if (music_id <= 0)
	{
		s_tf_songlist_force_replay = 1;
		s_tf_need_catchup_play = 1;
		divoom_print("tf music: defer play, no valid track\n");
		return;
	}
	s_tf_need_catchup_play = 0;
	if (divoom_main_set_tf_music_id(-1) != music_id)
		divoom_main_set_tf_music_id(music_id);
	divoom_tf_music_set_music_id(music_id);
}

/* 仅补「0x5e 已到但当时没曲可播」；吃掉 5e pending，避免 defer 任务再发一次 0x0e */
static void divoom_tf_music_try_play_saved_if_jl_ready(void)
{
	if (!s_tf_need_catchup_play)
		return;
	if (divoom_music_ctrl_get_mode() != DIVOOM_MUSIC_CTRL_MODE_LOCAL)
		return;
	if (!divoom_music_ctrl_tf_jl_ready_for_play())
		return;
	s_tf_need_catchup_play = 0;
	(void)divoom_tf_music_take_jl_5e_pending_play();
	divoom_tf_music_play_saved_force_replay();
}

/*********************************************************************
  Function:        divoom_tf_music_start
  Description:     启动播放
  Input:           none
  Return:          none
**********************************************************************/

#ifdef DIVOOM_TIMES_VIBE
void divoom_tf_music_prepare(void)
{
	if (pdivoom_tf_music_handle != NULL)
	{
		pdivoom_tf_music_handle->mode = divoom_main_set_tf_music_mode(-1);
		pdivoom_tf_music_handle->play_order = divoom_main_set_tf_music_order(-1);
	}
	else if (!divoom_tf_music_ctrl_init_if_needed())
	{
		return;
	}
#if DIVOOM_SHOW_TF_NAND
	divoom_tf_music_scan_root_dir(DIVOOM_TF_MUSIC_NAND_DIR);
	return;
#else
	divoom_tf_music_send_get_music_count();
#endif
}

void divoom_tf_music_play_saved(void)
{
	divoom_tf_music_start_play_saved();
}

void divoom_tf_music_play_saved_force_replay(void)
{
	s_tf_songlist_force_replay = 1;
	divoom_tf_music_start_play_saved();
}

void divoom_tf_music_clear_songlist_force_replay(void)
{
	s_tf_songlist_force_replay = 0;
}

static volatile int s_tf_hotplug_pending_mask;
static uint8 s_tf_hotplug_task_run;
static volatile uint8 s_tf_ui_hotplug_reload;

void divoom_tf_music_request_ui_hotplug_reload(void)
{
	s_tf_ui_hotplug_reload = 1;
}

#define DIVOOM_TF_HOTPLUG_REMOVE  1
#define DIVOOM_TF_HOTPLUG_INSERT  2

static void divoom_tf_music_on_tf_hotplug_impl(int inserted);
static void divoom_tf_music_schedule_hotplug_task(void);
static void *divoom_tf_music_hotplug_task(void *arg);

static void divoom_tf_music_schedule_hotplug_task(void)
{
	if (s_tf_hotplug_task_run)
		return;
	s_tf_hotplug_task_run = 1;
	divoom_system_create_task(divoom_tf_music_hotplug_task, "tf_hotplug", NULL, DIVOOM_TASK_STACK_TF_HOTPLUG);
}

static void *divoom_tf_music_hotplug_task(void *arg)
{
	(void)arg;
	divoom_system_delay_ms(50);

	for (;;)
	{
		if (!(s_tf_hotplug_pending_mask & DIVOOM_TF_HOTPLUG_REMOVE)
		    && !(s_tf_hotplug_pending_mask & DIVOOM_TF_HOTPLUG_INSERT))
			break;

		if (s_tf_hotplug_pending_mask & DIVOOM_TF_HOTPLUG_REMOVE)
		{
			s_tf_hotplug_pending_mask &= ~DIVOOM_TF_HOTPLUG_REMOVE;
			divoom_tf_music_on_tf_hotplug_impl(0);
		}
		if (s_tf_hotplug_pending_mask & DIVOOM_TF_HOTPLUG_INSERT)
		{
			s_tf_hotplug_pending_mask &= ~DIVOOM_TF_HOTPLUG_INSERT;
			divoom_tf_music_on_tf_hotplug_impl(1);
		}
	}

	s_tf_hotplug_task_run = 0;
	if (s_tf_hotplug_pending_mask)
		divoom_tf_music_schedule_hotplug_task();
	return NULL;
}

static void divoom_tf_music_on_tf_hotplug_impl(int inserted)
{
	if (!inserted)
	{
		s_tf_wait_jl_songlist = 0;
		s_tf_hotplug_insert_ms = 0;
		s_tf_jl_5e_defer_ms = 0;
		s_tf_play_via_jl_5e_pending = 0;
		s_tf_need_catchup_play = 0;
		s_tf_user_scan_replay = 0;
		divoom_tf_music_finish_songlist_refresh(0);
		divoom_tf_music_reset_play_fail_guard();
		divoom_print("tf music hotplug remove\n");
		/* 拆除已结束；follow 杰理放到 music_jl_def，避免与本栈 stop/关库叠在一起 */
		divoom_music_ctrl_on_tf_unplug_teardown_done();
		return;
	}

	divoom_print("tf music hotplug insert\n");
#if DIVOOM_SHOW_TF_NAND
	return;
#endif
	s_tf_wait_jl_songlist = 1;
	s_tf_hotplug_insert_ms = divoom_system_get_tickms();
	s_tf_jl_5e_defer_ms = 0;
	divoom_tf_music_reset_play_fail_guard();
	divoom_tf_music_sync_prepare();
	/* 换卡/重插：不发 0x4d。JL 自扫后发 0x42，不一致再推歌单；0x4d 仅 UI 手动刷新 */
	divoom_tf_music_send_songlist_request();
	divoom_tf_music_request_ui_hotplug_reload();
}

uint8 divoom_tf_music_defer_jl_play_for_songlist(void)
{
	if (!s_tf_wait_jl_songlist)
		return 0;

	if (s_songlist_transfer_active)
		return 1;

	/* 0x5e 后留短窗给歌单文件回调；换卡先推歌单再 0x5e，同卡无传输则超时后播 */
	if (s_tf_jl_5e_defer_ms != 0u)
	{
		if (divoom_system_get_tickms() - s_tf_jl_5e_defer_ms
		    < DIVOOM_TF_HOTPLUG_SONGLIST_PLAY_GRACE_MS)
			return 1;
		s_tf_wait_jl_songlist = 0;
		s_tf_hotplug_insert_ms = 0;
		s_tf_jl_5e_defer_ms = 0;
		return 0;
	}

	/* 尚未收到 0x5e：继续等歌单，不要按插卡时刻立刻放行（原 GRACE=0 会马上播） */
	return 1;
}

void divoom_tf_music_on_jl_play_deferred_for_songlist(void)
{
	if (s_tf_jl_5e_defer_ms == 0u)
		s_tf_jl_5e_defer_ms = divoom_system_get_tickms();
	s_tf_play_via_jl_5e_pending = 1;
}

uint8 divoom_tf_music_take_jl_5e_pending_play(void)
{
	if (!s_tf_play_via_jl_5e_pending)
		return 0;
	s_tf_play_via_jl_5e_pending = 0;
	return 1;
}

int divoom_tf_music_take_ui_hotplug_reload(void)
{
	if (!s_tf_ui_hotplug_reload)
		return 0;

	s_tf_ui_hotplug_reload = 0;
	return 1;
}

int divoom_tf_music_ui_hotplug_reload_pending(void)
{
	return s_tf_ui_hotplug_reload ? 1 : 0;
}

void divoom_tf_music_on_tf_hotplug(int inserted)
{
	if (inserted)
		s_tf_hotplug_pending_mask |= DIVOOM_TF_HOTPLUG_INSERT;
	else
		s_tf_hotplug_pending_mask |= DIVOOM_TF_HOTPLUG_REMOVE;
	divoom_tf_music_schedule_hotplug_task();
}
#endif

void divoom_tf_music_start(void)
{
    if (pdivoom_tf_music_handle != NULL)
    {
        pdivoom_tf_music_handle->mode = divoom_main_set_tf_music_mode(-1);
        pdivoom_tf_music_handle->play_order = divoom_main_set_tf_music_order(-1);
    }
    else if (!divoom_tf_music_ctrl_init_if_needed())
    {
        return;
    }

#ifdef DIVOOM_TIMES_VIBE
    divoom_tf_music_prepare();
    divoom_tf_music_play_saved();
#else
    divoom_tf_music_send_get_music_count();
    divoom_tf_music_start_play_saved();
#endif
}

int divoom_tf_music_compare_music_id(void *item1, void *item2)
{
    int *id = (int *)item1;
    Pdivoom_tf_music_item pitem = (Pdivoom_tf_music_item)item2;

    if(*id == pitem->id)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

/*********************************************************************
  Function:        divoom_tf_music_set_like_flag
  Description:     设置歌曲的收藏偏好
  Input:           id
                   flag
  Return:          none
**********************************************************************/


void divoom_tf_music_set_like_flag(int id, int like_flag)
{
    if (id <= 0)
        return;
    divoom_database_tf_set_like(id, like_flag);
}

/*********************************************************************
  Function:        divoom_tf_music_set_mode
  Description:     设置播放偏好
  Input:           mode
  Return:          none
**********************************************************************/


void divoom_tf_music_set_order(int mode)
{
    if(pdivoom_tf_music_handle != NULL)
        pdivoom_tf_music_handle->play_order = mode;
}

/*********************************************************************
  Function:        divoom_tf_music_set_mode
  Description:     设置播放模式
  Input:           mode
  Return:          none
**********************************************************************/


void divoom_tf_music_set_mode(int mode)
{
    if(pdivoom_tf_music_handle != NULL)
    {
        pdivoom_tf_music_handle->mode = mode;
    }
}
/*********************************************************************
  Function:        divoom_tf_music_stop
  Description:     停止本地播放状态
  Input:           none
  Return:          1:播放；0：暂停
**********************************************************************/

void divoom_tf_music_stop(void)
{
#ifdef DIVOOM_TIMES_VIBE
    return;
#else
    if (pdivoom_tf_music_handle == NULL)
        return;
#ifdef DIVOOM_SIMULATOR
    divoom_local_music_ctrl_stop_music();
#else
    divoom_local_music_ctrl_stop_music();
	divoom_local_music_ctrl_stop_music_and_wait(500);
#endif
	divoom_main_set_tf_music_order(pdivoom_tf_music_handle->play_order);
	divoom_main_set_tf_music_mode(pdivoom_tf_music_handle->mode);
    divoom_system_free(pdivoom_tf_music_handle);
    pdivoom_tf_music_handle = NULL;
#endif
}

/*********************************************************************
  Function:        divoom_tf_music_get_playing_music_id
  Description:     获取本地播放歌曲ID
  Input:           none
  Return:          ID
**********************************************************************/

int divoom_tf_music_get_playing_music_id(void)
{
    if(pdivoom_tf_music_handle == NULL || pdivoom_tf_music_handle->pcur_play_item == NULL)
    {
        return 0;
    }
    return pdivoom_tf_music_handle->pcur_play_item->id;
}


/*********************************************************************
  Function:        divoom_tf_music_set_playing_music_id
  Description:     启动本地播放歌曲ID
  Input:           none
  Return:          ID
**********************************************************************/

int divoom_tf_music_folder_left_to_play_order(int left_selected_index)
{
#if DIVOOM_TF_MUSIC_FAVORITES_UI
    return left_selected_index;
#else
    if (left_selected_index <= 0)
        return 0;
    return left_selected_index + 1;
#endif
}

int divoom_tf_music_folder_play_order_to_left(int play_order)
{
#if DIVOOM_TF_MUSIC_FAVORITES_UI
    if (play_order < 0)
        return 0;
    return play_order;
#else
    if (play_order <= 0)
        return 0;
    if (play_order == 1)
        return 0;
    return play_order - 1;
#endif
}

int divoom_tf_music_get_play_order(void)
{
    if (pdivoom_tf_music_handle == NULL)
        return 0;
    return pdivoom_tf_music_handle->play_order;
}

void divoom_tf_music_restore_saved_playback(void)
{
#ifdef DIVOOM_TIMES_VIBE
    int music_id;

    if (!divoom_tf_music_ctrl_init_if_needed())
        return;

    pdivoom_tf_music_handle->play_order = divoom_main_set_tf_music_order(-1);

    /* 已在播放：只同步 play_order / pcur，不向 JL 重发 0x0e */
    if (pdivoom_tf_music_handle->pcur_play_item == NULL)
    {
        music_id = divoom_main_set_tf_music_id(-1);
        if (music_id > 0)
            divoom_tf_music_apply_music_id(music_id);
    }

#endif
}

#ifdef DIVOOM_TIMES_VIBE
/* 同步 JL 播放状态到 playing；查询失败且已有当前曲目时假定仍在播 */
void divoom_tf_music_sync_play_status_ui(void)
{
    if (!divoom_tf_music_ctrl_init_if_needed())
        return;

    if (divoom_music_ctrl_sync_play_status_from_jl() != 0
        && pdivoom_tf_music_handle->pcur_play_item != NULL
        && divoom_music_ctrl_get_mode() == DIVOOM_MUSIC_CTRL_MODE_LOCAL)
    {
        divoom_music_ctrl_set_playing_flag(1);
    }
}
#endif

void divoom_tf_music_set_playing_music_id(int play_order, int music_id)
{
    if(pdivoom_tf_music_handle == NULL)
    {
        return ;
    }
    pdivoom_tf_music_handle->play_order = play_order;
    divoom_main_set_tf_music_order(play_order);
    divoom_main_set_tf_music_id(music_id);
    divoom_tf_music_set_music_id(music_id);
}

Pdivoom_tf_music_item divoom_tf_music_get_item_by_id(int music_id)
{
	return divoom_tf_music_resolve_item_by_id(music_id);
}

int divoom_tf_music_find_id_by_path(const char *music_path)
{
	return divoom_database_tf_find_id_by_path(music_path);
}

int divoom_tf_music_alarm_resolve_music_id(const char *leave_url)
{
	int id;
	char *end;

	if(leave_url == NULL || leave_url[0] == '\0')
	{
		return 0;
	}
	id = (int)strtol(leave_url, &end, 10);
	if(end > leave_url && *end == '\0' && id > 0)
	{
		return id;
	}
	return divoom_tf_music_find_id_by_path(leave_url);
}

int divoom_tf_music_get_first_music_id(void)
{
	return divoom_database_tf_min_id();
}

#ifdef DIVOOM_TIMES_VIBE
int divoom_tf_music_play_music_id_ex(int music_id, uint8 volume, uint8 voice_owned)
{
	if(music_id <= 0)
	{
		return -1;
	}
#ifdef DIVOOM_SIMULATOR
	divoom_tf_music_set_volume(volume);
	divoom_tf_music_sim_play_file_for_id(music_id);
	return 0;
#endif
	/* 闹钟/试听等同曲目也需重发 0x0e，避免 cur_id 相同被 set_music_id 跳过 */
	s_tf_songlist_force_replay = 1;
	return divoom_music_ctrl_play_tf_track_id_ex(music_id, volume, voice_owned);
}

int divoom_tf_music_play_music_id(int music_id, uint8 volume)
{
	return divoom_tf_music_play_music_id_ex(music_id, volume, 0);
}

void divoom_tf_music_pause_jl_play(void)
{
#ifdef DIVOOM_SIMULATOR
	divoom_local_music_ctrl_stop_music();
	return;
#endif
	if(divoom_music_ctrl_get_mode() == DIVOOM_MUSIC_CTRL_MODE_LOCAL)
	{
		divoom_tf_music_music_play_ctrl(0);
	}
}
#endif

/*********************************************************************
  Function:        divoom_tf_music_get_playing_music_info
  Description:     获取本地播放歌曲信息
  Input:           none
  Return:          file_name
**********************************************************************/

Pdivoom_tf_music_item divoom_tf_music_get_playing_music_info(void)
{
    if(pdivoom_tf_music_handle == NULL || pdivoom_tf_music_handle->pcur_play_item == NULL)
    {
        return NULL;
    }
    return pdivoom_tf_music_handle->pcur_play_item;
}

char *divoom_tf_music_get_playing_disp_name(void)
{
	if (pdivoom_tf_music_handle == NULL)
		return (char *)"";
	return pdivoom_tf_music_handle->disp_name;
}

/*********************************************************************
  Function:        divoom_tf_music_get_all_music_info
  Description:     获取音乐列表
  Input:           total
                   flag  1:收藏列表； 0：全部列表
  Return:          音乐列表
**********************************************************************/

Pdivoom_tf_music_item divoom_tf_music_get_all_music_info(int *total, int flag)
{
	(void)flag;
	if (total)
		*total = 0;
	return NULL;
}

/*********************************************************************
  Function:        divoom_tf_music_get_music_status
  Description:     获取本地播放状态
  Input:           none
  Return:          1:播放；0：暂停
**********************************************************************/

int divoom_tf_music_get_music_status(void)
{
#ifdef DIVOOM_TIMES_VIBE
	if (divoom_music_ctrl_get_mode() == DIVOOM_MUSIC_CTRL_MODE_LOCAL)
		return divoom_music_ctrl_is_playing() ? 1 : 0;
#endif
	return divoom_local_music_ctrl_get_music_status();
}

/*********************************************************************
  Function:        divoom_tf_music_music_play_ctrl
  Description:     本地播放与暂停控制
  Input:           play  1:播放；0：暂停
  Return:          none
**********************************************************************/

void divoom_tf_music_music_play_ctrl(uint8 play)
{
#ifdef DIVOOM_SIMULATOR
	if (divoom_music_ctrl_get_mode() == DIVOOM_MUSIC_CTRL_MODE_LOCAL)
	{
		divoom_local_music_ctrl_music_ctrl(play);
		return;
	}
#endif
#ifdef DIVOOM_TIMES_VIBE
	if (divoom_music_ctrl_get_mode() == DIVOOM_MUSIC_CTRL_MODE_LOCAL)
	{
		divoom_music_ctrl_set_play(play ? 1 : 0);
		return;
	}
#endif
	divoom_local_music_ctrl_music_ctrl(play);
}

/*********************************************************************
  Function:        divoom_tf_music_set_volume
  Description:     设置 TF 卡音乐音量（通过 local_music_ctrl）
  Input:           volume  0-100
  Return:          none
**********************************************************************/

void divoom_tf_music_set_volume(uint8 volume)
{
    divoom_local_music_ctrl_set_music_volume(volume);
}

/*********************************************************************
  Function:        divoom_tf_music_get_volume
  Description:     获取 TF 卡音乐当前音量（0-100）
  Input:           none
  Return:          当前音量，未初始化返回 0
**********************************************************************/

uint8 divoom_tf_music_get_volume(void)
{
#ifdef DIVOOM_SIMULATOR
    Pdivoom_sys_setting_info psys;
    uint8 vol;

    psys = divoom_main_get_system_config();
    if (psys != NULL)
    {
        vol = psys->music_volume_local;
        if (vol > 100)
        {
            vol = 100;
        }
        return vol;
    }
    return 50;
#else
    return (uint8)divoom_local_music_ctrl_get_music_volume();
#endif
}

/*********************************************************************
  Function:        divoom_tf_music_add_music_item
  Description:     开机扫描音乐，先查询，如果不存在就插入
  Input:           file_name  
  Return:          none
**********************************************************************/

void divoom_tf_music_add_music_item(char *file_name)
{
	(void)file_name;
}

static void divoom_tf_music_play_id_now(int track_id, int fail_seek_prev)
{
	if (pdivoom_tf_music_handle == NULL || track_id <= 0)
		return;
	divoom_tf_music_refresh_cur_from_id(pdivoom_tf_music_handle, track_id);
	if (pdivoom_tf_music_handle->cur_play_id <= 0)
		return;
#ifdef DIVOOM_TIMES_VIBE
	s_tf_play_skip_blocked = 0;
	s_tf_play_fast_fail_count = 0;
	s_tf_fail_seek_prev = fail_seek_prev ? 1 : 0;
	divoom_main_set_tf_music_id(track_id);
	divoom_tf_music_send_play_music_id(track_id);
#else
	(void)fail_seek_prev;
	divoom_local_music_ctrl_music_play(pdivoom_tf_music_handle->music_name,
		divoom_tf_music_get_volume(), divoom_tf_music_play_get_next);
#endif
}

/*********************************************************************
  Function:        divoom_tf_music_music_play_prev
  Description:     本地播放下一首歌曲
  Input:           none
  Return:          none
**********************************************************************/

void divoom_tf_music_music_play_next(void)
{
	int next;

	if (pdivoom_tf_music_handle == NULL)
		return;
	next = divoom_tf_music_pick_next_id(pdivoom_tf_music_handle->cur_play_id,
		pdivoom_tf_music_handle->mode);
	divoom_tf_music_play_id_now(next, 0);
}

/*********************************************************************
  Function:        divoom_tf_music_music_play_prev
  Description:     本地播放上一首歌曲
  Input:           none
  Return:          none
**********************************************************************/

void divoom_tf_music_music_play_prev(void)
{
	int prev;

	if (pdivoom_tf_music_handle == NULL)
		return;
	if (pdivoom_tf_music_handle->mode == DIVOOM_FT_MUSIC_MODE_RANDOM)
		prev = divoom_tf_music_pick_next_id(pdivoom_tf_music_handle->cur_play_id,
			DIVOOM_FT_MUSIC_MODE_RANDOM);
	else
		prev = divoom_database_tf_prev_id(pdivoom_tf_music_handle->cur_play_id);
	divoom_tf_music_play_id_now(prev, 1);
}

int divoom_tf_music_get_like_music_count(void)
{
	return divoom_database_tf_like_count();
}

int divoom_tf_music_root_is_mount_label(const char *seg)
{
	if (seg == NULL || seg[0] == '\0')
		return 1;
	if (strcasecmp(seg, "tf") == 0 || strcasecmp(seg, "TF") == 0)
		return 1;
	if (strcasecmp(seg, "sdcard") == 0 || strcasecmp(seg, "SDCARD") == 0)
		return 1;
	return 0;
}

int divoom_tf_music_get_root_count(void)
{
	divoom_tf_folder_info info;

	if (!divoom_database_tf_folder_info(0, &info))
		return 0;
	return info.dir_cnt;
}

int divoom_tf_music_get_root_name(int index, char *buf, int buf_size)
{
	divoom_tf_entry ent;

	if (!buf || buf_size <= 0 || index < 0)
		return 0;
	buf[0] = '\0';
	if (divoom_database_tf_dir_page(0, index, 1, &ent, 1) <= 0 || !ent.is_dir)
		return 0;
	strncpy(buf, ent.name, (size_t)buf_size - 1u);
	buf[buf_size - 1] = '\0';
	return 1;
}

char *divoom_tf_music_get_cur_play_root_name(void)
{
    int index;

    if (pdivoom_tf_music_handle == NULL)
    {
	    return "";
    }

    if (pdivoom_tf_music_handle->play_order == 0)
    {
	    return divoom_menu_control_get_disp_str("Allpath");
    }

    if (pdivoom_tf_music_handle->play_order == 1)
    {
	    return divoom_menu_control_get_disp_str("Favorites");
    }

    index = pdivoom_tf_music_handle->play_order - 2;
    if (index >= 0 && divoom_tf_music_get_root_name(index, s_tf_root_name_buf, (int)sizeof(s_tf_root_name_buf)))
	    return s_tf_root_name_buf;
    return divoom_menu_control_get_disp_str("Allpath");
}

int divoom_tf_music_get_root_list(char **out_list, int max_count)
{
	(void)out_list;
	(void)max_count;
	return divoom_tf_music_get_root_count();
}

Pdivoom_tf_music_item divoom_tf_music_get_music_info_by_root(const char *root_name, int *total)
{
	(void)root_name;
	if (total)
		*total = 0;
	return NULL;
}

void *divoom_tf_music_get_root_queue_by_index(int index)
{
	(void)index;
	return NULL;
}

int divoom_tf_music_queue_count(void *queue)
{
	(void)queue;
	return 0;
}

Pdivoom_tf_music_item divoom_tf_music_queue_item_at(void *queue, int index)
{
	(void)queue;
	(void)index;
	return NULL;
}

void divoom_tf_music_pick_list_destroy(void *handle)
{
	(void)handle;
}

void *divoom_tf_music_pick_list_create(void)
{
	return NULL;
}

int divoom_tf_music_pick_list_root_count(void *handle)
{
	(void)handle;
	return divoom_tf_music_get_root_count();
}

int divoom_tf_music_pick_list_root_name(void *handle, int index, char *buf, int buf_size)
{
	(void)handle;
	return divoom_tf_music_get_root_name(index, buf, buf_size);
}

void *divoom_tf_music_pick_list_root_queue(void *handle, int index)
{
	(void)handle;
	(void)index;
	return NULL;
}

void *divoom_tf_music_pick_list_music_queue(void *handle)
{
	(void)handle;
	return NULL;
}

int divoom_tf_music_pick_list_find_id_by_path(void *handle, const char *music_path)
{
	(void)handle;
	return divoom_tf_music_find_id_by_path(music_path);
}

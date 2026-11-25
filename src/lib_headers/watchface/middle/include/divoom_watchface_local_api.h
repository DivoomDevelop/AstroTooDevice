
#ifndef DIVOOM_WATCHFACE_LOCAL_API_H
#define DIVOOM_WATCHFACE_LOCAL_API_H

#include "divoom_type.h"

/* POST /replace_clock_dial_bg 底图校验（与 §5.4.1 文档一致） */
#define DIVOOM_REPLACE_DIAL_BG_REQUIRED_W 480
#define DIVOOM_REPLACE_DIAL_BG_REQUIRED_H 480
/* 纯图片负载须小于 512000 字节（500KiB）；等于或超过均为失败 */
#define DIVOOM_REPLACE_DIAL_BG_MAX_FILE_BYTES ((int)(500 * 1024))
#define DIVOOM_REPLACE_DIAL_BG_MAX_HTTP_BODY ((int)((500 * 1024) + (96 * 1024)))
/* POST /create_local_clock 第二段为 tar.gz 包体时整体大小上限（gzip 后） */
#define DIVOOM_CREATE_LOCAL_CLOCK_BUNDLE_MAX_UPLOAD_BYTES ((int)(6 * 1024 * 1024))
/* tar 内 ItemList[].bundle_image 仅存 leaf 名，最大 95 字符 + '\0' */
#define DIVOOM_WATCHFACE_BUNDLE_FILENAME_MAX 96

/*********************************************************************
 * Local LAN watchface helpers for Device/GetLocalClockInfo,
 * Device/PatchLocalClockInfo, Device/GetLocalFontList,
 * Device/GetStoreClockMarketList,
 * Device/ResetLocalClockFromServer HTTP commands.
 *********************************************************************/

/* Returns heap-allocated JSON string for clock storage (sys layer) with root "Config" merged from
 * app layer when available (first AppList entry), for composite / SubClockId_* analysis by agents.
 * Caller must divoom_system_free or NULL on OOM. */
char *divoom_watchface_local_http_get_payload(int clock_id, char *errmsg, int errmsg_len);

char *divoom_watchface_local_http_get_full_json(int clock_id);

char *divoom_watchface_local_http_patch_response_json(const char *patch_body_json, char *errmsg, int errmsg_len);

/** POST /patch_local_clock：首段 Patch JSON（同 Device/PatchLocalClockInfo），可选第二段单底图或 tar.gz（DialAssets/bundle_image 规则同 create_local_clock；tar 可无 clock_bg 仅刷新元素）。 */
char *divoom_watchface_local_http_patch_local_clock_with_upload(const char *patch_body_json, const char *saved_upload_fullpath,
    int upload_file_len, char *errmsg, int errmsg_len);

char *divoom_watchface_local_http_get_font_list_full_json(void);

/** 商店分类预取完成后：返回全部表盘 ClockId + Name（跨分类去重，保留首次出现的名称） */
char *divoom_watchface_local_http_get_store_clock_market_list_json(void);

char *divoom_watchface_local_http_error_json(const char *command, const char *msg);

char *divoom_watchface_local_http_reset_from_server_response_json(int clock_id, int sync_requested_ok, const char *return_message_or_null);

/* Device/ReplaceClockDialBgFile + POST /replace_clock_dial_bg: overwrite dial bg cache (.bin path from divoom_manage_image_download_get_file_name), do not patch DeviceImageUrl. */
int divoom_watchface_replace_clock_dial_bg_resolve_clock_id(Pdivoom_clock_id_info pcid);

void divoom_watchface_replace_clock_dial_bg_cache_path(int clock_id, char *path_buf, int path_buf_len);

void divoom_watchface_replace_clock_dial_bg_apply_saved_file(int clock_id);

char *divoom_watchface_replace_clock_dial_bg_result_json(int clock_id, int return_code, const char *msg_if_null_ok);

/* 校验收尾文件：宽高、格式 JPG/WEBP、大小 < DIVOOM_REPLACE_DIAL_BG_MAX_FILE_BYTES。成功返回 0，失败返回 -1 并通过 errbuf 返回原因（UTF-8 中文短语）。*/
int divoom_watchface_replace_clock_dial_bg_validate_saved_file(const char *path, int file_size, char *errbuf, int errbuf_len);

/* Replace through the clock-write wait page, then restore/rebuild the displayed clock. */
char *divoom_watchface_local_http_replace_clock_dial_bg(int clock_id, const char *saved_file, int file_size);

/* POST /create_local_clock — build new clock{N}.cfg; ClockId from divoom_main_get_new_custon_clock_id()（user_clock_id_list.json / ClockIdList）。 */
char *divoom_watchface_local_http_create_local_clock(const char *json_body, const char *bg_saved_fullpath, int bg_file_len);

#endif

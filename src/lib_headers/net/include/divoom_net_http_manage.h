#ifndef _DIVOOM_NET_HTTP_MANAGE_H_
#define _DIVOOM_NET_HTTP_MANAGE_H_

#include "divoom_wifi_con.h"

void divoom_net_http_manage_update_srv_time(int time);

void divoom_net_http_manage_update_ip_host(char *ip_host);

void divoom_net_http_manage_update_device_id(int device_id);

void divoom_net_http_manage_update_accesstoken(uint32 userid, uint32 accesstoken);

void *divoom_net_http_manage_send(Pdivoom_net_command pcmd_info);
/* Guard context remains owned by the synchronous caller. */
void *divoom_net_http_manage_send_guarded(Pdivoom_net_command pcmd_info, int (*allow)(void *, int), void *user);

void divoom_net_http_manage_init(int device_id);

void divoom_net_http_manage_deinit(void);

/* 仅识别 Musixmatch API 主机，不包含版权追踪；不输出 URL 或授权参数。 */
int divoom_net_http_is_musixmatch_api(const char *url);
/* 返回计数单位：search=1 按实际返回曲目数；其它接口 200/404 计一次。proxy=1 解析代拉内层响应。 */
int divoom_net_http_music_response_count(const char *json, int proxy, int search);
/* 符合统计条件的 API 调用异步通知一次；通知失败不重试。 */
int divoom_net_http_notify_music_request(void);
/* 按计数单位分别入队通知，失败停止，不重试，不影响歌词。 */
void divoom_net_http_notify_music_requests(int count);
/* 每首歌首次实际发送 track.search 后异步上报一次，分页不重复；不重试。 */
int divoom_net_http_notify_music_record(const char *title, const char *artist, int duration);

#endif





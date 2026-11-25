#ifndef _divoom_ai_music_comment_h_
#define _divoom_ai_music_comment_h_

#include "divoom_type.h"

/* 音乐小框短评总缓存固定为 256 字节，包含字符串结尾。 */
#define DIVOOM_AI_MUSIC_COMMENT_MAX_LEN 256
#define DIVOOM_AI_MUSIC_COMMENT_META_LEN 192

/* AI 表盘进入/退出时绑定会话；停止不会等待正在进行的网络请求。 */
int divoom_ai_music_comment_start(uint32 session_id);
void divoom_ai_music_comment_stop(uint32 session_id);

/*
 * 提交歌手短评请求。同一时刻最多一个请求在途；阻塞期间新歌手覆盖
 * 唯一待办槽，因此网络恢复后只继续处理最后一个歌手。
 */
int divoom_ai_music_comment_request(uint32 session_id, int role_id,
    const char *singer, uint16 max_chars);

/* 由 AI 表盘现有 LVGL 定时节拍调用；正式环境不执行网络 I/O。 */
void divoom_ai_music_comment_update(void);

/* 仅复制与当前会话、角色和歌手完全匹配的新结果，成功后消费该结果。 */
int divoom_ai_music_comment_take_result(uint32 session_id, int role_id,
    const char *singer, char *text, int text_size);

/* 消费与当前歌手匹配的异步失败，使调度器结束等待且不消耗短评次数。 */
int divoom_ai_music_comment_take_failure(uint32 session_id, int role_id,
    const char *singer, int *status);

#if defined(DIVOOM_SIMULATOR)
typedef struct _divoom_ai_music_comment_whitebox_result
{
    uint32 dispatch_count;
    uint32 publish_count;
    uint32 stale_drop_count;
    uint32 pending_replace_count;
    uint8 latest_only_ok;
    uint8 max_one_in_flight_ok;
    uint8 retry_after_failure_ok;
} divoom_ai_music_comment_whitebox_result,
    *Pdivoom_ai_music_comment_whitebox_result;

/* 延迟 mock 不创建线程，用于验证“先元数据、后异步短评”和 latest-only。 */
void divoom_ai_music_comment_whitebox_set_mock(uint8 enabled,
    uint32 delay_ms, const char *response_text);
int divoom_ai_music_comment_whitebox_run(
    Pdivoom_ai_music_comment_whitebox_result result);
#endif

#endif

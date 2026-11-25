#ifndef DIVOOM_RADIO_H
#define DIVOOM_RADIO_H
#include "divoom_type.h"

/* UI只发命令和取快照。目录/缓存worker与播放worker分离；跨平台共用格式策略。
 * TOP100为显示上限，源目录最多扫描Top500；每次HTTP最多取6台。 */
#define DIVOOM_RADIO_MAX 100
#define DIVOOM_RADIO_PAGE_SIZE 6
#define DIVOOM_RADIO_NAME_MAX 192
typedef enum
{
    DIVOOM_RADIO_LOADING,
    DIVOOM_RADIO_READY,
    DIVOOM_RADIO_LOAD_FAILED
} divoom_radio_load;
typedef enum
{
    DIVOOM_RADIO_STOPPED,
    DIVOOM_RADIO_CONNECTING,
    DIVOOM_RADIO_PLAYING,
    DIVOOM_RADIO_PLAY_FAILED
} divoom_radio_play;
typedef struct
{
    uint32 id;
    char name[DIVOOM_RADIO_NAME_MAX];
    char genre[96];
    char track[192];
    char mt[48];
    char url[256];
    uint16 bitrate;
    uint8 favorite;
    uint32 recent;
    uint8 compatibility; /* 0待检测/暂时失败，1格式已确认，2明确不支持。 */
    uint32 retry_after; /* 仅暂时失败退避，单位毫秒，不写入永久否定配置。 */
} divoom_radio_station;
typedef struct
{
    /* 跨分类当前音源额外保留一槽，不计入100台显示上限。 */
    divoom_radio_station stations[DIVOOM_RADIO_MAX + 1];
    int32 count;
    int32 total;
    int32 favorite_total;
    int32 recent_total;
    uint8 loading_more;
    uint8 more_error;
    uint32 page_requests;
    uint32 scanned;
    uint32 filtered;
    uint32 probe_requests;
    uint32 cache_hits;
    int32 selected;
    divoom_radio_load load;
    divoom_radio_play play;
    uint32 revision;
    uint32 favorite_error;
    uint8 offline;
    char live_track[192];
} divoom_radio_snapshot;
/* 启动常驻worker；失败返回0，允许重试。 */
int32 divoom_radio_start(void);
/* 页面进入/恢复传1，离开/暂停传0；只投递可见性，目录worker收尾释放JSON，不停止音源。 */
void divoom_radio_set_menu_active(uint8 active);
/* 只拷贝快照；调用者不得在栈上放置大型snapshot。 */
void divoom_radio_poll(divoom_radio_snapshot *out);

/* The list keeps only navigation metadata for stations outside the viewport. */
#define DIVOOM_RADIO_VISIBLE_ROWS 4
typedef struct
{
    uint32 id;
    uint32 recent;
    uint8 favorite;
} divoom_radio_station_index;

typedef struct
{
    divoom_radio_station_index stations[DIVOOM_RADIO_MAX + 1];
    int32 count;
    int32 total;
    int32 favorite_total;
    int32 recent_total;
    uint8 loading_more;
    uint8 more_error;
    uint32 page_requests;
    int32 selected;
    divoom_radio_load load;
    divoom_radio_play play;
    uint32 revision;
    uint32 favorite_error;
    uint8 offline;
    char selected_name[DIVOOM_RADIO_NAME_MAX];
    char live_track[192];
} divoom_radio_view;

typedef struct
{
    char name[DIVOOM_RADIO_NAME_MAX];
    char genre[96];
    char track[192];
    uint16 bitrate;
} divoom_radio_row;

/* Skip the copy on an unchanged revision unless a new viewport is requested. */
uint8 divoom_radio_poll_view(divoom_radio_view *out, uint32 revision, uint8 force);
/* Reject stale indices when the worker changes the catalog between the two reads. */
uint8 divoom_radio_copy_rows(uint32 revision, const int32 *indices, int32 count, divoom_radio_row *out);
void divoom_radio_hint_id(uint32 id);
/* 锁内只复制当前电台名称，无网络、JSON 或大型快照；无选台时清空并返回0。 */
char* divoom_radio_copy_selected_name(void);
/* 选择目录下标；-1停止并取消旧代请求。 */
void divoom_radio_select(int32 index);
/* UI使用稳定id，避免后台过滤搬移后用旧下标选错电台。0表示停止。 */
void divoom_radio_select_id(uint32 id);
/* 列表ENTER：当前电台暂停/恢复，其它电台按当前浏览来源建立播放队列。 */
void divoom_radio_activate_id(uint32 id);
/* 播放专键：只投递实际选台的播放/暂停意图，不在按键线程调用UART。 */
void divoom_radio_toggle_play(uint32 fallback_id);
/* start后显式HTTP(S)地址进入统一音源；可选名称来自用户数据，异步播放，失败返回0。 */
int32 divoom_radio_play_url(const char *url, const char *name);
/* 统一音源调度调用，暂停保留选台，恢复重新连接直播；不在调用线程等待网络。 */
void divoom_radio_set_play(uint8 playing);
/* 系统切入音源时异步恢复最后成功播放的电台；新选台请求优先，禁止按键内执行I/O。 */
void divoom_radio_resume(void);
/* 仅music→radio锁序调用：按确切代号提交恢复目标/出声许可，包括首次启动的0代号。 */
uint8 divoom_radio_prepare_resume(uint8 allow_output, int32 generation);
int32 divoom_radio_get_request_generation(void);
/* 调度暂停可保留切源目标，显式退出仍通过set_play撤销全部意图。 */
void divoom_radio_set_voice_output(uint8 playing);
/* 控制提交时验证worker取出请求的代号；不分配内存或执行I/O。 */
uint8 divoom_radio_request_current(int32 generation);
/* 独立于页面的上下台；direction >0下一台，<0上一台。 */
void divoom_radio_step(int32 direction);
/* 正在连接或播放的只读意图，供统一播放/暂停键使用。 */
uint8 divoom_radio_is_playing(void);
/* 只读短锁快照：PLAYING 仅在 PCM 提交后确认，连接意图仍由 is_playing 查询。 */
divoom_radio_play divoom_radio_get_play_state(void);
/* 显式起播或换台等待；首批 PCM 成功后的自动重缓冲不算起播。 */
uint8 divoom_radio_is_starting(uint32 *request);
/* 收藏异步原子落盘；失败计入favorite_error。 */
void divoom_radio_favorite(int32 index);
void divoom_radio_favorite_id(uint32 id);
/* 失败后重试本段，保留已加载内容与原始源游标。 */
void divoom_radio_reload(void);
/* 再补足最多6台，跳过不支持项并继续原始目录分页。 */
void divoom_radio_more(void);
/* 当前焦点提示：缓存未命中时由后台检测，UI线程不联网。-1取消提示。 */
void divoom_radio_hint(int32 index);
#endif

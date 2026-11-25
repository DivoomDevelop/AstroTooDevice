/* 电台目录与兼容性服务：6条源分页、按需补足TOP100、持久化格式结果。
 * invariant：源游标与可显示数量独立；未知/网络失败永不写成不支持。
 * cache仅目录worker读写；播放以id+generation关联，过滤搬移不会写错台。
 * 所有网络/文件操作在worker执行，LVGL仅短锁读快照。 */
#include "divoom.h"
#include "divoom_radio.h"
#include "divoom_radio_browse.h"
#include "divoom_device_json.h"
#include "divoom_net_app.h"
#include "divoom_net_command.h"
#include "divoom_port_fs.h"
#if defined(_WIN32)
#include <curl/curl.h>
#else
#include "divoom_http_client.h"
#endif
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(_WIN32)
#include <windows.h>
#include "sim_ai_voice_audio.h"
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/channel_layout.h>
#else
#include "divoom_net_audio.h"
#include <console.h>
#endif

/* 策略变更必须换版本，旧配置不能越过新增的能力约束。 */
#define DIVOOM_RADIO_POLICY "http-mp3-adts-aac-hls-v2"
#define DIVOOM_RADIO_LEGACY_POLICY "http-mp3-adts-aac-v1"
#define DIVOOM_RADIO_SOURCE_MAX 500
#if defined(_WIN32) || defined(CONFIG_DIVOOM_MPP_NET_AUDIO)
#define DIVOOM_RADIO_MUSIC_SOURCE 1
#else
#define DIVOOM_RADIO_MUSIC_SOURCE 0
#endif
#define DIVOOM_RADIO_RETRY_MS 60000u
#define DIVOOM_RADIO_IO_MS 15000u
/* 真机Cdx自身单次探测15秒；外层20秒留出其重试状态切换的余量，按尝试刷新。 */
#define DIVOOM_RADIO_DEVICE_CONNECT_MS 20000u
/* RTOS pthread_attr内部仅uint16保存字节数；128KiB会截断为0。目录和调度均用堆缓冲，32KiB足够；PC解码仍保留128KiB。 */
#if defined(_WIN32)
#define DIVOOM_RADIO_WORKER_STACK (128 * 1024)
#else
#define DIVOOM_RADIO_WORKER_STACK (32 * 1024)
#endif
typedef struct
{
    uint32 magic;
    uint32 ids[DIVOOM_RADIO_MAX];
    uint32 recent[DIVOOM_RADIO_MAX];
    /* 保留旧版收藏/最近记录前缀；尾部仅保存最后成功出声的电台，后台恢复不依赖JSON。 */
    uint32 last_magic;
    divoom_radio_station last_station;
} divoom_radio_store;
/* 紧凑兼容记录：data依次为MIME、播放URL，以及status字节+线路URL。
 * 不含JSON节点；目录worker独占，已确认变化在原子落盘成功前保留dirty。 */
typedef struct
{
    uint32 id;
    uint32 bytes;
    uint32 route_count;
    uint8 status;
    uint8 dirty;
    char data[];
} divoom_radio_compat;

/* 播放队列上限100个ID；仅分页事务临时申请6台记录，不驻留第二份完整目录。 */
typedef struct
{
    divoom_radio_browse_query query;
    uint32 browse_revision;
    uint32 ids[DIVOOM_RADIO_MAX];
    int32 count;
    int32 position;
    uint32 epoch;
    int32 steps;
    int32 step_generation;
    uint32 step_serial;
    int32 retry_cursor; /* 元数据失败只推进本轮扫描，下一次轮询再查。 */
    int32 step_origin; /* 本轮上下台起点，绕回后结束。 */
    int8 retry_direction; /* 当前候选首次出声前失败时继续前进的方向。 */
    uint8 retry_queued; /* 自动继续待提交时，新按键接管方向。 */
    uint8 wait_for_commit; /* 已有起播时，实际换台才更新提示代号。 */
    uint32 offset;
    int32 total;
    uint8 exhausted;
    uint8 domestic_loaded;
} divoom_radio_play_queue;

/* 同一个解析器服务页面和后台队列，各自持有源游标。 */
typedef struct
{
    divoom_radio_browse_query query;
    divoom_radio_station *stations;
    uint32 offset;
    int32 total;
    int32 count;
    uint8 exhausted;
    uint8 domestic_loaded;
    uint8 browsing;
    uint8 retry; /* 部分成功时保留失败游标，下次显式重试。 */
    uint8 *restore_budget;
    uint32 revision;
} divoom_radio_page;

#define DIVOOM_RADIO_DIRECTORY_PAGES ((DIVOOM_RADIO_MAX + DIVOOM_RADIO_PAGE_SIZE - 1) / DIVOOM_RADIO_PAGE_SIZE)

typedef struct
{
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
} divoom_radio_state;

typedef struct
{
    void *lock;
    void *task;
    void *player_task;
    divoom_radio_state state;
    /* 第0槽保留当前台；其余槽按页申请，后台暂停后仍可恢复原台。 */
    divoom_radio_station first_station;
    divoom_radio_station *directory[DIVOOM_RADIO_DIRECTORY_PAGES];
    char selected_name[DIVOOM_RADIO_NAME_MAX];
    volatile int32 initializing; /* MODE和电台页面共用一次初始化。 */
    volatile int32 generation;
    int32 active_generation;
    uint32 startup_request;   /* 用户操作代号，跨目录等待和实际连接保持不变。 */
    int32 started_generation; /* 完成首批 PCM 的请求代号，重缓冲时保持不变。 */
    uint32 active_id;
    uint32 deadline;
    uint32 request_id;
    uint8 resume_last; /* 切源目标与出声许可分开；抢占期间保留最后成功电台的恢复意图。 */
    uint8 resume_pending; /* 切入音源只投递恢复意图；配置/目录I/O由目录worker完成。 */
    uint8 source_request; /* 1选台激活音源，2用户停止；仅播放器worker消费。 */
    uint32 favorite_id;
    uint32 hint_id;
    uint32 manual_id; /* 单个显式URL音源；来自用户/CLI，不在固件内置电台。 */
    uint32 recent_clock;
    uint32 source_offset;
    int32 source_total;
    int32 target_count;
    divoom_radio_station *pending; /* 目录worker独占，指针在锁内发布给只读诊断。 */
    int32 pending_pos;
    int32 pending_count;
    uint8 exhausted;
    uint8 reload;
    uint8 more;
    uint8 background_more; /* 后台切台允许补目录，但不得加载完整兼容JSON。 */
    uint8 store_dirty;
    uint8 store_from_backup; /* 从有效备份恢复后，修复提交成功前禁止覆盖该备份。 */
    uint8 catalog_dirty;
    char playing_url[1024];
    uint32 persist_retry;
    uint8 cache_dirty;
    uint8 cache_from_disk;
    uint8 cache_from_backup; /* 修复坏主文件时，提交成功前保留这份有效备份。 */
    divoom_radio_store saved;
    divoom_cJSON *cache;
    void *compat_records;
    uint8 compat_loaded;
    uint8 compat_dirty;
    uint8 cache_apply_failed;
    uint8 cache_migrated;
    uint8 cache_read_failed; /* I/O/申请失败与坏格式分开，禁止OOM把有效配置重置为空。 */
    uint32 compat_version;
    uint32 cache_pending_version;
    char *cache_pending_text; /* 仅落盘失败时保留序列化结果重试，禁止保留整棵JSON树。 */
    uint8 menu_active; /* 页面线程只发布可见性，树的生命周期始终由目录worker独占。 */
    uint8 cache_live;
    uint32 cache_loads;
    uint32 cache_frees;
    divoom_radio_browse_query browse_desired;
    divoom_radio_browse_query browse_active;
    uint32 browse_recent_ids[DIVOOM_RADIO_MAX]; /* 本次最近页面快照，仅400字节；点播及后续分页不重排。 */
    uint32 browse_requested;
    uint32 browse_applied;
    uint32 browse_pinned;
    uint8 browse_domestic_loaded;
    uint8 restore_budget; /* 单次目录事务最多补齐6台旧记录。 */
    uint8 pending_restore_retry;
    uint8 browse_class_request;
    divoom_radio_load browse_class_load;
    divoom_device_radio_classify_list *browse_classes;
    void *browse_genres;
    uint8 browse_genre_request;
    divoom_radio_load browse_genre_load;
    uint8 browse_initial_checked;
    uint8 browse_config_dirty;
    uint32 browse_config_retry;
    divoom_radio_browse_query browse_config_query;
    divoom_radio_play_queue play_queue;
} divoom_radio_ctrl;
static divoom_radio_ctrl g_divoom_radio;

static void divoom_radio_cache_load(void);
static void divoom_radio_cache_release(void);
static void divoom_radio_more_request(uint8 background);

/* 不持radio锁执行文件/网络操作；页面退出只撤销目录意图，不干扰播放器generation。 */
static uint8 divoom_radio_menu_active(void)
{
    uint8 active;
    divoom_system_get_lock(g_divoom_radio.lock);
    active = g_divoom_radio.menu_active;
    divoom_system_free_lock(g_divoom_radio.lock);
    return active;
}

static const char *divoom_radio_env(const char *key)
{
#if defined(_WIN32)
    return getenv(key);
#else
    (void)key;
    return NULL;
#endif
}

/* 锁在任务创建成功后才一次性发布，此后与电台模块同生命周期。 */
static uint8 divoom_radio_has_lock(void)
{
    return __atomic_load_n(&g_divoom_radio.lock, __ATOMIC_ACQUIRE) != NULL;
}

static int32 divoom_radio_generation(void)
{
#if defined(_WIN32)
    return InterlockedCompareExchange((volatile LONG *)&g_divoom_radio.generation, 0, 0);
#else
    return __atomic_load_n(&g_divoom_radio.generation, __ATOMIC_ACQUIRE);
#endif
}

static void divoom_radio_next_generation(void)
{
#if defined(_WIN32)
    InterlockedIncrement((volatile LONG *)&g_divoom_radio.generation);
#else
    __atomic_add_fetch(&g_divoom_radio.generation, 1, __ATOMIC_RELEASE);
#endif
}

/* 目录下标与页指针均由radio锁保护。 */
static divoom_radio_station *divoom_radio_station_at(int32 index)
{
    if (index == 0)
    {
        return &g_divoom_radio.first_station;
    }
    index--;
    return &g_divoom_radio.directory[index / DIVOOM_RADIO_PAGE_SIZE][index % DIVOOM_RADIO_PAGE_SIZE];
}

static int32 divoom_radio_directory_append(void)
{
    int32 index = g_divoom_radio.state.count;
    if (index > DIVOOM_RADIO_MAX)
    {
        return -1;
    }
    if (index > 0)
    {
        divoom_radio_station **page = &g_divoom_radio.directory[(index - 1) / DIVOOM_RADIO_PAGE_SIZE];
        if (!*page)
        {
            *page = divoom_system_malloc(DIVOOM_RADIO_PAGE_SIZE * sizeof(**page));
            if (!*page)
            {
                return -1;
            }
        }
    }
    memset(divoom_radio_station_at(index), 0, sizeof(divoom_radio_station));
    g_divoom_radio.state.count++;
    return index;
}

static void divoom_radio_directory_trim(void)
{
    int32 rows = g_divoom_radio.state.count;
    int32 pages = rows > 0 ? (rows + DIVOOM_RADIO_PAGE_SIZE - 2) / DIVOOM_RADIO_PAGE_SIZE : 0;
    for (int32 i = pages; i < DIVOOM_RADIO_DIRECTORY_PAGES; i++)
    {
        if (g_divoom_radio.directory[i])
        {
            divoom_system_free(g_divoom_radio.directory[i]);
            g_divoom_radio.directory[i] = NULL;
        }
    }
}

static void divoom_radio_pending_release(void)
{
    if (g_divoom_radio.pending)
    {
        divoom_system_free(g_divoom_radio.pending);
        g_divoom_radio.pending = NULL;
    }
    g_divoom_radio.pending_count = 0;
    g_divoom_radio.pending_pos = 0;
}

static divoom_radio_station *divoom_radio_pending_acquire(void)
{
    if (!g_divoom_radio.pending)
    {
        divoom_radio_station *page = divoom_system_malloc(DIVOOM_RADIO_PAGE_SIZE * sizeof(*page));
        divoom_system_get_lock(g_divoom_radio.lock);
        g_divoom_radio.pending = page;
        divoom_system_free_lock(g_divoom_radio.lock);
    }
    return g_divoom_radio.pending;
}

static void divoom_radio_directory_keep_selected(void)
{
    int32 selected = g_divoom_radio.state.selected;
    g_divoom_radio.browse_pinned = 0;
    if (selected >= 0 && selected < g_divoom_radio.state.count)
    {
        g_divoom_radio.first_station = *divoom_radio_station_at(selected);
        g_divoom_radio.browse_pinned = g_divoom_radio.first_station.id;
    }
    g_divoom_radio.state.count = g_divoom_radio.browse_pinned != 0;
    g_divoom_radio.state.selected = g_divoom_radio.browse_pinned ? 0 : -1;
    divoom_radio_directory_trim();
    divoom_radio_pending_release();
}

static int32 divoom_radio_find(uint32 id)
{
    int32 i;
    for (i = 0; i < g_divoom_radio.state.count; i++)
    {
        if (divoom_radio_station_at(i)->id == id)
        {
            return i;
        }
    }
    return -1;
}

static void divoom_radio_path(char *out, size_t size, uint8 cache, uint8 temp)
{
    const char *override = divoom_radio_env("DIVOOM_SIM_RADIO_STORE");
    const char *custom = divoom_radio_env("DIVOOM_SIM_RADIO_CONFIG");
    if (cache && custom && custom[0])
    {
        snprintf(out, size, "%s%s", custom, temp ? ".tmp" : "");
    }
    else
    {
        snprintf(out, size, "%s%s%s", override && override[0] ? override :
            DIVOOM_USER_ROOT "/net_radio_top100.dat", cache ? ".compat.json" : "", temp ? ".tmp" : "");
    }
}

/* temp写完并sync后替换；启动时校验主/备配置，双坏则按需重新检测。 */
static int32 divoom_radio_write(const void *data, size_t size, uint8 cache)
{
    char path[1024], temp[1024];
    void *file;
    int32 ok;
    uint8 from_backup = cache ? g_divoom_radio.cache_from_backup : g_divoom_radio.store_from_backup;
    divoom_radio_path(path, sizeof(path), cache, 0);
    divoom_radio_path(temp, sizeof(temp), cache, 1);
    file = divoom_port_file_open(temp, "wb");
    if (!file)
    {
        return 0;
    }
    ok = divoom_port_file_write(data, 1, size, file) == size;
    if (ok && divoom_port_file_sync(file) != 0)
    {
        ok = 0;
    }
    if (divoom_port_file_close(file) != 0)
    {
        ok = 0;
    }
#if defined(_WIN32)
    if (ok)
    {
        char resolved[2048];
        snprintf(resolved, sizeof(resolved), "%s", divoom_port_path_resolve(temp));
        ok = MoveFileExA(resolved, divoom_port_path_resolve(path), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
    }
#else
    if (ok)
    {
        /* FatFs rename不覆盖已有目标，保留bak直到新文件提交成功。 */
        char backup[1032];
        snprintf(backup, sizeof(backup), "%s.bak", path);
        if (from_backup)
        {
            /* 不能把损坏的主文件覆盖到刚验证过的有效备份上。 */
            if (divoom_port_file_exists(path) && divoom_port_remove_file(path) != 0)
            {
                ok = 0;
            }
        }
        else
        {
            divoom_port_remove_file(backup);
            if (divoom_port_file_exists(path) && divoom_port_rename_file(path, backup) != 0)
            {
                ok = 0;
            }
        }
        if (ok && divoom_port_rename_file(temp, path) != 0)
        {
            if (!from_backup)
            {
                divoom_port_rename_file(backup, path);
            }
            ok = 0;
        }
    }
#endif
    if (ok && cache)
    {
        g_divoom_radio.cache_from_backup = 0;
    }
    if (ok && !cache)
    {
        g_divoom_radio.store_from_backup = 0;
    }
    if (!ok)
    {
        divoom_port_remove_file(temp);
    }
    return ok;
}

static char *divoom_radio_read_checked(const char *path, uint8 *failed)
{
    void *file = divoom_port_file_open(path, "rb");
    char *text = NULL;
    long size;
    *failed = 0;
    if (!file)
    {
        *failed = divoom_port_file_exists(path) != 0;
        return NULL;
    }
    divoom_port_file_seek(file, 0, DIVOOM_PORT_SEEK_END);
    size = divoom_port_file_tell(file);
    divoom_port_file_seek(file, 0, DIVOOM_PORT_SEEK_SET);
    if (size >= 0 && size < 1024 * 1024)
    {
        text = divoom_system_malloc((size_t)size + 1);
        *failed = text == NULL;
        if (text && divoom_port_file_read(text, 1, size, file) != (size_t)size)
        {
            divoom_system_free(text);
            text = NULL;
            *failed = 1;
        }
    }
    divoom_port_file_close(file);
    return text;
}

static char *divoom_radio_read(const char *path)
{
    uint8 failed;
    return divoom_radio_read_checked(path, &failed);
}

static const char *divoom_radio_string(divoom_cJSON *item, const char *key)
{
    divoom_cJSON *value = item ? divoom_cJSON_GetObjectItem(item, key) : NULL;
    return value && value->valuestring ? value->valuestring : "";
}

static int32 divoom_radio_number(divoom_cJSON *item, const char *key)
{
    divoom_cJSON *value = item ? divoom_cJSON_GetObjectItem(item, key) : NULL;
    return value ? value->valueint : 0;
}

static divoom_cJSON *divoom_radio_records(void)
{
    return divoom_cJSON_GetObjectItem(g_divoom_radio.cache, "stations");
}

static divoom_cJSON *divoom_radio_cached(uint32 id)
{
    divoom_cJSON *item = divoom_radio_records();
    item = item ? item->child : NULL;
    while (item)
    {
        if ((uint32)strtoul(divoom_radio_string(item, "id"), NULL, 10) == id)
        {
            return item;
        }
        item = item->next;
    }
    return NULL;
}

/* 紧凑队列只由目录worker访问；不持队列锁调用radio或播放器。 */
static divoom_radio_compat *divoom_radio_compat_find(void *queue, uint32 id)
{
    int32 count = divoom_queue_items(queue);
    for (int32 i = 0; i < count; i++)
    {
        divoom_radio_compat *record = divoom_queue_get_index_item(queue, i);
        if (record->id == id)
        {
            return record;
        }
    }
    return NULL;
}

static const char *divoom_radio_compat_url(const divoom_radio_compat *record)
{
    return record->data + strlen(record->data) + 1;
}

static const char *divoom_radio_compat_routes(const divoom_radio_compat *record)
{
    const char *url = divoom_radio_compat_url(record);
    return url + strlen(url) + 1;
}

static int32 divoom_radio_route_present(divoom_cJSON *routes, const char *url)
{
    divoom_cJSON *route = routes ? routes->child : NULL;
    while (route)
    {
        if (strcmp(divoom_radio_string(route, "url"), url) == 0)
        {
            return 1;
        }
        route = route->next;
    }
    return 0;
}

/* 先申请并完整复制新记录，再替换旧记录；失败时旧结论不变，调用者不得发布新结论。 */
static int32 divoom_radio_compat_put(void *queue, uint32 id, const char *mt, uint8 status,
    const char *url, divoom_cJSON *routes, uint8 dirty)
{
    divoom_radio_compat *old = divoom_radio_compat_find(queue, id);
    const divoom_radio_compat *previous = old && strcmp(old->data, mt) == 0 ? old : NULL;
    const char *cursor;
    divoom_cJSON *route;
    size_t bytes = strlen(mt) + strlen(url) + 2;
    uint32 count = 0;
    route = routes ? routes->child : NULL;
    while (route)
    {
        bytes += strlen(divoom_radio_string(route, "url")) + 2;
        count++;
        route = route->next;
    }
    cursor = previous ? divoom_radio_compat_routes(previous) : NULL;
    for (uint32 i = 0; previous && i < previous->route_count; i++)
    {
        if (!divoom_radio_route_present(routes, cursor + 1))
        {
            bytes += strlen(cursor + 1) + 2;
            count++;
        }
        cursor += strlen(cursor + 1) + 2;
    }
    /* 与配置文件读取上限一致，拒绝畸形巨型记录，单位字节。 */
    if (bytes > 1024 * 1024)
    {
        return 0;
    }
    divoom_radio_compat *record = divoom_system_malloc(sizeof(*record) + bytes);
    if (!record)
    {
        return 0;
    }
    record->id = id;
    record->bytes = (uint32)(sizeof(*record) + bytes);
    record->status = status <= 2 ? status : 0;
    record->dirty = dirty;
    record->route_count = count;
    char *write = record->data;
    memcpy(write, mt, strlen(mt) + 1);
    write += strlen(mt) + 1;
    memcpy(write, url, strlen(url) + 1);
    write += strlen(url) + 1;
    route = routes ? routes->child : NULL;
    while (route)
    {
        const char *route_url = divoom_radio_string(route, "url");
        int32 route_status = divoom_radio_number(route, "status");
        *write++ = route_status >= 0 && route_status <= 2 ? (uint8)route_status : 0;
        memcpy(write, route_url, strlen(route_url) + 1);
        write += strlen(route_url) + 1;
        route = route->next;
    }
    cursor = previous ? divoom_radio_compat_routes(previous) : NULL;
    for (uint32 i = 0; previous && i < previous->route_count; i++)
    {
        size_t length = strlen(cursor + 1) + 2;
        if (!divoom_radio_route_present(routes, cursor + 1))
        {
            memcpy(write, cursor, length);
            write += length;
        }
        cursor += length;
    }
    /* push先分配队列节点；成功之后移除旧记录，避免OOM造成替换丢失。 */
    if (!divoom_queue_push(queue, record))
    {
        divoom_system_free(record);
        return 0;
    }
    if (old)
    {
        divoom_queue_pop_item(queue, old);
        divoom_system_free(old);
    }
    return 1;
}

/* 字段和值均校验申请结果；保持普通key所有权，兼容现有cJSON的深复制实现。 */
static int32 divoom_radio_json_field(divoom_cJSON *object, const char *key, divoom_cJSON *value)
{
    if (!value)
    {
        return 0;
    }
    divoom_cJSON_AddItemToObject(object, key, value);
    return value->string != NULL;
}

static int32 divoom_radio_cache_apply(const divoom_radio_compat *record)
{
    divoom_cJSON *array = divoom_radio_records();
    divoom_cJSON *item = divoom_cJSON_CreateObject();
    divoom_cJSON *routes = NULL;
    char id[24];
    if (!array || !item)
    {
        goto failed;
    }
    snprintf(id, sizeof(id), "%u", record->id);
    if (!divoom_radio_json_field(item, "id", divoom_cJSON_CreateString(id)) ||
        !divoom_radio_json_field(item, "mt", divoom_cJSON_CreateString(record->data)) ||
        !divoom_radio_json_field(item, "status", divoom_cJSON_CreateNumber(record->status)) ||
        !divoom_radio_json_field(item, "url", divoom_cJSON_CreateString(divoom_radio_compat_url(record))))
    {
        goto failed;
    }
    if (record->route_count)
    {
        const char *cursor = divoom_radio_compat_routes(record);
        routes = divoom_cJSON_CreateArray();
        if (!routes)
        {
            goto failed;
        }
        for (uint32 i = 0; i < record->route_count; i++)
        {
            divoom_cJSON *route = divoom_cJSON_CreateObject();
            if (!route || !divoom_radio_json_field(route, "url", divoom_cJSON_CreateString(cursor + 1)) ||
                !divoom_radio_json_field(route, "status", divoom_cJSON_CreateNumber((uint8)*cursor)))
            {
                divoom_cJSON_Delete(route);
                goto failed;
            }
            divoom_cJSON_AddItemToArray(routes, route);
            cursor += strlen(cursor + 1) + 2;
        }
        divoom_cJSON *owned_routes = routes;
        routes = NULL;
        if (!divoom_radio_json_field(item, "routes", owned_routes))
        {
            goto failed;
        }
    }
    /* 新节点完整后才删除旧节点；任何申请失败都不能把半份配置写回磁盘。 */
    divoom_cJSON *old = divoom_radio_cached(record->id);
    for (int32 i = 0; old && i < divoom_cJSON_GetArraySize(array); i++)
    {
        if (divoom_cJSON_GetArrayItem(array, i) == old)
        {
            divoom_cJSON_DeleteItemFromArray(array, i);
            break;
        }
    }
    divoom_cJSON_AddItemToArray(array, item);
    g_divoom_radio.cache_dirty = 1;
    return 1;
failed:
    divoom_cJSON_Delete(routes);
    divoom_cJSON_Delete(item);
    g_divoom_radio.cache_apply_failed = 1;
    return 0;
}

/* 全量导入必须成功后才发布索引；失败保留现有队列，禁止用不完整结果过滤。 */
static int32 divoom_radio_compat_import(void)
{
    void *queue = divoom_queue_init(65535, __FILE__, __LINE__);
    divoom_cJSON *item = divoom_radio_records();
    item = item ? item->child : NULL;
    if (!queue)
    {
        return 0;
    }
    while (item)
    {
        uint32 id = (uint32)strtoul(divoom_radio_string(item, "id"), NULL, 10);
        if (id && !divoom_radio_compat_put(queue, id, divoom_radio_string(item, "mt"),
            (uint8)divoom_radio_number(item, "status"), divoom_radio_string(item, "url"),
            divoom_cJSON_GetObjectItem(item, "routes"), 0))
        {
            divoom_queue_destroy(queue, NULL);
            return 0;
        }
        item = item->next;
    }
    divoom_queue_destroy(g_divoom_radio.compat_records, NULL);
    g_divoom_radio.compat_records = queue;
    g_divoom_radio.compat_loaded = 1;
    return 1;
}

static void divoom_radio_compat_saved(void)
{
    int32 count = divoom_queue_items(g_divoom_radio.compat_records);
    for (int32 i = 0; i < count; i++)
    {
        divoom_radio_compat *record = divoom_queue_get_index_item(g_divoom_radio.compat_records, i);
        record->dirty = 0;
    }
    g_divoom_radio.compat_dirty = 0;
}

/* 先保留二进制结果，再允许调用者发布状态；JSON失败/页面退出不会丢失确认及routes。 */
static int32 divoom_radio_cache_put(const divoom_radio_station *station, divoom_cJSON *routes)
{
    if (!g_divoom_radio.compat_records)
    {
        g_divoom_radio.compat_records = divoom_queue_init(65535, __FILE__, __LINE__);
    }
    int32 ok = g_divoom_radio.compat_records && divoom_radio_compat_put(g_divoom_radio.compat_records,
        station->id, station->mt, station->compatibility, station->url, routes, 1);
    divoom_cJSON_Delete(routes);
    if (!ok)
    {
        return 0;
    }
    g_divoom_radio.compat_dirty = 1;
    g_divoom_radio.compat_version++;
    if (g_divoom_radio.cache)
    {
        divoom_radio_cache_apply(divoom_radio_compat_find(g_divoom_radio.compat_records, station->id));
    }
    return 1;
}

/* MIME只初筛；空/未知标记保留，避免把信息缺失误判成不可用。 */
static int32 divoom_radio_mime_rejected(const char *mt)
{
    static const char *types[] = { "audio/ogg", "application/ogg", "audio/opus", "audio/flac",
        "audio/x-flac", "audio/x-ms-wma", "video/x-ms-asf", "audio/wav", "audio/x-wav" };
    for (int32 i = 0; i < (int32)(sizeof(types) / sizeof(types[0])); i++)
    {
        if (strcmp(mt, types[i]) == 0)
        {
            return 1;
        }
    }
    return 0;
}

typedef struct
{
    char *data;
    size_t used;
    size_t capacity;
} divoom_radio_buffer;

static size_t divoom_radio_receive(void *data, size_t size, size_t count, void *user)
{
    divoom_radio_buffer *buffer = user;
    size_t bytes = size * count;
    size_t copy = bytes;
    if (copy > buffer->capacity - buffer->used - 1)
    {
        copy = buffer->capacity - buffer->used - 1;
    }
    memcpy(buffer->data + buffer->used, data, copy);
    buffer->used += copy;
    buffer->data[buffer->used] = '\0';
    return copy == bytes ? bytes : 0;
}

static int32 divoom_radio_interrupted(void *unused)
{
    (void)unused;
    return divoom_radio_generation() != g_divoom_radio.active_generation ||
        (int32)(divoom_system_get_tickms() - g_divoom_radio.deadline) >= 0;
}

/* 后台队列补元数据独立于页面和播放器；只接受发起时的队列/切源令牌。 */
enum
{
    DIVOOM_RADIO_HTTP_MENU = 0,
    DIVOOM_RADIO_HTTP_PLAYER = 1,
    DIVOOM_RADIO_HTTP_QUEUE = 2
};
typedef struct
{
    uint32 deadline;
    uint32 queue_epoch;
    uint32 step_serial;
    int32 generation;
    uint8 owner;
} divoom_radio_http_scope;

static void divoom_radio_http_scope_init(divoom_radio_http_scope *scope, uint8 owner, uint32 timeout)
{
    scope->owner = owner;
    scope->deadline = divoom_system_get_tickms() + (owner == DIVOOM_RADIO_HTTP_QUEUE ? DIVOOM_RADIO_IO_MS : timeout);
    if (owner == DIVOOM_RADIO_HTTP_QUEUE && divoom_radio_has_lock())
    {
        divoom_system_get_lock(g_divoom_radio.lock);
        scope->queue_epoch = g_divoom_radio.play_queue.epoch;
        scope->step_serial = g_divoom_radio.play_queue.step_serial;
        scope->generation = divoom_radio_generation();
        divoom_system_free_lock(g_divoom_radio.lock);
    }
}

static int divoom_radio_http_scope_cancel(const divoom_radio_http_scope *scope)
{
    int cancel;
    if (!scope || (int32)(divoom_system_get_tickms() - scope->deadline) >= 0)
    {
        return 1;
    }
    if (scope->owner == DIVOOM_RADIO_HTTP_PLAYER)
    {
        return divoom_radio_interrupted(NULL);
    }
    if (scope->owner == DIVOOM_RADIO_HTTP_QUEUE)
    {
        if (!divoom_radio_has_lock())
        {
            return 1;
        }
        divoom_system_get_lock(g_divoom_radio.lock);
        cancel = scope->queue_epoch != g_divoom_radio.play_queue.epoch ||
            scope->step_serial != g_divoom_radio.play_queue.step_serial ||
            scope->generation != divoom_radio_generation();
        divoom_system_free_lock(g_divoom_radio.lock);
        return cancel;
    }
    return 0;
}


#if defined(_WIN32)
static int divoom_radio_cancel_http(void *unused, curl_off_t total, curl_off_t now,
    curl_off_t upload_total, curl_off_t upload_now)
{
    (void)total;
    (void)now;
    (void)upload_total;
    (void)upload_now;
    return divoom_radio_http_scope_cancel(unused) ||
        (((divoom_radio_http_scope *)unused)->owner == DIVOOM_RADIO_HTTP_MENU && !divoom_radio_menu_active());
}

/* 仅HTTP(S)，重定向也受限；probe取到24KiB即可中止，网络错误不产生否定结果。 */
static char *divoom_radio_http(const char *url, const char *post, uint8 player, uint8 probe, size_t *received)
{
    CURL *curl = curl_easy_init();
    divoom_radio_buffer buffer = {0};
    divoom_radio_http_scope scope = {0};
    CURLcode result;
    long status = 0;
    struct curl_slist *aliases = NULL;
    divoom_radio_http_scope_init(&scope, player, probe ? 8000 : DIVOOM_RADIO_IO_MS);
    buffer.capacity = probe ? 24577 : 256 * 1024;
    buffer.data = divoom_system_malloc(buffer.capacity);
    if (!curl || !buffer.data)
    {
        goto failed;
    }
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, probe ? 8L : 15L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 6L);
    curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS);
    curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, divoom_radio_receive);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "VLC/3.0.21");
    aliases = curl_slist_append(aliases, "ICY 200 OK");
    curl_easy_setopt(curl, CURLOPT_HTTP200ALIASES, aliases);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, divoom_radio_cancel_http);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &scope);
    if (post)
    {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post);
    }
    result = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    curl_easy_cleanup(curl);
    curl = NULL;
    curl_slist_free_all(aliases);
    aliases = NULL;
    if (status == 200 && (result == CURLE_OK || (probe && buffer.used >= 4096 &&
        (result == CURLE_WRITE_ERROR || result == CURLE_OPERATION_TIMEDOUT))))
    {
        if (received)
        {
            *received = buffer.used;
        }
        return buffer.data;
    }
    divoom_print("[net_radio] HTTP failed status=%ld curl=%d\n", status, (int32)result);
failed:
    if (curl)
    {
        curl_easy_cleanup(curl);
    }
    curl_slist_free_all(aliases);
    divoom_system_free(buffer.data);
    return NULL;
}

#else
/* 真机复用RTOS HTTP传输；探测有独立截止时间，不触发全局HTTP abort。 */
typedef struct
{
    divoom_radio_buffer buffer;
    divoom_radio_http_scope scope;
} divoom_radio_request;
static int divoom_radio_request_cancel(void *user)
{
    divoom_radio_request *request = user;
    return request->buffer.used + 1 >= request->buffer.capacity ||
        divoom_radio_http_scope_cancel(&request->scope) ||
        (request->scope.owner == DIVOOM_RADIO_HTTP_MENU && !divoom_radio_menu_active());
}
static void divoom_radio_request_data(void *user, uint8 *data, int size, int total)
{
    divoom_radio_request *request = user;
    (void)total;
    if (size > 0)
    {
        divoom_radio_receive(data, 1, size, &request->buffer);
    }
}
static char *divoom_radio_http(const char *url, const char *post, uint8 player, uint8 probe, size_t *received)
{
    divoom_radio_request request = {0};
    char *text;
    int size = 0;
    if (strncmp(url, "http://", 7) && strncmp(url, "https://", 8))
    {
        return NULL;
    }
    divoom_radio_http_scope_init(&request.scope, player, probe ? 8000 : DIVOOM_RADIO_IO_MS);
    request.buffer.capacity = probe ? 24577 : 256 * 1024;
    if (post)
    {
        /* 库默认POST为JSON；SHOUTcast目录需要表单Content-Type。 */
        divoom_http_client_url_head_info *header = divoom_system_malloc(sizeof(*header));
        if (!header)
        {
            return NULL;
        }
        snprintf(header->key, sizeof(header->key), "Content-Type");
        snprintf(header->word, sizeof(header->word), "application/x-www-form-urlencoded");
        text = divoom_http_send_data((char *)url, (char *)post, 1, header, 1);
        divoom_system_free(header);
        return text;
    }
    if (!probe)
    {
        return divoom_http_client_get_file((char *)url, &size, NULL, 0,
            divoom_radio_request_cancel, &request);
    }
    request.buffer.data = divoom_system_malloc(request.buffer.capacity);
    if (!request.buffer.data)
    {
        return NULL;
    }
    divoom_http_client_get_net_tofile((char *)url, divoom_radio_request_data, &request,
        NULL, 0, divoom_radio_request_cancel, &request);
    if (request.buffer.used < 14)
    {
        divoom_system_free(request.buffer.data);
        return NULL;
    }
    if (received)
    {
        *received = request.buffer.used;
    }
    return request.buffer.data;
}
#endif

static void divoom_radio_attribute(const char *start, const char *end,
    const char *key, char *out, size_t size)
{
    char pattern[48];
    const char *value, *close;
    snprintf(pattern, sizeof(pattern), "%s=\"", key);
    value = start;
    while ((value = strstr(value, pattern)) != NULL)
    {
        if (value == start || value[-1] == ' ' || value[-1] == '\t' || value[-1] == '\n' || value[-1] == '\r')
        {
            break;
        }
        value++;
    }
    out[0] = '\0';
    if (!value || value >= end)
    {
        return;
    }
    value += strlen(pattern);
    close = strchr(value, '"');
    if (close && close <= end)
    {
        size_t length = (size_t)(close - value);
        if (length >= size)
        {
            length = size - 1;
        }
        memcpy(out, value, length);
        out[length] = '\0';
    }
    /* XML实体不直接显示给用户，保持UTF-8正文。 */
    {
        static const char *entities[] = {"&amp;", "&quot;", "&apos;", "&lt;", "&gt;"};
        static const char decoded[] = {'&', '"', '\'', '<', '>'};
        int32 i;
        for (i = 0; i < 5; i++)
        {
            char *found;
            while ((found = strstr(out, entities[i])) != NULL)
            {
                *found = decoded[i];
                memmove(found + 1, found + strlen(entities[i]), strlen(found + strlen(entities[i])) + 1);
            }
        }
    }
}



static void divoom_radio_select_locked(uint32 id, uint8 new_request);
static void divoom_radio_fail_locked(int32 generation);

/* 分类目录的磁盘记录是轻量值对象；JSON只在协议解析或兼容配置事务中使用。 */
static void divoom_radio_browse_path(char *path, size_t capacity, uint32 id, uint8 domestic)
{
    snprintf(path, capacity, DIVOOM_USER_ROOT "/radio_%s_%08x.dat", domestic ? "class" : "station", id);
}

static uint8 divoom_radio_browse_write(const char *path, const void *data, size_t size)
{
    char temp[256];
    void *file;
    uint8 ok;
    snprintf(temp, sizeof(temp), "%s.tmp", path);
    file = divoom_port_file_open(temp, "wb");
    if (!file)
    {
        return 0;
    }
    ok = divoom_port_file_write(data, 1, size, file) == size;
    if (ok && divoom_port_file_sync(file) != 0)
    {
        ok = 0;
    }
    if (divoom_port_file_close(file) != 0)
    {
        ok = 0;
    }
#if defined(_WIN32)
    if (ok)
    {
        char resolved[512];
        snprintf(resolved, sizeof(resolved), "%s", divoom_port_path_resolve(temp));
        ok = MoveFileExA(resolved, divoom_port_path_resolve(path), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
    }
#else
    if (ok)
    {
        /* 这些记录可从服务器重建；保存失败不会改成格式不支持。 */
        if (divoom_port_file_exists(path) && divoom_port_remove_file(path) != 0)
        {
            ok = 0;
        }
        if (ok)
        {
            ok = divoom_port_rename_file(temp, path) == 0;
        }
    }
#endif
    return ok;
}

static uint8 divoom_radio_browse_station_read(uint32 id, divoom_radio_station *station)
{
    char path[192];
    uint8 ok;
    void *file;
    divoom_radio_browse_path(path, sizeof(path), id, 0);
    file = divoom_port_file_open(path, "rb");
    if (!file)
    {
        return 0;
    }
    ok = divoom_port_file_read(station, sizeof(*station), 1, file) == 1;
    divoom_port_file_close(file);
    return ok && station->id == id && station->compatibility <= 2 && memchr(station->name, 0, sizeof(station->name)) &&
        memchr(station->genre, 0, sizeof(station->genre)) && memchr(station->track, 0, sizeof(station->track)) &&
        memchr(station->mt, 0, sizeof(station->mt)) && memchr(station->url, 0, sizeof(station->url)) &&
        !strpbrk(station->url, "\r\n\t ") &&
        (!station->url[0] || !strncmp(station->url, "http://", 7) || !strncmp(station->url, "https://", 8));
}

static void divoom_radio_browse_station_save(const divoom_radio_station *station)
{
    char path[192];
    divoom_radio_browse_path(path, sizeof(path), station->id, 0);
    divoom_radio_browse_write(path, station, sizeof(*station));
}

/* 国内RadioId与SHOUTcast不共用命名空间。直接URL使用稳定高位ID；检测哈希碰撞后探测空槽，
 * 同URL始终复用相同记录，绝不把国内ID误发到SHOUTcast的PLS接口。 */
static uint32 divoom_radio_browse_url_id(const char *url)
{
    uint32 id = 2166136261u;
    divoom_radio_station *old = divoom_system_malloc(sizeof(*old));
    if (!old)
    {
        return 0;
    }
    for (const uint8 *p = (const uint8 *)url; *p; p++)
    {
        id = (id ^ *p) * 16777619u;
    }
    id |= 0x80000000u;
    for (int32 attempt = 0; attempt < 256; attempt++)
    {
        if (!divoom_radio_browse_station_read(id, old) || !strcmp(old->url, url))
        {
            divoom_system_free(old);
            return id;
        }
        id = (id + 1) | 0x80000000u;
    }
    divoom_system_free(old);
    return 0;
}

void divoom_radio_browse_request(const divoom_radio_browse_query *query)
{
    if (!query || !divoom_radio_has_lock() || query->kind > DIVOOM_RADIO_BROWSE_AUTO ||
        (query->kind == DIVOOM_RADIO_BROWSE_DOMESTIC &&
        (divoom_main_get_server_type() != 0 || query->classify_id <= 0)) ||
        !memchr(query->genre, 0, sizeof(query->genre)))
    {
        return;
    }
    divoom_system_get_lock(g_divoom_radio.lock);
    if (query->kind != DIVOOM_RADIO_BROWSE_RECENT && g_divoom_radio.browse_requested &&
        !memcmp(&g_divoom_radio.browse_desired, query, sizeof(*query)))
    {
        divoom_system_free_lock(g_divoom_radio.lock);
        return;
    }
    g_divoom_radio.browse_desired = *query;
    g_divoom_radio.browse_requested++;
    /* 只发布意图，旧目录在worker下一轮提交时被替换，播放器代号保持不变。 */
    g_divoom_radio.state.revision++;
    divoom_system_free_lock(g_divoom_radio.lock);
}

void divoom_radio_browse_poll(divoom_radio_browse_state *out)
{
    memset(out, 0, sizeof(*out));
    if (!divoom_radio_has_lock())
    {
        return;
    }
    divoom_system_get_lock(g_divoom_radio.lock);
    out->requested = g_divoom_radio.browse_requested;
    out->applied = g_divoom_radio.browse_applied;
    out->pinned_id = g_divoom_radio.browse_pinned;
    out->classify_count = g_divoom_radio.browse_classes ? g_divoom_radio.browse_classes->count : 0;
    out->classify_load = g_divoom_radio.browse_class_load;
    out->genre_load = g_divoom_radio.browse_genre_load;
    out->active_query = g_divoom_radio.browse_active;
    divoom_system_free_lock(g_divoom_radio.lock);
}

void divoom_radio_browse_classify_request(void)
{
    if (divoom_radio_has_lock() && divoom_main_get_server_type() == 0)
    {
        divoom_system_get_lock(g_divoom_radio.lock);
        if (!g_divoom_radio.browse_classes && !g_divoom_radio.browse_class_request)
        {
            g_divoom_radio.browse_class_request = 1;
            g_divoom_radio.browse_class_load = DIVOOM_RADIO_LOADING;
        }
        divoom_system_free_lock(g_divoom_radio.lock);
    }
}

uint8 divoom_radio_browse_classify_copy(int32 index, int32 *id, char *name, uint32 capacity)
{
    uint8 copied = 0;
    if (!divoom_radio_has_lock() || !name || !capacity || !id)
    {
        return 0;
    }
    divoom_system_get_lock(g_divoom_radio.lock);
    if (g_divoom_radio.browse_classes && index >= 0 && index < g_divoom_radio.browse_classes->count)
    {
        *id = g_divoom_radio.browse_classes->items[index].classify_id;
        snprintf(name, capacity, "%s", g_divoom_radio.browse_classes->items[index].name);
        copied = 1;
    }
    divoom_system_free_lock(g_divoom_radio.lock);
    return copied;
}

/* 分类名称来自目录服务原文，UI不再维护翻译/固定类目表。 */
typedef struct
{
    int32 parent;
    int32 row;
    int32 count;
    divoom_radio_browse_genre *out;
} divoom_radio_genre_lookup;

static void divoom_radio_genre_visit(void *user, void *item, int position)
{
    divoom_radio_browse_genre *genre = item;
    divoom_radio_genre_lookup *lookup = user;
    (void)position;
    if (genre->parent == lookup->parent)
    {
        if (lookup->out && lookup->count == lookup->row)
        {
            *lookup->out = *genre;
        }
        lookup->count++;
    }
}

int32 divoom_radio_browse_genre_count(int32 parent)
{
    divoom_radio_genre_lookup lookup = {0};
    lookup.parent = parent;
    if (divoom_radio_has_lock())
    {
        divoom_system_get_lock(g_divoom_radio.lock);
        if (g_divoom_radio.browse_genres)
        {
            divoom_queue_get_all_items(g_divoom_radio.browse_genres, &lookup, divoom_radio_genre_visit);
        }
        divoom_system_free_lock(g_divoom_radio.lock);
    }
    return lookup.count;
}

uint8 divoom_radio_browse_genre_copy(int32 parent, int32 row, divoom_radio_browse_genre *out)
{
    divoom_radio_genre_lookup lookup = {0};
    if (!out || !divoom_radio_has_lock() || row < 0)
    {
        return 0;
    }
    lookup.parent = parent;
    lookup.row = row;
    lookup.out = out;
    divoom_system_get_lock(g_divoom_radio.lock);
    if (g_divoom_radio.browse_genres)
    {
        divoom_queue_get_all_items(g_divoom_radio.browse_genres, &lookup, divoom_radio_genre_visit);
    }
    divoom_system_free_lock(g_divoom_radio.lock);
    return row < lookup.count;
}

void divoom_radio_browse_genre_request(void)
{
    if (divoom_radio_has_lock())
    {
        divoom_system_get_lock(g_divoom_radio.lock);
        if (!g_divoom_radio.browse_genres && !g_divoom_radio.browse_genre_request)
        {
            g_divoom_radio.browse_genre_request = 1;
            g_divoom_radio.browse_genre_load = DIVOOM_RADIO_LOADING;
            g_divoom_radio.state.revision++;
        }
        divoom_system_free_lock(g_divoom_radio.lock);
    }
}

static void divoom_radio_genre_free(void *item)
{
    divoom_system_free(item);
}

/* 明确限定服务回包大小/分类数量，按实际数量动态排队，退出页面整队释放。 */
static void *divoom_radio_genre_parse(const char *text)
{
    void *queue;
    const char *cursor = text;
    int32 count = 0;
    if (!text || !strstr(text, "<statusCode>200</statusCode>") || !strstr(text, "<genrelist") || strlen(text) > 256 * 1024)
    {
        return NULL;
    }
    queue = divoom_queue_init(512, __func__, __LINE__);
    if (!queue)
    {
        return NULL;
    }
    while ((cursor = strstr(cursor, "<genre ")) != NULL)
    {
        const char *end = strchr(cursor, '>');
        char number[24], *tail;
        divoom_radio_browse_genre *genre = divoom_system_malloc(sizeof(*genre));
        uint8 valid = end && genre && count < 512;
        if (valid)
        {
            divoom_radio_attribute(cursor, end, "id", number, sizeof(number));
            long id = strtol(number, &tail, 10);
            valid = number[0] && !*tail && id > 0 && id <= 0x7fffffff;
            genre->id = (int32)id;
            divoom_radio_attribute(cursor, end, "parentid", number, sizeof(number));
            long parent = strtol(number, &tail, 10);
            valid &= number[0] && !*tail && parent >= 0 && parent <= 0x7fffffff && parent != id;
            genre->parent = (int32)parent;
            /* 多留一字节检测超长名称；不能截断名称再向服务器查询另一分类。 */
            char name[sizeof(genre->name) + 1];
            divoom_radio_attribute(cursor, end, "name", name, sizeof(name));
            valid &= name[0] && strlen(name) < sizeof(genre->name);
            if (valid)
            {
                memcpy(genre->name, name, strlen(name) + 1);
            }
        }
        if (!valid || !divoom_queue_push(queue, genre))
        {
            divoom_system_free(genre);
            divoom_queue_destroy(queue, divoom_radio_genre_free);
            return NULL;
        }
        count++;
        cursor = end + 1;
    }
    return queue;
}

/* 仅目录worker调用；所有网络/解析都在锁外，新树以短锁提交，过期页面不接收回包。 */
static void divoom_radio_genre_update(void)
{
    uint8 requested;
    void *released = NULL;
    divoom_system_get_lock(g_divoom_radio.lock);
    if (!g_divoom_radio.menu_active)
    {
        released = g_divoom_radio.browse_genres;
        g_divoom_radio.browse_genres = NULL;
        g_divoom_radio.browse_genre_request = 0;
    }
    requested = g_divoom_radio.menu_active && g_divoom_radio.browse_genre_request;
    g_divoom_radio.browse_genre_request = 0;
    divoom_system_free_lock(g_divoom_radio.lock);
    divoom_queue_destroy(released, divoom_radio_genre_free);
    if (requested)
    {
        extern const char *divoom_net_radio_directory_key(void);
        const char *key = divoom_radio_env("DIVOOM_SHOUTCAST_KEY");
        const char *fixture = divoom_radio_env("DIVOOM_SIM_RADIO_GENRES");
        char *text = NULL;
        if (fixture && fixture[0])
        {
            text = divoom_radio_read(fixture);
        }
        else
        {
            char post[320];
            if (!key || !key[0])
            {
                key = divoom_net_radio_directory_key();
            }
            if (key && key[0] && strlen(key) < 256)
            {
                snprintf(post, sizeof(post), "k=%s&f=xml", key);
                text = divoom_radio_http("http://api.shoutcast.com/genre/secondary", post, 0, 0, NULL);
                memset(post, 0, sizeof(post));
            }
        }
        void *genres = divoom_radio_genre_parse(text);
        divoom_system_free(text);
        divoom_system_get_lock(g_divoom_radio.lock);
        if (g_divoom_radio.menu_active && genres)
        {
            released = g_divoom_radio.browse_genres;
            g_divoom_radio.browse_genres = genres;
            genres = NULL;
            g_divoom_radio.browse_genre_load = DIVOOM_RADIO_READY;
        }
        else
        {
            released = NULL;
            g_divoom_radio.browse_genre_load = DIVOOM_RADIO_LOAD_FAILED;
        }
        g_divoom_radio.state.revision++;
        divoom_system_free_lock(g_divoom_radio.lock);
        divoom_queue_destroy(released, divoom_radio_genre_free);
        divoom_queue_destroy(genres, divoom_radio_genre_free);
    }
}


/* 列表配置只保存筛选条件；不保存站表/JSON，不改变正在播放的独立音源队列。 */
typedef struct
{
    uint32 magic;
    divoom_radio_browse_query query;
    uint32 checksum;
} divoom_radio_browse_config;

static void divoom_radio_browse_config_path(char *path, uint32 capacity)
{
    const char *override = divoom_radio_env("DIVOOM_SIM_RADIO_BROWSE_CONFIG");
    snprintf(path, capacity, "%s", override && override[0] ? override : DIVOOM_USER_ROOT "/net_radio_browse.dat");
}

static uint32 divoom_radio_browse_config_hash(const divoom_radio_browse_config *config)
{
    const uint8 *bytes = (const uint8 *)config;
    uint32 hash = 2166136261u;
    for (uint32 i = 0; i < sizeof(*config) - sizeof(config->checksum); i++)
    {
        hash = (hash ^ bytes[i]) * 16777619u;
    }
    return hash;
}

static uint8 divoom_radio_browse_config_read(divoom_radio_browse_query *query)
{
    char path[1024];
    divoom_radio_browse_config config;
    uint8 extra, valid;
    divoom_radio_browse_config_path(path, sizeof(path));
    void *file = divoom_port_file_open(path, "rb");
    if (!file)
    {
        return 0;
    }
    valid = divoom_port_file_read(&config, sizeof(config), 1, file) == 1 &&
        divoom_port_file_read(&extra, 1, 1, file) == 0;
    divoom_port_file_close(file);
    valid = valid && config.magic == 0x52425231 && config.query.kind <= DIVOOM_RADIO_BROWSE_DOMESTIC &&
        config.query.kind >= DIVOOM_RADIO_BROWSE_TOP && memchr(config.query.genre, 0, sizeof(config.query.genre)) &&
        config.checksum == divoom_radio_browse_config_hash(&config);
    if (valid && config.query.kind == DIVOOM_RADIO_BROWSE_GENRE)
    {
        valid = config.query.genre[0] && !strpbrk(config.query.genre, "\r\n\t");
    }
    if (valid)
    {
        /* 地区变化时不恢复另一地区目录；国内走既有央广默认项查询。 */
        uint8 domestic = config.query.kind == DIVOOM_RADIO_BROWSE_DOMESTIC;
        valid = domestic == (divoom_main_get_server_type() == 0) &&
            (!domestic || config.query.classify_id > 0);
    }
    if (valid)
    {
        *query = config.query;
    }
    return valid;
}

/* 仅worker保存，失败留dirty下次重试；不覆盖收藏/最近的旧804字节保存布局。 */
static void divoom_radio_browse_config_save(void)
{
    char path[1024];
    divoom_radio_browse_config config = {0};
    uint32 now = divoom_system_get_tickms();
    if (!g_divoom_radio.browse_config_dirty ||
        (g_divoom_radio.browse_config_retry && (int32)(now - g_divoom_radio.browse_config_retry) < 0))
    {
        return;
    }
    config.magic = 0x52425231;
    config.query = g_divoom_radio.browse_config_query;
    config.checksum = divoom_radio_browse_config_hash(&config);
    divoom_radio_browse_config_path(path, sizeof(path));
    if (divoom_radio_browse_write(path, &config, sizeof(config)))
    {
        g_divoom_radio.browse_config_dirty = 0;
        g_divoom_radio.browse_config_retry = 0;
    }
    else
    {
        g_divoom_radio.browse_config_retry = now + 1000u;
    }
}

/* 首次打开：国内由回包查“央广”的真实ID，国外TOP100。旧异步结果不得覆盖用户后来选择。 */
/* 央广的分类 ID 使用服务器回包，页面和后台沿用同一匹配规则。 */
static int32 divoom_radio_classify_cnr(const divoom_device_radio_classify_list *classes)
{
    for (int32 i = 0; classes && i < classes->count; i++)
    {
        if (!strcmp(classes->items[i].name, "央广"))
        {
            return classes->items[i].classify_id;
        }
    }
    return 0;
}

static void divoom_radio_browse_initial(void)
{
    divoom_radio_browse_query query = {0};
    uint32 requested;
    uint8 checked;
    divoom_system_get_lock(g_divoom_radio.lock);
    requested = g_divoom_radio.browse_requested;
    checked = g_divoom_radio.browse_initial_checked;
    uint8 pending = (g_divoom_radio.menu_active || g_divoom_radio.resume_pending) &&
        requested == g_divoom_radio.browse_applied &&
        g_divoom_radio.browse_active.kind == DIVOOM_RADIO_BROWSE_AUTO;
    divoom_system_free_lock(g_divoom_radio.lock);
    if (!pending)
    {
        return;
    }
    uint8 resolved = !checked && divoom_radio_browse_config_read(&query);
    if (!resolved && divoom_main_get_server_type() != 0)
    {
        query.kind = DIVOOM_RADIO_BROWSE_TOP;
        resolved = 1;
    }
    if (!resolved)
    {
        if (!checked)
        {
            divoom_system_get_lock(g_divoom_radio.lock);
            g_divoom_radio.browse_initial_checked = 1;
            divoom_system_free_lock(g_divoom_radio.lock);
            divoom_radio_browse_classify_request();
        }
        divoom_system_get_lock(g_divoom_radio.lock);
        query.classify_id = divoom_radio_classify_cnr(g_divoom_radio.browse_classes);
        if (query.classify_id > 0)
        {
            query.kind = DIVOOM_RADIO_BROWSE_DOMESTIC;
            resolved = 1;
        }
        if (!resolved && (g_divoom_radio.browse_classes || g_divoom_radio.browse_class_load == DIVOOM_RADIO_LOAD_FAILED) &&
            g_divoom_radio.state.load != DIVOOM_RADIO_LOAD_FAILED)
        {
            g_divoom_radio.state.load = DIVOOM_RADIO_LOAD_FAILED;
            g_divoom_radio.state.revision++;
        }
        divoom_system_free_lock(g_divoom_radio.lock);
    }
    if (resolved)
    {
        divoom_system_get_lock(g_divoom_radio.lock);
        if ((g_divoom_radio.menu_active || g_divoom_radio.resume_pending) &&
            g_divoom_radio.browse_requested == requested &&
            g_divoom_radio.browse_desired.kind == DIVOOM_RADIO_BROWSE_AUTO)
        {
            g_divoom_radio.browse_desired = query;
            g_divoom_radio.browse_requested++;
            g_divoom_radio.state.revision++;
        }
        divoom_system_free_lock(g_divoom_radio.lock);
    }
}


/* 目录worker是唯一文件/MQTT生产者；切类目时保留一台正在使用的音源槽，其余最多100台。 */
static void divoom_radio_browse_update(void)
{
    divoom_radio_browse_config_save();
    divoom_radio_genre_update();
    divoom_device_radio_classify_list *released = NULL;
    uint8 classes;
    divoom_system_get_lock(g_divoom_radio.lock);
    if (!g_divoom_radio.menu_active && !g_divoom_radio.resume_pending)
    {
        released = g_divoom_radio.browse_classes;
        g_divoom_radio.browse_classes = NULL;
        g_divoom_radio.browse_class_request = 0;
    }
    if (g_divoom_radio.browse_applied != g_divoom_radio.browse_requested)
    {
        divoom_radio_directory_keep_selected();
        g_divoom_radio.browse_active = g_divoom_radio.browse_desired;
        if (g_divoom_radio.browse_active.kind == DIVOOM_RADIO_BROWSE_RECENT)
        {
            /* 只在用户重新进入最近时取新顺序，历史正常更新不能移动当前页面行。 */
            memcpy(g_divoom_radio.browse_recent_ids, g_divoom_radio.saved.recent, sizeof(g_divoom_radio.browse_recent_ids));
        }
        g_divoom_radio.browse_initial_checked = 0;
        if (g_divoom_radio.browse_active.kind <= DIVOOM_RADIO_BROWSE_DOMESTIC)
        {
            g_divoom_radio.browse_config_query = g_divoom_radio.browse_active;
            g_divoom_radio.browse_config_dirty = 1;
        }
        g_divoom_radio.browse_applied = g_divoom_radio.browse_requested;
        g_divoom_radio.browse_domestic_loaded = 0;
        g_divoom_radio.source_offset = 0;
        g_divoom_radio.source_total = DIVOOM_RADIO_SOURCE_MAX;
        g_divoom_radio.exhausted = 0;
        g_divoom_radio.target_count = DIVOOM_RADIO_PAGE_SIZE + g_divoom_radio.state.count;
        g_divoom_radio.state.total = DIVOOM_RADIO_MAX + g_divoom_radio.state.count;
        g_divoom_radio.state.load = DIVOOM_RADIO_LOADING;
        g_divoom_radio.state.more_error = 0;
        g_divoom_radio.hint_id = 0;
        g_divoom_radio.reload = 1;
        g_divoom_radio.more = 0;
        g_divoom_radio.state.revision++;
    }
    classes = (g_divoom_radio.menu_active || g_divoom_radio.resume_pending) && g_divoom_radio.browse_class_request;
    g_divoom_radio.browse_class_request = 0;
    divoom_system_free_lock(g_divoom_radio.lock);
    divoom_radio_cache_release();
    divoom_system_free(released);
    if (classes && divoom_main_get_server_type() == 0)
    {
        Pdivoom_net_command reply = divoom_net_app_send_packet_wait(DIVOOM_NET_COMM_RADIO_GET_CLASSIFY, DIVOOM_SERVER_TOPIC, NULL);
        divoom_system_get_lock(g_divoom_radio.lock);
        if ((g_divoom_radio.menu_active || g_divoom_radio.resume_pending) &&
            divoom_main_get_server_type() == 0 && reply && reply->cmd_data)
        {
            released = g_divoom_radio.browse_classes;
            g_divoom_radio.browse_classes = reply->cmd_data;
            reply->cmd_data = NULL;
            g_divoom_radio.browse_class_load = DIVOOM_RADIO_READY;
        }
        else
        {
            released = NULL;
            g_divoom_radio.browse_class_load = DIVOOM_RADIO_LOAD_FAILED;
        }
        g_divoom_radio.state.revision++;
        divoom_system_free_lock(g_divoom_radio.lock);
        divoom_system_free(released);
        divoom_net_app_free_packet(reply);
    }
    divoom_radio_browse_initial();
}

static uint8 divoom_radio_browse_current(void)
{
    uint8 current;
    divoom_system_get_lock(g_divoom_radio.lock);
    current = g_divoom_radio.browse_requested == g_divoom_radio.browse_applied &&
        g_divoom_radio.browse_active.kind != DIVOOM_RADIO_BROWSE_AUTO;
    divoom_system_free_lock(g_divoom_radio.lock);
    return current;
}

/* 仅目录worker/队列worker按需补齐旧收藏、历史的台名及线路。
 * 不改播放generation、队列、saved ID，也不驻留JSON。budget由一次分页/切台事务持有。
 * 返回1=恢复成功，0=本次失败可重试，-1=浏览已取消，-2=本事务网络预算用完。 */
static char *divoom_radio_playlist(uint32 id, uint8 player);

static uint8 divoom_radio_saved_metadata_complete(uint32 id, const divoom_radio_station *station)
{
    return station->id == id && station->compatibility <= 2 &&
        memchr(station->name, 0, sizeof(station->name)) && station->name[0] &&
        memchr(station->url, 0, sizeof(station->url)) &&
        ((!station->url[0] && !(id & 0x80000000u)) ||
            ((!strncmp(station->url, "http://", 7) || !strncmp(station->url, "https://", 8)) &&
                !strpbrk(station->url, "\r\n\t ")));
}

/* PLS字段不可截断；匹配完整key，不能把Title10作为Title1。 */
static uint8 divoom_radio_saved_pls_value(const char *text, const char *key, char *out, uint32 capacity)
{
    uint32 key_length = (uint32)strlen(key);
    while (text && *text)
    {
        const char *end = strpbrk(text, "\r\n");
        uint32 length = end ? (uint32)(end - text) : (uint32)strlen(text);
        if (length > key_length && text[key_length] == '=' && !memcmp(text, key, key_length))
        {
            uint32 bytes = length - key_length - 1;
            if (!bytes || bytes >= capacity)
            {
                return 0;
            }
            memcpy(out, text + key_length + 1, bytes);
            out[bytes] = 0;
            return 1;
        }
        text = end ? end + 1 : NULL;
    }
    return 0;
}

/* 仅去掉SHOUTcast确认过的“(#线路 - 在线人数/容量) ”前缀，不修改站名正文。 */
static const char *divoom_radio_saved_pls_title(const char *title)
{
    const char *cursor = title;
    if (cursor[0] != '(' || cursor[1] != '#')
    {
        return title;
    }
    cursor += 2;
    const char *digits = cursor;
    while (*cursor >= '0' && *cursor <= '9')
    {
        cursor++;
    }
    if (cursor == digits || strncmp(cursor, " - ", 3))
    {
        return title;
    }
    cursor += 3;
    digits = cursor;
    while (*cursor >= '0' && *cursor <= '9')
    {
        cursor++;
    }
    if (cursor == digits || *cursor++ != '/')
    {
        return title;
    }
    digits = cursor;
    while (*cursor >= '0' && *cursor <= '9')
    {
        cursor++;
    }
    if (cursor == digits || strncmp(cursor, ") ", 2) || !cursor[2])
    {
        return title;
    }
    return cursor + 2;
}

static uint8 divoom_radio_saved_pls_metadata(const char *text, uint32 id, divoom_radio_station *station)
{
    const char *cursor = text;
    while (cursor && *cursor)
    {
        const char *end = strpbrk(cursor, "\r\n");
        uint32 length = end ? (uint32)(end - cursor) : (uint32)strlen(cursor);
        if (length > 6 && !memcmp(cursor, "File", 4))
        {
            const char *number = cursor + 4;
            uint32 index = 0;
            uint32 used = 4;
            while (used < length && *number >= '0' && *number <= '9')
            {
                /* 线路号只用于FileN/TitleN配对，拒绝溢出，不依赖精简libc的宽整数解析。 */
                if (index > 6553 || (index == 6553 && *number > '5'))
                {
                    index = 0;
                    break;
                }
                index = index * 10 + (uint32)(*number++ - '0');
                used++;
            }
            if (index && used < length && *number == '=')
            {
                char file_key[20], title_key[20], title[256];
                snprintf(file_key, sizeof(file_key), "File%u", index);
                snprintf(title_key, sizeof(title_key), "Title%u", index);
                if (divoom_radio_saved_pls_value(text, file_key, station->url, sizeof(station->url)) &&
                    divoom_radio_saved_pls_value(text, title_key, title, sizeof(title)))
                {
                    const char *name = divoom_radio_saved_pls_title(title);
                    size_t bytes = strlen(name);
                    if (bytes && bytes < sizeof(station->name) && !strpbrk(name, "\r\n\t"))
                    {
                        station->id = id;
                        memcpy(station->name, name, bytes + 1);
                        if (divoom_radio_saved_metadata_complete(id, station))
                        {
                            return 1;
                        }
                    }
                }
            }
        }
        cursor = end ? end + 1 : NULL;
    }
    return 0;
}

static uint8 divoom_radio_saved_metadata_current(uint8 browsing, uint32 revision)
{
    uint8 current;
    divoom_system_get_lock(g_divoom_radio.lock);
    current = !browsing || (g_divoom_radio.menu_active &&
        g_divoom_radio.browse_requested == revision && g_divoom_radio.browse_applied == revision);
    divoom_system_free_lock(g_divoom_radio.lock);
    return current;
}

static int32 divoom_radio_saved_metadata_restore(uint32 id, divoom_radio_station *station,
    uint8 *budget, uint8 browsing, uint32 revision)
{
    uint8 found = 0;
    if (!id || !station || !budget || !divoom_radio_saved_metadata_current(browsing, revision))
    {
        return -1;
    }
    found = divoom_radio_browse_station_read(id, station) &&
        divoom_radio_saved_metadata_complete(id, station);
    /* 低位台若暂用URL显示名称，下次浏览仍允许PLS补真实名称，不把一次离线固化为名称。 */
    if (found && ((id & 0x80000000u) ||
        strncmp(station->url, station->name, strlen(station->name))))
    {
        return 1;
    }
    /* 读取失败可能已部分覆盖输出缓冲；先清理再取当前目录/上次成功播放的单台快照。 */
    if (!found)
    {
        memset(station, 0, sizeof(*station));
    }
    divoom_system_get_lock(g_divoom_radio.lock);
    int32 index = divoom_radio_find(id);
    if (index >= 0 && divoom_radio_saved_metadata_complete(id, divoom_radio_station_at(index)))
    {
        *station = *divoom_radio_station_at(index);
        found = 1;
    }
    else if (g_divoom_radio.saved.last_magic == 0x52414432 &&
        divoom_radio_saved_metadata_complete(id, &g_divoom_radio.saved.last_station))
    {
        /* 仿真旧store没有last_station：仿真版本省略此else-if，不能改其旧804字节布局。 */
        *station = g_divoom_radio.saved.last_station;
        found = 1;
    }
    divoom_system_free_lock(g_divoom_radio.lock);
    if (found && strncmp(station->url, station->name, strlen(station->name)))
    {
        divoom_radio_browse_station_save(station);
        return 1;
    }
    if (!found)
    {
        /* 真机旧JSON先导入既有紧凑索引，立即释放DOM，再做任何HTTP。
         * 后台队列只使用已经导入的索引，不能为了补资料重新驻留JSON。 */
        if (browsing && !g_divoom_radio.compat_loaded)
        {
            divoom_radio_cache_load();
            divoom_radio_cache_release();
        }
        divoom_radio_compat *known = divoom_radio_compat_find(g_divoom_radio.compat_records, id);
        const char *url = known ? divoom_radio_compat_url(known) : NULL;
        if (url && url[0] && strlen(url) < sizeof(station->url))
        {
            station->id = id;
            snprintf(station->url, sizeof(station->url), "%s", url);
            /* 仅显示名可裁剪，实际播放URL绝不截断。无原台名时显示原URL，不猜造名称。 */
            snprintf(station->name, sizeof(station->name), "%s", url);
            snprintf(station->mt, sizeof(station->mt), "%s", known->data);
            station->compatibility = known->status;
            found = divoom_radio_saved_metadata_complete(id, station);
        }
    }
    if ((id & 0x80000000u) || (found && station->compatibility == 2))
    {
        /* 哈希ID不能送到国外PLS；无URL则本轮可重试且saved ID不变，已明确不支持也不重复探测。 */
        if (found)
        {
            divoom_radio_browse_station_save(station);
        }
        return found;
    }
    if (!*budget)
    {
        return found ? 1 : -2;
    }
    (*budget)--;
    /* 必须锁外HTTP；不调用播放器/切源，也不使用兼容JSON树。 */
    divoom_radio_station resolved = {0};
    /* 上一事务或前置persist也可能留下DOM；任何补资料HTTP前都必须归还。 */
    divoom_radio_cache_release();
    char *playlist = divoom_radio_playlist(id, browsing ? 0 : 2);
    uint8 ok = playlist && divoom_radio_saved_pls_metadata(playlist, id, &resolved);
    divoom_system_free(playlist);
    if (!divoom_radio_saved_metadata_current(browsing, revision))
    {
        return -1;
    }
    if (ok)
    {
        if (found && !strcmp(station->url, resolved.url))
        {
            memcpy(resolved.mt, station->mt, sizeof(resolved.mt));
            resolved.compatibility = station->compatibility;
        }
        *station = resolved;
    }
    if (ok || found)
    {
        divoom_radio_browse_station_save(station);
    }
    return ok || found;
}

static int32 divoom_radio_saved_page(divoom_radio_page *page, int32 limit)
{
    uint32 offset = page->offset;
    int32 count = 0;
    uint8 local_budget = DIVOOM_RADIO_PAGE_SIZE;
    uint8 *budget = page->restore_budget ? page->restore_budget : &local_budget;
    page->retry = 0;
    page->total = DIVOOM_RADIO_MAX;
    while (offset < DIVOOM_RADIO_MAX && count < limit)
    {
        uint32 id;
        divoom_system_get_lock(g_divoom_radio.lock);
        id = page->query.kind == DIVOOM_RADIO_BROWSE_FAVORITE ?
            g_divoom_radio.saved.ids[offset] :
            (page->browsing ? g_divoom_radio.browse_recent_ids[offset] : g_divoom_radio.saved.recent[offset]);
        divoom_system_free_lock(g_divoom_radio.lock);
        if (!id)
        {
            offset++;
            continue;
        }
        int32 restored = divoom_radio_saved_metadata_restore(id, &page->stations[count],
            budget, page->browsing, page->revision);
        if (restored != 1)
        {
            page->retry = 1;
            break;
        }
        offset++;
        count++;
    }
    page->count = count;
    page->offset = offset;
    page->exhausted = offset >= DIVOOM_RADIO_MAX;
    return count != 0 || !page->retry;
}

/* 国内接口没有分页字段：一次取最多100条轻量记录落盘后释放回包，显示/解码仍每段6条。 */
static int32 divoom_radio_browse_fetch_page(divoom_radio_page *page, int32 limit)
{
    divoom_radio_browse_query *query = &page->query;
    int32 count = 0;
    uint32 offset = page->offset;
    if (query->kind == DIVOOM_RADIO_BROWSE_DOMESTIC)
    {
        char path[192];
        void *file;
        divoom_radio_browse_path(path, sizeof(path), (uint32)query->classify_id, 1);
        if (divoom_main_get_server_type() != 0)
        {
            return 0;
        }
        if (!page->domestic_loaded)
        {
            Pdivoom_net_command reply = divoom_net_app_send_packet_wait(DIVOOM_NET_COMM_RADIO_GET_LIST,
                DIVOOM_SERVER_TOPIC, &query->classify_id);
            divoom_device_radio_list *list = reply ? reply->cmd_data : NULL;
            uint8 ok = list && list->classify_id == query->classify_id && (!page->browsing || divoom_radio_browse_current()) &&
                divoom_radio_browse_write(path, list, sizeof(*list) + list->count * sizeof(list->items[0]));
            if (ok)
            {
                page->total = list->count;
                page->domestic_loaded = 1;
            }
            divoom_net_app_free_packet(reply);
            if (!ok)
            {
                return 0;
            }
        }
        file = divoom_port_file_open(path, "rb");
        if (!file)
        {
            page->domestic_loaded = 0;
            return 0;
        }
        /* userdata可能被截断或损坏；使用缓存前校验头、游标和字符串边界，坏文件只触发重取。 */
        divoom_device_radio_list header;
        if (divoom_port_file_read(&header, sizeof(header), 1, file) != 1 ||
            header.classify_id != query->classify_id || header.count != page->total ||
            header.count < 0 || header.count > DIVOOM_RADIO_MAX || offset > (uint32)header.count ||
            divoom_port_file_seek(file, sizeof(header) + offset * sizeof(divoom_device_radio_item), SEEK_SET) != 0)
        {
            divoom_port_file_close(file);
            page->domestic_loaded = 0;
            return 0;
        }
        while (offset + count < (uint32)page->total && count < limit)
        {
            divoom_device_radio_item item;
            divoom_radio_station *station = &page->stations[count];
            if (divoom_port_file_read(&item, sizeof(item), 1, file) != 1 || !item.radio_id ||
                !memchr(item.name, 0, sizeof(item.name)) || !memchr(item.url, 0, sizeof(item.url)) ||
                !item.name[0] || (strncmp(item.url, "http://", 7) && strncmp(item.url, "https://", 8)) ||
                strpbrk(item.url, "\r\n\t "))
            {
                divoom_port_file_close(file);
                page->domestic_loaded = 0;
                return 0;
            }
            memset(station, 0, sizeof(*station));
            station->id = divoom_radio_browse_url_id(item.url);
            if (!station->id)
            {
                divoom_port_file_close(file);
                return 0;
            }
            snprintf(station->name, sizeof(station->name), "%s", item.name);
            snprintf(station->url, sizeof(station->url), "%s", item.url);
            count++;
        }
        divoom_port_file_close(file);
    }
    else
    {
        return divoom_radio_saved_page(page, limit);
    }

    page->count = count;
    page->offset += count;
    page->exhausted = page->offset >= (uint32)page->total;
    return 1;
}

static int32 divoom_radio_browse_fetch(int32 limit)
{
    divoom_radio_page page;
    memset(&page, 0, sizeof(page));
    page.query = g_divoom_radio.browse_active;
    page.offset = g_divoom_radio.source_offset;
    page.total = g_divoom_radio.source_total;
    page.domestic_loaded = g_divoom_radio.browse_domestic_loaded;
    page.stations = divoom_radio_pending_acquire();
    if (!page.stations)
    {
        return 0;
    }
    page.browsing = 1;
    page.restore_budget = &g_divoom_radio.restore_budget;
    page.revision = g_divoom_radio.browse_applied;
    int32 ok = divoom_radio_browse_fetch_page(&page, limit);
    g_divoom_radio.pending_restore_retry = page.retry;
    g_divoom_radio.browse_domestic_loaded = page.domestic_loaded;
    if (ok)
    {
        g_divoom_radio.source_offset = page.offset;
        g_divoom_radio.source_total = page.total;
        g_divoom_radio.browse_domestic_loaded = page.domestic_loaded;
        g_divoom_radio.exhausted = page.exhausted;
        g_divoom_radio.pending_count = page.count;
        g_divoom_radio.pending_pos = 0;
    }
    return ok;
}


static int32 divoom_radio_fetch_page(divoom_radio_page *page)
{
    const char *snapshot = divoom_radio_env("DIVOOM_SIM_RADIO_SNAPSHOT");
    char *text = NULL;
    uint32 offset = page->offset;
    int32 count = 0, parsed = 0;
    int32 limit = DIVOOM_RADIO_SOURCE_MAX - offset;
    if (limit > DIVOOM_RADIO_PAGE_SIZE)
    {
        limit = DIVOOM_RADIO_PAGE_SIZE;
    }
    if (limit <= 0)
    {
        page->exhausted = 1;
        return 1;
    }
    divoom_system_get_lock(g_divoom_radio.lock);
    g_divoom_radio.state.page_requests++;
    divoom_system_free_lock(g_divoom_radio.lock);
    if (page->query.kind >= DIVOOM_RADIO_BROWSE_DOMESTIC)
    {
        return divoom_radio_browse_fetch_page(page, limit);
    }
    if (snapshot && snapshot[0])
    {
        text = divoom_radio_read(snapshot);
        if (text)
        {
            divoom_cJSON *root = divoom_cJSON_Parse(text);
            divoom_cJSON *array = root ? divoom_cJSON_GetObjectItem(root, "stations") : NULL;
            if (array && (array->type & 0xff) == divoom_cJSON_Array)
            {
                parsed = 1;
                page->total = divoom_cJSON_GetArraySize(array);
                if (page->total > DIVOOM_RADIO_SOURCE_MAX)
                {
                    page->total = DIVOOM_RADIO_SOURCE_MAX;
                }
                for (int32 i = offset; i < page->total && count < limit; i++)
                {
                    divoom_cJSON *item = divoom_cJSON_GetArrayItem(array, i);
                    divoom_radio_station *station = &page->stations[count++];
                    memset(station, 0, sizeof(*station));
                    station->id = (uint32)strtoul(divoom_radio_string(item, "id"), NULL, 10);
                    snprintf(station->name, sizeof(station->name), "%s", divoom_radio_string(item, "name"));
                    snprintf(station->genre, sizeof(station->genre), "%s", divoom_radio_string(item, "genre"));
                    snprintf(station->mt, sizeof(station->mt), "%s", divoom_radio_string(item, "mt"));
                    station->bitrate = (uint16)atoi(divoom_radio_string(item, "br"));
                }
            }
            divoom_cJSON_Delete(root);
        }
    }
    else
    {
        const char *key = divoom_radio_env("DIVOOM_SHOUTCAST_KEY");
        /* 与工程原SHOUTcast入口共用凭据，不在日志/配置中保存。 */
        extern const char *divoom_net_radio_directory_key(void);
        if (!key || !key[0])
        {
            key = divoom_net_radio_directory_key();
        }
        if (key && key[0] && strlen(key) < 256)
        {
            char post[768];
            char genre[289];
            size_t used = 0;
            const char *endpoint = divoom_radio_env("DIVOOM_SIM_RADIO_DIRECTORY_URL");
            /* 分类名称来自固定模型；按UTF-8字节编码，避免空格、&或非ASCII破坏表单。 */
            for (const uint8 *p = (const uint8 *)page->query.genre; *p && used + 3 < sizeof(genre); p++)
            {
                used += (size_t)snprintf(genre + used, sizeof(genre) - used, "%%%02X", *p);
            }
            genre[used] = 0;
            snprintf(post, sizeof(post), "k=%s&limit=%u,%d%s%s", key, offset, limit,
                page->query.kind == DIVOOM_RADIO_BROWSE_GENRE ? "&genre=" : "",
                page->query.kind == DIVOOM_RADIO_BROWSE_GENRE ? genre : "");
            text = divoom_radio_http(endpoint && endpoint[0] ? endpoint :
                (page->query.kind == DIVOOM_RADIO_BROWSE_GENRE ?
                "http://api.shoutcast.com/legacy/genresearch" : "http://api.shoutcast.com/legacy/Top500"), post, 0, 0, NULL);
            memset(post, 0, sizeof(post));
        }
        if (text)
        {
            const char *cursor = text;
            parsed = strstr(text, "<stationlist") != NULL;
            while ((cursor = strstr(cursor, "<station ")) != NULL && count < limit)
            {
                const char *end = strchr(cursor, '>');
                divoom_radio_station *station = &page->stations[count++];
                char number[32];
                if (!end)
                {
                    parsed = 0;
                    break;
                }
                memset(station, 0, sizeof(*station));
                divoom_radio_attribute(cursor, end, "id", number, sizeof(number));
                station->id = (uint32)strtoul(number, NULL, 10);
                divoom_radio_attribute(cursor, end, "name", station->name, sizeof(station->name));
                divoom_radio_attribute(cursor, end, "genre", station->genre, sizeof(station->genre));
                divoom_radio_attribute(cursor, end, "ct", station->track, sizeof(station->track));
                divoom_radio_attribute(cursor, end, "mt", station->mt, sizeof(station->mt));
                divoom_radio_attribute(cursor, end, "br", number, sizeof(number));
                station->bitrate = (uint16)atoi(number);
                cursor = end + 1;
            }
            if (count < limit)
            {
                page->total = offset + count;
            }
        }
    }
    divoom_system_free(text);
    if (parsed)
    {
            page->count = count;
        page->offset += count;
        if (page->offset >= (uint32)page->total)
        {
            page->exhausted = 1;
        }
    }
    return parsed;
}

static int32 divoom_radio_fetch(void)
{
    divoom_radio_page page;
    memset(&page, 0, sizeof(page));
    page.query = g_divoom_radio.browse_active;
    page.offset = g_divoom_radio.source_offset;
    page.total = g_divoom_radio.source_total;
    page.domestic_loaded = g_divoom_radio.browse_domestic_loaded;
    page.stations = divoom_radio_pending_acquire();
    if (!page.stations)
    {
        return 0;
    }
    page.browsing = 1;
    page.restore_budget = &g_divoom_radio.restore_budget;
    page.revision = g_divoom_radio.browse_applied;
    int32 ok = divoom_radio_fetch_page(&page);
    g_divoom_radio.pending_restore_retry = page.retry;
    g_divoom_radio.browse_domestic_loaded = page.domestic_loaded;
    if (ok)
    {
        g_divoom_radio.source_offset = page.offset;
        g_divoom_radio.source_total = page.total;
        g_divoom_radio.browse_domestic_loaded = page.domestic_loaded;
        g_divoom_radio.exhausted = page.exhausted;
        g_divoom_radio.pending_count = page.count;
        g_divoom_radio.pending_pos = 0;
    }
    return ok;
}

/* 显式URL也能增加目录数量；判断和最终提交都使用同一radio锁。 */
static uint8 divoom_radio_directory_needed(int32 target, uint8 background)
{
    uint8 needed;
    divoom_system_get_lock(g_divoom_radio.lock);
    needed = g_divoom_radio.state.count < target && g_divoom_radio.state.count < DIVOOM_RADIO_MAX + (g_divoom_radio.browse_pinned != 0) &&
        g_divoom_radio.browse_requested == g_divoom_radio.browse_applied &&
        g_divoom_radio.browse_active.kind != DIVOOM_RADIO_BROWSE_AUTO &&
        (background || g_divoom_radio.menu_active);
    divoom_system_free_lock(g_divoom_radio.lock);
    return needed;
}

/* 点选决定播放上下文；只保存ID，浏览目录的替换不改变队列。调用时持radio锁。 */
static void divoom_radio_queue_capture(uint32 id)
{
    divoom_radio_play_queue *queue = &g_divoom_radio.play_queue;
    uint32 epoch = queue->epoch + 1;
    memset(queue, 0, sizeof(*queue));
    queue->epoch = epoch;
    queue->query = g_divoom_radio.browse_active;
    queue->browse_revision = g_divoom_radio.browse_applied;
    queue->total = DIVOOM_RADIO_SOURCE_MAX;
    int32 limit = queue->query.kind >= DIVOOM_RADIO_BROWSE_FAVORITE ?
        DIVOOM_RADIO_MAX : g_divoom_radio.state.count;
    for (int32 i = 0; i < limit && queue->count < DIVOOM_RADIO_MAX; i++)
    {
        uint32 candidate = 0;
        if (queue->query.kind == DIVOOM_RADIO_BROWSE_FAVORITE)
        {
            candidate = g_divoom_radio.saved.ids[i];
        }
        else if (queue->query.kind == DIVOOM_RADIO_BROWSE_RECENT)
        {
            candidate = g_divoom_radio.browse_recent_ids[i];
        }
        else if (i < g_divoom_radio.state.count)
        {
            candidate = divoom_radio_station_at(i)->id;
            if (candidate == g_divoom_radio.browse_pinned)
            {
                candidate = 0;
            }
        }
        if (candidate)
        {
            uint8 duplicate = 0;
            for (int32 j = 0; j < queue->count; j++)
            {
                duplicate |= queue->ids[j] == candidate;
            }
            if (!duplicate)
            {
                queue->ids[queue->count++] = candidate;
            }
        }
    }
    queue->position = -1;
    for (int32 i = 0; i < queue->count; i++)
    {
        if (queue->ids[i] == id)
        {
            queue->position = i;
        }
    }
    if (queue->position < 0)
    {
        /* 恢复槽/手动URL不属于当前浏览分类，不能误接到该分类的上下首。 */
        queue->count = 1;
        queue->ids[0] = id;
        queue->position = 0;
        queue->exhausted = 1;
    }
    queue->exhausted |= queue->count >= DIVOOM_RADIO_MAX ||
        queue->query.kind >= DIVOOM_RADIO_BROWSE_FAVORITE;
}

/* 仅结束本次未提交的切台，避免覆盖新的按键意图。 */
static void divoom_radio_queue_abort(const divoom_radio_play_queue *snapshot)
{
    divoom_system_get_lock(g_divoom_radio.lock);
    if (g_divoom_radio.play_queue.epoch == snapshot->epoch &&
        g_divoom_radio.play_queue.step_serial == snapshot->step_serial)
    {
        g_divoom_radio.play_queue.steps = 0;
        g_divoom_radio.play_queue.retry_queued = 0;
    }
    divoom_system_free_lock(g_divoom_radio.lock);
}

/* 重启后只保存了最后成功台。首次后台换台按当前地区的上次列表/默认列表补足队列。 */
static uint8 divoom_radio_queue_resolve(divoom_radio_browse_query *query)
{
    if (query->kind != DIVOOM_RADIO_BROWSE_AUTO || divoom_radio_browse_config_read(query))
    {
        return 1;
    }
    if (divoom_main_get_server_type() != 0)
    {
        query->kind = DIVOOM_RADIO_BROWSE_TOP;
        return 1;
    }
    divoom_system_get_lock(g_divoom_radio.lock);
    int32 classify_id = divoom_radio_classify_cnr(g_divoom_radio.browse_classes);
    divoom_system_free_lock(g_divoom_radio.lock);
    if (classify_id <= 0)
    {
        Pdivoom_net_command reply = divoom_net_app_send_packet_wait(DIVOOM_NET_COMM_RADIO_GET_CLASSIFY,
            DIVOOM_SERVER_TOPIC, NULL);
        classify_id = divoom_radio_classify_cnr(reply ? reply->cmd_data : NULL);
        divoom_net_app_free_packet(reply);
    }
    if (classify_id <= 0)
    {
        return 0;
    }
    query->kind = DIVOOM_RADIO_BROWSE_DOMESTIC;
    query->classify_id = classify_id;
    return 1;
}

/* 一次只取6条；不暂借/覆盖browse_active，后台与页面各自维护源游标。 */
static void divoom_radio_queue_page(const divoom_radio_play_queue *snapshot)
{
    divoom_radio_page page;
    divoom_radio_station *previous;
    memset(&page, 0, sizeof(page));
    page.query = snapshot->query;
    if (!divoom_radio_queue_resolve(&page.query))
    {
        divoom_radio_queue_abort(snapshot);
        return;
    }
    page.offset = snapshot->offset;
    page.total = snapshot->total;
    page.domestic_loaded = snapshot->domestic_loaded;
    page.stations = divoom_system_malloc(sizeof(*page.stations) * DIVOOM_RADIO_PAGE_SIZE);
    previous = divoom_system_malloc(sizeof(*previous));
    if (!page.stations || !previous)
    {
        divoom_system_free(page.stations);
        divoom_system_free(previous);
        divoom_radio_queue_abort(snapshot);
        return;
    }
    if (divoom_radio_fetch_page(&page))
    {
        for (int32 i = 0; i < page.count; i++)
        {
            divoom_radio_station *station = &page.stations[i];
            if (divoom_radio_browse_station_read(station->id, previous) && !strcmp(previous->mt, station->mt))
            {
                station->compatibility = previous->compatibility;
                if (!station->url[0])
                {
                    snprintf(station->url, sizeof(station->url), "%s", previous->url);
                }
            }
            if (station->id)
            {
                divoom_radio_browse_station_save(station);
            }
        }
        divoom_system_get_lock(g_divoom_radio.lock);
        divoom_radio_play_queue *queue = &g_divoom_radio.play_queue;
        if (queue->epoch == snapshot->epoch)
        {
            queue->query = page.query;
            queue->offset = page.offset;
            queue->total = page.total;
            queue->domestic_loaded = page.domestic_loaded;
            queue->exhausted = page.exhausted;
            for (int32 i = 0; i < page.count && queue->count < DIVOOM_RADIO_MAX; i++)
            {
                divoom_radio_station *station = &page.stations[i];
                uint8 duplicate = !station->id || station->compatibility == 2 || divoom_radio_mime_rejected(station->mt);
                for (int32 j = 0; j < queue->count; j++)
                {
                    duplicate |= queue->ids[j] == station->id;
                }
                if (!duplicate)
                {
                    queue->ids[queue->count++] = station->id;
                }
            }
            queue->exhausted |= queue->count >= DIVOOM_RADIO_MAX;
        }
        divoom_system_free_lock(g_divoom_radio.lock);
    }
    /* 超时只结束本次切换，不写不支持、不移除ID；下一次按键可重新请求。 */
    else
    {
        divoom_radio_queue_abort(snapshot);
    }
    divoom_system_free(previous);
    divoom_system_free(page.stations);
}

/* 目录worker唯一执行文件/网络操作；最后以队列代号+播放代号校验提交，旧上下首不能覆盖新选台/切源。 */
static void divoom_radio_queue_update(void)
{
    divoom_radio_play_queue snapshot;
    divoom_radio_station *station;
    int32 position, direction;
    divoom_system_get_lock(g_divoom_radio.lock);
    if (g_divoom_radio.play_queue.step_generation != divoom_radio_generation())
    {
        g_divoom_radio.play_queue.steps = 0;
    }
    snapshot = g_divoom_radio.play_queue;
    divoom_system_free_lock(g_divoom_radio.lock);
    if (!snapshot.steps || !snapshot.count)
    {
        return;
    }
    direction = snapshot.steps > 0 ? 1 : -1;
    position = snapshot.retry_queued ? snapshot.retry_cursor : snapshot.position;
    station = divoom_system_malloc(sizeof(*station));
    if (!station)
    {
        divoom_radio_queue_abort(&snapshot);
        return;
    }
    uint8 metadata_budget = 1;
    for (int32 checked = 0; checked < snapshot.count; checked++)
    {
        int32 previous_position = position;
        position += direction;
        if ((position >= snapshot.count || position < 0) && !snapshot.exhausted)
        {
            divoom_system_free(station);
            divoom_radio_queue_page(&snapshot);
            return;
        }
        position = position < 0 ? snapshot.count - 1 : position % snapshot.count;
        /* 失败也占用本轮的一个候选；绕回最初位置即结束，避免反复连接。 */
        if (position == snapshot.position || (snapshot.retry_queued && position == snapshot.step_origin))
        {
            break;
        }
        uint32 id = snapshot.ids[position];
        int32 index;
        divoom_system_get_lock(g_divoom_radio.lock);
        index = divoom_radio_find(id);
        if (index >= 0)
        {
            *station = *divoom_radio_station_at(index);
        }
        divoom_system_free_lock(g_divoom_radio.lock);
        int32 restored = index >= 0 ? 1 : divoom_radio_saved_metadata_restore(id, station, &metadata_budget, 0, 0);
        if (restored != 1)
        {
            if (snapshot.retry_queued && (restored == 0 || restored == -2))
            {
                /* 本轮跳过暂时取不到线路的旧收藏，保留原ID；一次轮询最多补一台元数据。 */
                divoom_system_get_lock(g_divoom_radio.lock);
                divoom_radio_play_queue *queue = &g_divoom_radio.play_queue;
                if (queue->epoch == snapshot.epoch && queue->step_serial == snapshot.step_serial &&
                    snapshot.step_generation == divoom_radio_generation())
                {
                    queue->retry_cursor = restored == -2 ? previous_position : position;
                }
                divoom_system_free_lock(g_divoom_radio.lock);
                divoom_system_free(station);
                return;
            }
            break;
        }
        if (station->compatibility == 2 || divoom_radio_mime_rejected(station->mt))
        {
            continue;
        }
        divoom_system_get_lock(g_divoom_radio.lock);
        divoom_radio_play_queue *queue = &g_divoom_radio.play_queue;
        if (queue->epoch == snapshot.epoch && queue->step_serial == snapshot.step_serial &&
            queue->step_generation == divoom_radio_generation())
        {
            index = divoom_radio_find(id);
            if (index < 0)
            {
                index = divoom_radio_find(g_divoom_radio.browse_pinned);
                if (index < 0)
                {
                    index = divoom_radio_directory_append();
                    if (index < 0)
                    {
                        divoom_system_free_lock(g_divoom_radio.lock);
                        break;
                    }
                    g_divoom_radio.target_count++;
                    g_divoom_radio.state.total++;
                }
                *divoom_radio_station_at(index) = *station;
                g_divoom_radio.browse_pinned = id;
            }
            divoom_radio_select_locked(id, queue->wait_for_commit);
            queue->wait_for_commit = 0;
            if (!snapshot.retry_queued)
            {
                queue->step_origin = snapshot.position;
            }
            queue->position = position;
            queue->steps -= direction;
            queue->step_generation = divoom_radio_generation();
            queue->retry_direction = direction;
            queue->retry_queued = 0;
        }
        divoom_system_free_lock(g_divoom_radio.lock);
        divoom_system_free(station);
        return;
    }
    divoom_radio_queue_abort(&snapshot);
    divoom_system_free(station);
}


static void divoom_radio_load_directory(uint8 background)
{
    int32 target, failed = 0;
    divoom_system_get_lock(g_divoom_radio.lock);
    g_divoom_radio.restore_budget = DIVOOM_RADIO_PAGE_SIZE;
    g_divoom_radio.pending_restore_retry = 0;
    target = g_divoom_radio.target_count;
    if (target > DIVOOM_RADIO_MAX + (g_divoom_radio.browse_pinned != 0))
    {
        target = DIVOOM_RADIO_MAX + (g_divoom_radio.browse_pinned != 0);
    }
    divoom_system_free_lock(g_divoom_radio.lock);
    while (divoom_radio_directory_needed(target, background))
    {
        divoom_radio_station *station;
        divoom_radio_compat *known;
        int32 reject;
        if (g_divoom_radio.pending_pos >= g_divoom_radio.pending_count)
        {
            if (g_divoom_radio.pending_restore_retry)
            {
                failed = 1;
                break;
            }
            if (g_divoom_radio.exhausted)
            {
                break;
            }
            /* 翻页等待HTTP前先归还完整配置，网络阻塞也不延长JSON驻留。 */
            divoom_radio_cache_release();
            if (!divoom_radio_fetch())
            {
                failed = 1;
                break;
            }
            if (!g_divoom_radio.pending_count)
            {
                break;
            }
        }
        if (!divoom_radio_browse_current() || (!background && !divoom_radio_menu_active()))
        {
            break;
        }
        if (!background)
        {
            divoom_radio_cache_load();
        }
        if (!background && !g_divoom_radio.compat_loaded)
        {
            /* 缺配置会导入空索引；只有内存不足等暂时失败才保留本段待重试。 */
            failed = 1;
            break;
        }
        station = &g_divoom_radio.pending[g_divoom_radio.pending_pos++];
        known = divoom_radio_compat_find(g_divoom_radio.compat_records, station->id);
        if (known && strcmp(known->data, station->mt) == 0)
        {
            station->compatibility = known->status;
            const char *cached_url = divoom_radio_compat_url(known);
            if (strlen(cached_url) < sizeof(station->url))
            {
                snprintf(station->url, sizeof(station->url), "%s", cached_url);
            }
        }
        reject = divoom_radio_mime_rejected(station->mt) || station->compatibility == 2;
        divoom_system_get_lock(g_divoom_radio.lock);
        if (g_divoom_radio.state.count >= target || g_divoom_radio.state.count >= DIVOOM_RADIO_MAX + (g_divoom_radio.browse_pinned != 0))
        {
            /* HTTP期间可能被显式URL补满；保留该源项，绝不越界或静默吃掉游标。 */
            g_divoom_radio.pending_pos--;
            divoom_system_free_lock(g_divoom_radio.lock);
            break;
        }
        g_divoom_radio.state.scanned++;
        if (reject)
        {
            g_divoom_radio.state.filtered++;
        }
        else if (station->id && station->id == g_divoom_radio.browse_pinned)
        {
            /* 同一台也属于新分类时直接显示保留槽，不重复追加；首段仍为6条。 */
            g_divoom_radio.browse_pinned = 0;
            target--;
            g_divoom_radio.target_count--;
        }
        else if (station->id && divoom_radio_find(station->id) < 0)
        {
            int32 index = divoom_radio_directory_append();
            if (index < 0)
            {
                g_divoom_radio.state.scanned--;
                g_divoom_radio.pending_pos--;
                failed = 1;
                divoom_system_free_lock(g_divoom_radio.lock);
                break;
            }
            /* 台名/URL文件中的旧标记不能复活已取消的收藏。 */
            station->favorite = 0;
            station->recent = 0;
            for (int32 j = 0; j < DIVOOM_RADIO_MAX; j++)
            {
                station->favorite |= g_divoom_radio.saved.ids[j] == station->id;
                if (g_divoom_radio.saved.recent[j] == station->id)
                {
                    station->recent = DIVOOM_RADIO_MAX - j;
                }
            }
            *divoom_radio_station_at(index) = *station;
            if (station->compatibility)
            {
                g_divoom_radio.state.cache_hits++;
            }
        }
        divoom_system_free_lock(g_divoom_radio.lock);
        if (station->id)
        {
            divoom_radio_browse_station_save(station);
        }
    }
    divoom_system_get_lock(g_divoom_radio.lock);
    if (g_divoom_radio.pending_pos >= g_divoom_radio.pending_count)
    {
        divoom_radio_pending_release();
    }
    /* 后台仍在播放时保留未完成源游标，重进可续载。 */
    if (!g_divoom_radio.menu_active && g_divoom_radio.state.count < target)
    {
        g_divoom_radio.more = 1;
    }
    g_divoom_radio.state.total = g_divoom_radio.exhausted &&
        g_divoom_radio.pending_pos >= g_divoom_radio.pending_count ? g_divoom_radio.state.count : DIVOOM_RADIO_MAX + (g_divoom_radio.browse_pinned != 0);
    g_divoom_radio.state.load = failed && g_divoom_radio.state.count == (g_divoom_radio.browse_pinned != 0) ? DIVOOM_RADIO_LOAD_FAILED : DIVOOM_RADIO_READY;
    g_divoom_radio.state.more_error = failed && g_divoom_radio.state.count > 0;
    g_divoom_radio.state.offline = divoom_radio_env("DIVOOM_SIM_RADIO_SNAPSHOT") != NULL;
    g_divoom_radio.state.loading_more = 0;
    g_divoom_radio.state.revision++;
    divoom_system_free_lock(g_divoom_radio.lock);
}

/* 仅可靠魔数/连续帧给明确结论；无法识别、HTML错误页均保持未知。 */
static uint8 divoom_radio_sniff(const uint8 *data, size_t size)
{
    if (size >= 12 && (!memcmp(data, "OggS", 4) || !memcmp(data, "fLaC", 4) ||
        !memcmp(data + 4, "ftyp", 4) ||
        (!memcmp(data, "RIFF", 4) && !memcmp(data + 8, "WAVE", 4))))
    {
        return 2;
    }
    for (size_t i = 0; i + 14 < size; i++)
    {
        const uint8 *p = data + i;
        if (p[0] == 0xff && (p[1] & 0xf6) == 0xf0)
        {
            size_t length = ((p[3] & 3) << 11) | (p[4] << 3) | (p[5] >> 5);
            if (length >= 7 && i + length + 7 < size && p[length] == 0xff &&
                (p[length + 1] & 0xf6) == 0xf0)
            {
                return 1;
            }
        }
        if (p[0] == 0xff && (p[1] & 0xe6) == 0xe2 && (p[1] & 0x18) != 0x08 &&
            (p[2] & 0xf0) != 0 && (p[2] & 0xf0) != 0xf0 && (p[2] & 0x0c) != 0x0c)
        {
            static const uint16 br1[] = {0,32,40,48,56,64,80,96,112,128,160,192,224,256,320};
            static const uint16 br2[] = {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160};
            static const uint32 rates[] = {44100,48000,32000};
            int32 mpeg1 = (p[1] & 0x18) == 0x18;
            uint32 rate = rates[(p[2] >> 2) & 3] >> (mpeg1 ? 0 : ((p[1] & 0x18) ? 1 : 2));
            size_t length = (mpeg1 ? 144000 : 72000) * (mpeg1 ? br1[p[2] >> 4] : br2[p[2] >> 4]) /
                rate + ((p[2] >> 1) & 1);
            if (length >= 24 && i + length + 4 < size && p[length] == 0xff &&
                (p[length + 1] & 0xfe) == (p[1] & 0xfe))
            {
                return 1;
            }
        }
    }
    return 0;
}

static char *divoom_radio_playlist(uint32 id, uint8 player)
{
    if (id & 0x80000000u)
    {
        return NULL;
    }

    char url[1200];
    const char *base = divoom_radio_env("DIVOOM_SIM_RADIO_PLAYLIST_BASE");
    snprintf(url, sizeof(url), "%s%u", base && base[0] ? base :
        "https://yp.shoutcast.com/sbin/tunein-station.pls?id=", id);
    return divoom_radio_http(url, NULL, player, 0, NULL);
}

/* PLS逐行扫描，无固定线路数量数组；拒绝超长URL，绝不截断成另一条线路。 */
static const char *divoom_radio_next_url(const char *cursor, char *url, size_t capacity)
{
    while (cursor && *cursor)
    {
        const char *end = strpbrk(cursor, "\r\n");
        const char *equal;
        size_t length = end ? (size_t)(end - cursor) : strlen(cursor);
        equal = memchr(cursor, '=', length);
        if (length > 6 && !strncmp(cursor, "File", 4) && equal)
        {
            size_t bytes = length - (size_t)(equal + 1 - cursor);
            if (bytes && bytes < capacity)
            {
                memcpy(url, equal + 1, bytes);
                url[bytes] = '\0';
                return end ? end + 1 : cursor + length;
            }
        }
        cursor = end ? end + 1 : NULL;
    }
    return NULL;
}

static void divoom_radio_probe(void)
{
    divoom_radio_station *station = divoom_system_malloc(sizeof(*station));
    divoom_cJSON *routes = NULL, *known, *old_routes = NULL;
    uint32 id;
    int32 index, all_rejected = 1, seen = 0;
    char *playlist = NULL;
    char url[1024];
    const char *cursor;
    if (!station)
    {
        return;
    }
    divoom_system_get_lock(g_divoom_radio.lock);
    id = g_divoom_radio.hint_id;
    index = divoom_radio_find(id);
    if (!g_divoom_radio.menu_active || index < 0 || id == g_divoom_radio.manual_id ||
        divoom_radio_station_at(index)->compatibility ||
        (divoom_radio_station_at(index)->retry_after &&
        (int32)(divoom_system_get_tickms() - divoom_radio_station_at(index)->retry_after) < 0) ||
        g_divoom_radio.state.offline)
    {
        divoom_system_free_lock(g_divoom_radio.lock);
        divoom_system_free(station);
        return;
    }
    *station = *divoom_radio_station_at(index);
    g_divoom_radio.state.probe_requests++;
    divoom_system_free_lock(g_divoom_radio.lock);
    divoom_radio_cache_load();
    known = divoom_radio_cached(id);
    /* 只拷贝当前台的线路结果，网络探测期间不保留200台的配置树。 */
    old_routes = known ? divoom_cJSON_Duplicate(divoom_cJSON_GetObjectItem(known, "routes"), 1) : NULL;
    divoom_radio_cache_release();
    routes = divoom_cJSON_CreateArray();
    if (station->url[0] && (id & 0x80000000u))
    {
        playlist = divoom_system_malloc(strlen(station->url) + 16);
        if (playlist)
        {
            sprintf(playlist, "File1=%s\n", station->url);
        }
    }
    else
    {
        playlist = divoom_radio_playlist(id, 0);
    }
    cursor = playlist;
    while (divoom_radio_menu_active() && (cursor = divoom_radio_next_url(cursor, url, sizeof(url))) != NULL)
    {
        divoom_cJSON *route = divoom_cJSON_CreateObject();
        divoom_cJSON *old = old_routes ? old_routes->child : NULL;
        uint8 status = 0;
        seen++;
        while (old && strcmp(divoom_radio_string(old, "url"), url) != 0)
        {
            old = old->next;
        }
        if (old)
        {
            status = (uint8)divoom_radio_number(old, "status");
        }
        if (!status && strncmp(url, "http://", 7) && strncmp(url, "https://", 8))
        {
            status = 2;
        }
        if (!status)
        {
            size_t size = 0;
            char *sample = divoom_radio_http(url, NULL, 0, 1, &size);
            if (sample)
            {
                status = divoom_radio_sniff((const uint8 *)sample, size);
            }
            divoom_system_free(sample);
        }
        divoom_cJSON_AddStringToObject(route, "url", url);
        divoom_cJSON_AddNumberToObject(route, "status", status);
        divoom_cJSON_AddItemToArray(routes, route);
        if (status != 2)
        {
            all_rejected = 0;
        }
        if (status == 1)
        {
            station->compatibility = 1;
            if (strlen(url) < sizeof(station->url))
            {
                snprintf(station->url, sizeof(station->url), "%s", url);
            }
            break;
        }
        /* 新焦点/翻页优先，未完成全部线路时不能发布整台不支持。 */
        divoom_system_get_lock(g_divoom_radio.lock);
        index = g_divoom_radio.hint_id != id || g_divoom_radio.more || g_divoom_radio.reload;
        divoom_system_free_lock(g_divoom_radio.lock);
        if (index)
        {
            all_rejected = 0;
            break;
        }
    }
    if (!station->compatibility && seen && all_rejected)
    {
        station->compatibility = 2;
    }
    divoom_system_free(playlist);
    divoom_cJSON_Delete(old_routes);
    divoom_system_get_lock(g_divoom_radio.lock);
    if (!g_divoom_radio.menu_active)
    {
        /* 取消不是格式否定；退出后释放本次线路树，下次进页面仍可重新确认。 */
        divoom_system_free_lock(g_divoom_radio.lock);
        divoom_cJSON_Delete(routes);
        divoom_system_free(station);
        return;
    }
    index = divoom_radio_find(id);
    if (index >= 0)
    {
        /* 网络探测在锁外运行。其间播放worker若已提交PCM，不能被旧否定/超时结果覆盖。 */
        if (divoom_radio_station_at(index)->compatibility == 1 && station->compatibility != 1)
        {
            *station = *divoom_radio_station_at(index);
        }
        /* 这段只复制紧凑数据，不联网/不解析JSON；先暂存成功，再搬移可见目录。 */
        if (!divoom_radio_cache_put(station, routes))
        {
            divoom_radio_station_at(index)->retry_after = divoom_system_get_tickms() + 1000;
            divoom_system_free_lock(g_divoom_radio.lock);
            divoom_system_free(station);
            return;
        }
        routes = NULL;
        divoom_radio_station_at(index)->compatibility = station->compatibility;
        divoom_radio_station_at(index)->retry_after = divoom_system_get_tickms() + DIVOOM_RADIO_RETRY_MS;
        snprintf(divoom_radio_station_at(index)->url, sizeof(station->url), "%s", station->url);
        if (station->compatibility == 2)
        {
            /* 稳定id关联播放；搬移后同步修正selected，不沿用过期数组下标。 */
            if (g_divoom_radio.state.selected == index)
            {
                int32 generation = divoom_radio_generation();
                divoom_radio_fail_locked(generation);
                divoom_radio_next_generation();
                g_divoom_radio.request_id = 0;
                g_divoom_radio.source_request = 0;
                if (g_divoom_radio.play_queue.step_generation == generation)
                {
                    g_divoom_radio.play_queue.step_generation = divoom_radio_generation();
                }
                g_divoom_radio.state.selected = -1;
            }
            else if (g_divoom_radio.state.selected > index)
            {
                g_divoom_radio.state.selected--;
            }
            for (int32 i = index; i + 1 < g_divoom_radio.state.count; i++)
            {
                *divoom_radio_station_at(i) = *divoom_radio_station_at(i + 1);
            }
            g_divoom_radio.state.count--;
            divoom_radio_directory_trim();
            g_divoom_radio.state.filtered++;
            g_divoom_radio.more = 1;
            g_divoom_radio.state.loading_more = 1;
        }
        g_divoom_radio.state.revision++;
    }
    divoom_system_free_lock(g_divoom_radio.lock);
    divoom_cJSON_Delete(routes);
    divoom_radio_cache_load();
    divoom_system_free(station);
}

/* 只有首批PCM提交成功才记最近；失败/探测均不冒充播放成功。 */
/* 只有同代、从上下台发起、且尚未出 PCM 的失败才自动继续。兼容记录不在这里修改。 */
static void divoom_radio_fail_locked(int32 generation)
{
    divoom_radio_play_queue *queue = &g_divoom_radio.play_queue;
    if (g_divoom_radio.started_generation != generation && queue->step_generation == generation &&
        queue->retry_direction)
    {
        if (!queue->steps)
        {
            queue->steps = queue->retry_direction;
            queue->retry_cursor = queue->position;
            queue->retry_queued = 1;
        }
        queue->retry_direction = 0;
    }
    g_divoom_radio.state.play = DIVOOM_RADIO_PLAY_FAILED;
    g_divoom_radio.state.revision++;
}

static void divoom_radio_mark_playing(uint32 id)
{
    int32 i, index;
    divoom_system_get_lock(g_divoom_radio.lock);
    index = divoom_radio_find(id);
    if (index < 0 || divoom_radio_interrupted(NULL))
    {
        divoom_system_free_lock(g_divoom_radio.lock);
        return;
    }
    for (i = DIVOOM_RADIO_MAX - 1; i > 0; i--)
    {
        if (g_divoom_radio.saved.recent[i - 1] != id)
        {
            continue;
        }
        memmove(&g_divoom_radio.saved.recent[i - 1], &g_divoom_radio.saved.recent[i],
            (DIVOOM_RADIO_MAX - i) * sizeof(uint32));
        g_divoom_radio.saved.recent[DIVOOM_RADIO_MAX - 1] = 0;
    }
    memmove(&g_divoom_radio.saved.recent[1], &g_divoom_radio.saved.recent[0],
        (DIVOOM_RADIO_MAX - 1) * sizeof(uint32));
    g_divoom_radio.saved.recent[0] = id;
    g_divoom_radio.state.recent_total = 0;
    for (i = 0; i < DIVOOM_RADIO_MAX; i++)
    {
        if (g_divoom_radio.saved.recent[i])
        {
            g_divoom_radio.state.recent_total++;
        }
    }
    g_divoom_radio.started_generation = divoom_radio_generation();
    divoom_radio_play_queue *queue = &g_divoom_radio.play_queue;
    if (queue->step_generation == g_divoom_radio.started_generation)
    {
        queue->retry_direction = 0;
        queue->step_origin = queue->position;
    }
    g_divoom_radio.state.play = DIVOOM_RADIO_PLAYING;
    divoom_radio_station_at(index)->recent = ++g_divoom_radio.recent_clock;
    divoom_radio_station_at(index)->compatibility = 1;
    if (strlen(g_divoom_radio.playing_url) < sizeof(divoom_radio_station_at(index)->url))
    {
        snprintf(divoom_radio_station_at(index)->url,
            sizeof(divoom_radio_station_at(index)->url), "%s", g_divoom_radio.playing_url);
    }
    /* 只有PCM已提交才更新恢复记录，连接失败/切换离开不能覆盖最后成功电台。 */
    g_divoom_radio.saved.last_magic = 0x52414432;
    g_divoom_radio.saved.last_station = *divoom_radio_station_at(index);
    g_divoom_radio.catalog_dirty = 1;
    g_divoom_radio.store_dirty = 1;
    g_divoom_radio.state.revision++;
    divoom_system_free_lock(g_divoom_radio.lock);
}

#if defined(_WIN32)
static void divoom_radio_play_stream(const char *url, uint32 id)
{
    AVFormatContext *format = NULL;
    AVCodecContext *codec = NULL;
    AVPacket *packet = NULL;
    AVFrame *frame = NULL;
    SwrContext *resample = NULL;
    AVChannelLayout stereo = AV_CHANNEL_LAYOUT_STEREO;
    AVDictionary *options = NULL;
    void *output = NULL;
    uint8 *pcm = NULL;
    int32 stream = -1, started = 0;
    int32 pcm_capacity = 32768;
    if (divoom_radio_interrupted(NULL))
    {
        goto done;
    }
    format = avformat_alloc_context();
    if (!format)
    {
        goto done;
    }
    format->interrupt_callback.callback = divoom_radio_interrupted;
    g_divoom_radio.deadline = divoom_system_get_tickms() + DIVOOM_RADIO_IO_MS;
    av_dict_set(&options, "rw_timeout", "10000000", 0);
    av_dict_set(&options, "icy", "1", 0);
    av_dict_set(&options, "tls_verify", "1", 0);
    av_dict_set(&options, "protocol_whitelist", "http,https,tcp,tls,crypto", 0);
    if (avformat_open_input(&format, url, NULL, &options) < 0 || avformat_find_stream_info(format, NULL) < 0)
    {
        goto done;
    }
    stream = av_find_best_stream(format, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if (stream < 0)
    {
        goto done;
    }
    /* 播放与目录共用能力边界，不能让PC的全格式FFmpeg冒充设备支持。 */
    if ((format->streams[stream]->codecpar->codec_id != AV_CODEC_ID_MP3 &&
        format->streams[stream]->codecpar->codec_id != AV_CODEC_ID_AAC) ||
        (strcmp(format->iformat->name, "mp3") && strcmp(format->iformat->name, "aac")))
    {
        goto done;
    }
    codec = avcodec_alloc_context3(avcodec_find_decoder(format->streams[stream]->codecpar->codec_id));
    if (!codec || avcodec_parameters_to_context(codec, format->streams[stream]->codecpar) < 0 ||
        avcodec_open2(codec, codec->codec, NULL) < 0)
    {
        goto done;
    }
    if (swr_alloc_set_opts2(&resample, &stereo, AV_SAMPLE_FMT_S16, 48000,
        &codec->ch_layout, codec->sample_fmt, codec->sample_rate, 0, NULL) < 0 || swr_init(resample) < 0)
    {
        goto done;
    }
    output = sim_ai_voice_output_open();
    frame = av_frame_alloc();
    packet = av_packet_alloc();
    pcm = divoom_system_malloc(pcm_capacity * 4);
    if (!output || !frame || !packet || !pcm)
    {
        goto done;
    }
    while (!divoom_radio_interrupted(NULL))
    {
        int32 samples, offset;
        g_divoom_radio.deadline = divoom_system_get_tickms() + DIVOOM_RADIO_IO_MS;
        if (av_read_frame(format, packet) < 0)
        {
            break;
        }
        /* ICY元数据随流更新，目录快照不会覆盖正在播放的真实曲目。 */
        {
            AVDictionaryEntry *metadata = av_dict_get(format->metadata, "StreamTitle", NULL, 0);
            char icy_title[192];
            const char *title = metadata ? metadata->value : NULL;
            if (!title || !title[0])
            {
                const char *start, *end;
                metadata = av_dict_get(format->metadata, "icy_metadata_packet", NULL, 0);
                start = metadata && metadata->value ? strstr(metadata->value, "StreamTitle='") : NULL;
                if (start)
                {
                    start += 13;
                    end = strstr(start, "';");
                    if (end)
                    {
                        size_t length = (size_t)(end - start);
                        if (length >= sizeof(icy_title))
                        {
                            length = sizeof(icy_title) - 1;
                        }
                        memcpy(icy_title, start, length);
                        icy_title[length] = '\0';
                        title = icy_title;
                    }
                }
            }
            if (title && title[0])
            {
                divoom_system_get_lock(g_divoom_radio.lock);
                if (!divoom_radio_interrupted(NULL) &&
                    strcmp(g_divoom_radio.state.live_track, title) != 0)
                {
                    snprintf(g_divoom_radio.state.live_track, sizeof(g_divoom_radio.state.live_track),
                        "%s", title);
                    g_divoom_radio.state.revision++;
                }
                divoom_system_free_lock(g_divoom_radio.lock);
            }
        }
        if (packet->stream_index == stream && avcodec_send_packet(codec, packet) >= 0)
        {
            while (avcodec_receive_frame(codec, frame) == 0)
            {
                samples = swr_convert(resample, &pcm, pcm_capacity,
                    (const uint8_t **)frame->extended_data, frame->nb_samples);
                /* 仿真同样跟随统一音源音量；每块仅缩放一次，背压重试不重复衰减。 */
                {
                    int32 sample;
                    uint32 volume = divoom_music_ctrl_get_volume();
                    int16 *stereo = (int16 *)pcm;
                    uint32 amplitude = volume * volume;
                    for (sample = 0; sample < samples * 2; sample++)
                    {
                        stereo[sample] = (int16)((int32)stereo[sample] * (int32)amplitude / 10000);
                    }
                }
                for (offset = 0; offset < samples && !divoom_radio_interrupted(NULL);)
                {
                    int32 frames = samples - offset > 1024 ? 1024 : samples - offset;
                    int32 written;
                    /* 极短提交与切台共用锁；取消后旧代PCM不能再进入系统队列。 */
                    divoom_system_get_lock(g_divoom_radio.lock);
                    written = divoom_radio_interrupted(NULL) ? -1 :
                        sim_ai_voice_output_write(output, pcm + offset * 4, frames);
                    divoom_system_free_lock(g_divoom_radio.lock);
                    if (written < 0)
                    {
                        goto done;
                    }
                    if (!written)
                    {
                        divoom_system_delay_ms(5);
                        continue;
                    }
                    if (!started)
                    {
                        divoom_radio_mark_playing(id);
                        started = 1;
                    }
                    offset += written;
                }
                av_frame_unref(frame);
            }
        }
        av_packet_unref(packet);
    }
done:
    if (output)
    {
        sim_ai_voice_output_close(output);
    }
    divoom_system_free(pcm);
    av_dict_free(&options);
    swr_free(&resample);
    av_frame_free(&frame);
    av_packet_free(&packet);
    avcodec_free_context(&codec);
    avformat_close_input(&format);
}
#else

/* 复用真机网络MP3/AAC解码和NETWORK音频策略；不在LVGL里停流等待。 */
static void divoom_radio_play_stream(const char *url, uint32 id)
{
    uint8 started = 0;
    divoom_net_audio_state_t previous = DIVOOM_NET_AUDIO_IDLE;
    if (divoom_radio_interrupted(NULL) || divoom_net_audio_play(url,
        divoom_music_ctrl_get_volume(), NULL, NULL) != 0)
    {
        return;
    }
    while (!divoom_radio_interrupted(NULL))
    {
        divoom_net_audio_state_t state = divoom_net_audio_get_state();
        if (state == DIVOOM_NET_AUDIO_IDLE)
        {
            break;
        }
        if (state != previous && (state == DIVOOM_NET_AUDIO_CONNECTING || state == DIVOOM_NET_AUDIO_RECONNECTING ||
            state == DIVOOM_NET_AUDIO_BUFFERING))
        {
            /* 不抢在Cdx的15秒探测/重试之前强停；每次新尝试仍有独立截止时间。 */
            g_divoom_radio.deadline = divoom_system_get_tickms() + DIVOOM_RADIO_DEVICE_CONNECT_MS;
        }
        if (state == DIVOOM_NET_AUDIO_PLAYING)
        {
            g_divoom_radio.deadline = divoom_system_get_tickms() + DIVOOM_RADIO_IO_MS;
            if (!started)
            {
                divoom_radio_mark_playing(id);
                started = 1;
            }
        }
        /* 重连时如实显示CONNECTING；恢复供PCM后再显示PLAYING，不重复写最近。 */
        divoom_system_get_lock(g_divoom_radio.lock);
        if (divoom_radio_generation() == g_divoom_radio.active_generation)
        {
            divoom_radio_play play = state == DIVOOM_NET_AUDIO_PLAYING ? DIVOOM_RADIO_PLAYING : DIVOOM_RADIO_CONNECTING;
            if (g_divoom_radio.state.play != play)
            {
                g_divoom_radio.state.play = play;
                g_divoom_radio.state.revision++;
            }
        }
        divoom_system_free_lock(g_divoom_radio.lock);
        previous = state;
        /* 在低优先级电台控制任务处理频谱，解码和 I2S 线程只负责供数。 */
        divoom_eq_noise_mic_fft_poll_radio();
        divoom_system_delay_ms(20);
    }
    divoom_net_audio_stop();
    divoom_eq_noise_mic_fft_poll_radio();
}

#endif


/* 统一音源切换/语音调度可能等待UART；必须在worker调用，且不持电台锁。
 * 控制器可持music锁调用set_play；反向调用只允许在释放radio锁之后。 */
static uint8 divoom_radio_apply_source_request(void)
{
#if DIVOOM_RADIO_MUSIC_SOURCE
    uint8 request;
    int32 generation;
    divoom_system_get_lock(g_divoom_radio.lock);
    request = g_divoom_radio.source_request;
    generation = divoom_radio_generation();
    g_divoom_radio.source_request = 0;
    divoom_system_free_lock(g_divoom_radio.lock);
    if (request)
    {
        /* 只传值；控制器在切源提交边界再次验证，radio持锁时禁止反调music。 */
        divoom_music_ctrl_radio_source_request(generation, request == 1);
    }
    return request != 0;
#else
    return 0;
#endif
}

static void *divoom_radio_player_worker(void *unused)
{
    char *playlist;
    const char *cursor;
    divoom_radio_station *station = divoom_system_malloc(sizeof(*station));
    (void)unused;
    if (!station)
    {
        return NULL;
    }
    for (;;)
    {
        uint32 id;
        uint8 manual;
        int32 index, generation;
        char url[1024];
        if (divoom_radio_apply_source_request())
        {
            continue;
        }
        divoom_system_get_lock(g_divoom_radio.lock);
        /* 取播放目标时再次确认选源许可，覆盖两次取锁之间的新选台。 */
        if (g_divoom_radio.source_request)
        {
            divoom_system_free_lock(g_divoom_radio.lock);
            continue;
        }
        id = g_divoom_radio.request_id;
        g_divoom_radio.request_id = 0;
        generation = divoom_radio_generation();
        index = divoom_radio_find(id);
        manual = id && id == g_divoom_radio.manual_id;
        if (index >= 0)
        {
            *station = *divoom_radio_station_at(index);
        }
        divoom_system_free_lock(g_divoom_radio.lock);
        if (id && index >= 0)
        {
            uint8 tried = 0;
            g_divoom_radio.active_generation = generation;
            g_divoom_radio.active_id = id;
            g_divoom_radio.deadline = divoom_system_get_tickms() + DIVOOM_RADIO_IO_MS;
            const char *test_url = divoom_radio_env("DIVOOM_SIM_RADIO_STREAM_URL");
            if (test_url && test_url[0])
            {
                snprintf(g_divoom_radio.playing_url, sizeof(g_divoom_radio.playing_url), "%s", test_url);
                divoom_radio_play_stream(test_url, id);
                tried = 1;
            }
            else if (station->url[0])
            {
                snprintf(g_divoom_radio.playing_url, sizeof(g_divoom_radio.playing_url), "%s", station->url);
                divoom_radio_play_stream(station->url, id);
                tried = manual;
            }
            /* 缓存线路断开才刷新PLS；真实播放可以重连，格式检测只查新线路。 */
            if (!tried && divoom_radio_generation() == generation)
            {
                g_divoom_radio.deadline = divoom_system_get_tickms() + DIVOOM_RADIO_IO_MS;
                playlist = divoom_radio_playlist(id, 1);
                cursor = playlist;
                while (divoom_radio_generation() == generation &&
                    (cursor = divoom_radio_next_url(cursor, url, sizeof(url))) != NULL)
                {
                    if ((!strncmp(url, "http://", 7) || !strncmp(url, "https://", 8)) &&
                        strcmp(url, station->url) != 0)
                    {
                        g_divoom_radio.deadline = divoom_system_get_tickms() + DIVOOM_RADIO_IO_MS;
                        snprintf(g_divoom_radio.playing_url, sizeof(g_divoom_radio.playing_url), "%s", url);
                        divoom_radio_play_stream(url, id);
                    }
                }
                divoom_system_free(playlist);
            }
            divoom_system_get_lock(g_divoom_radio.lock);
            if (divoom_radio_generation() == generation)
            {
                divoom_radio_fail_locked(generation);
            }
            divoom_system_free_lock(g_divoom_radio.lock);
        }
        divoom_system_delay_ms(10);
    }
}

static void divoom_radio_persist(void)
{
    divoom_radio_store next;
    uint32 favorite;
    if (g_divoom_radio.persist_retry && (int32)(divoom_system_get_tickms() - g_divoom_radio.persist_retry) < 0)
    {
        return;
    }
    int32 dirty, catalog_dirty, catalog_count, slot = -1, index, ok, adding = 0;
    divoom_system_get_lock(g_divoom_radio.lock);
    next = g_divoom_radio.saved;
    dirty = g_divoom_radio.store_dirty;
    catalog_dirty = g_divoom_radio.menu_active && g_divoom_radio.catalog_dirty;
    catalog_count = g_divoom_radio.state.count;
    if (catalog_dirty)
    {
        g_divoom_radio.catalog_dirty = 0;
    }
    g_divoom_radio.store_dirty = 0;
    favorite = g_divoom_radio.favorite_id;
    g_divoom_radio.favorite_id = 0;
    index = divoom_radio_find(favorite);
    if (index >= 0)
    {
        adding = !divoom_radio_station_at(index)->favorite;
        for (int32 i = 0; i < DIVOOM_RADIO_MAX; i++)
        {
            if (next.ids[i] == favorite)
            {
                next.ids[i] = 0;
            }
            if (!next.ids[i] && slot < 0)
            {
                slot = i;
            }
        }
        if (adding && slot < 0)
        {
            /* 收藏已满时保留旧配置，不发布无法在重启后恢复的虚假收藏状态。 */
            g_divoom_radio.state.favorite_error++;
            g_divoom_radio.state.revision++;
            favorite = 0;
        }
        else
        {
            if (adding)
            {
                next.ids[slot] = favorite;
            }
            dirty = 1;
        }
    }
    divoom_system_free_lock(g_divoom_radio.lock);
    if (dirty)
    {
        ok = divoom_radio_write(&next, sizeof(next), 0);
        if (ok && next.last_magic == 0x52414432)
        {
            /* 后台只保存一台轻量恢复记录，不加载兼容JSON。 */
            divoom_radio_browse_station_save(&next.last_station);
        }
        divoom_system_get_lock(g_divoom_radio.lock);
        index = divoom_radio_find(favorite);
        if (ok && favorite && index >= 0)
        {
            /* 最近可能在落盘期间更新，只合并本次收藏字段。 */
            memcpy(g_divoom_radio.saved.ids, next.ids, sizeof(next.ids));
            divoom_radio_station_at(index)->favorite = adding;
            g_divoom_radio.state.favorite_total += adding ? 1 : -1;
            g_divoom_radio.catalog_dirty = 1;
        }
        if (!ok)
        {
            g_divoom_radio.store_dirty = 1;
            g_divoom_radio.persist_retry = divoom_system_get_tickms() + 1000;
            if (favorite)
            {
                g_divoom_radio.state.favorite_error++;
            }
        }
        g_divoom_radio.state.revision++;
        divoom_system_free_lock(g_divoom_radio.lock);
    }
    if (catalog_dirty)
    {
        divoom_radio_cache_load();
        divoom_radio_station *station = divoom_system_malloc(sizeof(*station));
        if (!station || !g_divoom_radio.cache)
        {
            /* 页面在加载前退出或分配失败时，保留已播放确认，重进后再落盘。 */
            divoom_system_get_lock(g_divoom_radio.lock);
            g_divoom_radio.catalog_dirty = 1;
            divoom_system_free_lock(g_divoom_radio.lock);
        }
        if (station)
        {
            for (int32 i = 0; i < catalog_count; i++)
            {
                divoom_system_get_lock(g_divoom_radio.lock);
                *station = *divoom_radio_station_at(i);
                divoom_system_free_lock(g_divoom_radio.lock);
                divoom_radio_browse_station_save(station);
                if (station->compatibility == 1)
                {
                    divoom_cJSON *known = divoom_radio_cached(station->id);
                    if (!known || divoom_radio_number(known, "status") != 1 ||
                        strcmp(divoom_radio_string(known, "url"), station->url))
                    {
                        if (!divoom_radio_cache_put(station, NULL))
                        {
                            divoom_system_get_lock(g_divoom_radio.lock);
                            g_divoom_radio.catalog_dirty = 1;
                            divoom_system_free_lock(g_divoom_radio.lock);
                        }
                    }
                }
            }
            divoom_system_free(station);
        }
    }
    if (g_divoom_radio.cache_pending_text && !g_divoom_radio.cache &&
        g_divoom_radio.cache_pending_version == g_divoom_radio.compat_version)
    {
        /* 失败重试只写已序列化文本；后台播放无需为重试重建JSON树。 */
        if (divoom_radio_write(g_divoom_radio.cache_pending_text, strlen(g_divoom_radio.cache_pending_text), 1))
        {
            divoom_system_free(g_divoom_radio.cache_pending_text);
            g_divoom_radio.cache_pending_text = NULL;
            divoom_radio_compat_saved();
        }
        else
        {
            g_divoom_radio.persist_retry = divoom_system_get_tickms() + 1000;
        }
    }
    if (g_divoom_radio.compat_dirty && divoom_radio_menu_active())
    {
        divoom_radio_cache_load();
    }
    divoom_radio_cache_release();
}

/* 逐份校验userdata配置；主文件策略不匹配时，也必须尝试有效备份。 */
static divoom_cJSON *divoom_radio_cache_read(const char *path)
{
    const char *parse_end = NULL;
    g_divoom_radio.cache_migrated = 0;
    char *text = divoom_radio_read_checked(path, &g_divoom_radio.cache_read_failed);
    divoom_cJSON *root = text ? divoom_cJSON_ParseWithOpts(text, &parse_end, 1) : NULL;
    if (text && !root && !parse_end)
    {
        /* 本项目解析器语法错误会设置位置，申请失败则位置为空；使用局部位置避免全局ep竞争。 */
        g_divoom_radio.cache_read_failed = 1;
    }
    divoom_cJSON *records = root ? divoom_cJSON_GetObjectItem(root, "stations") : NULL;
    divoom_system_free(text);
    uint8 legacy = !strcmp(divoom_radio_string(root, "policy"), DIVOOM_RADIO_LEGACY_POLICY);
    if ((!legacy && strcmp(divoom_radio_string(root, "policy"), DIVOOM_RADIO_POLICY) != 0) ||
        !records || (records->type & 0xff) != divoom_cJSON_Array)
    {
        divoom_cJSON_Delete(root);
        return NULL;
    }
    if (legacy)
    {
        divoom_cJSON *policy = divoom_cJSON_CreateString(DIVOOM_RADIO_POLICY);
        if (policy)
        {
            divoom_cJSON_ReplaceItemInObject(root, "policy", policy);
        }
        if (!policy || !policy->string)
        {
            g_divoom_radio.cache_read_failed = 1;
            divoom_cJSON_Delete(root);
            return NULL;
        }
        g_divoom_radio.cache_migrated = 1;
    }
    return root;
}

/* 新能力只使旧版的部分否定失效；已确认支持及明确MIME/协议否定继续复用。 */
static void divoom_radio_compat_upgrade_hls(void)
{
    int32 count = divoom_queue_items(g_divoom_radio.compat_records);
    for (int32 i = 0; i < count; i++)
    {
        divoom_radio_compat *record = divoom_queue_get_index_item(g_divoom_radio.compat_records, i);
        if (divoom_radio_mime_rejected(record->data))
        {
            continue;
        }
        uint8 changed = 0;
        uint8 http = !strncmp(divoom_radio_compat_url(record), "http://", 7) ||
            !strncmp(divoom_radio_compat_url(record), "https://", 8);
        uint8 *route = (uint8 *)divoom_radio_compat_routes(record);
        for (uint32 j = 0; j < record->route_count; j++)
        {
            if (!strncmp((char *)route + 1, "http://", 7) || !strncmp((char *)route + 1, "https://", 8))
            {
                http = 1;
                if (*route == 2)
                {
                    *route = 0;
                    changed = 1;
                }
            }
            route += strlen((char *)route + 1) + 2;
        }
        if (record->status == 2 && (http || (!record->route_count && !divoom_radio_compat_url(record)[0])))
        {
            record->status = 0;
            changed = 1;
        }
        if (changed)
        {
            record->dirty = 1;
            g_divoom_radio.compat_dirty = 1;
            g_divoom_radio.compat_version++;
        }
    }
}

/* 兼容结果仅来自/userdata；缺失时只建立空结构，不把电台数据编译进固件。 */
static void divoom_radio_cache_load(void)
{
    char path[1024], backup[1032];
    uint8 imported = 0;
    if (g_divoom_radio.cache || !divoom_radio_menu_active())
    {
        return;
    }
    g_divoom_radio.cache_apply_failed = 0;
    g_divoom_radio.cache_migrated = 0;
    if (g_divoom_radio.cache_pending_text)
    {
        /* 新更新必须合并未提交结果，不能回读旧文件覆盖失败事务。 */
        g_divoom_radio.cache = divoom_cJSON_Parse(g_divoom_radio.cache_pending_text);
        if (!g_divoom_radio.cache)
        {
            return;
        }
        divoom_system_free(g_divoom_radio.cache_pending_text);
        g_divoom_radio.cache_pending_text = NULL;
        g_divoom_radio.cache_dirty = 1;
    }
    else
    {
        divoom_radio_path(path, sizeof(path), 1, 0);
        g_divoom_radio.cache = divoom_radio_cache_read(path);
        if (g_divoom_radio.cache_read_failed)
        {
            return;
        }
        g_divoom_radio.cache_dirty = g_divoom_radio.cache_migrated;
        g_divoom_radio.cache_from_backup = 0;
        if (!g_divoom_radio.cache)
        {
            snprintf(backup, sizeof(backup), "%s.bak", path);
            g_divoom_radio.cache = divoom_radio_cache_read(backup);
            if (g_divoom_radio.cache_read_failed)
            {
                return;
            }
            g_divoom_radio.cache_from_backup = g_divoom_radio.cache != NULL;
            g_divoom_radio.cache_dirty = 1;
        }
        g_divoom_radio.cache_from_disk = g_divoom_radio.cache != NULL;
        if (!g_divoom_radio.cache)
        {
            /* 这里只含配置结构；未知电台仍可展示，后台按需检测后原子写回文件。 */
            g_divoom_radio.cache = divoom_cJSON_Parse(
                "{\"policy\":\"" DIVOOM_RADIO_POLICY "\",\"schema\":1,\"stations\":[]}");
        }
    }
    if (g_divoom_radio.cache && !g_divoom_radio.compat_loaded)
    {
        if (!divoom_radio_compat_import())
        {
            divoom_cJSON_Delete(g_divoom_radio.cache);
            g_divoom_radio.cache = NULL;
            return;
        }
        imported = 1;
    }
    if (imported && g_divoom_radio.cache_migrated)
    {
        divoom_radio_compat_upgrade_hls();
    }
    if (g_divoom_radio.cache)
    {
        g_divoom_radio.compat_dirty |= g_divoom_radio.cache_dirty;
        int32 count = divoom_queue_items(g_divoom_radio.compat_records);
        for (int32 i = 0; i < count; i++)
        {
            divoom_radio_compat *record = divoom_queue_get_index_item(g_divoom_radio.compat_records, i);
            /* 主文件坏/缺失时，旧备份不能覆盖RAM中已经提交过的较新确认。 */
            if ((record->dirty || g_divoom_radio.cache_from_backup || !g_divoom_radio.cache_from_disk ||
                g_divoom_radio.cache_migrated) &&
                !divoom_radio_cache_apply(record))
            {
                break;
            }
        }
        g_divoom_radio.cache_dirty |= g_divoom_radio.compat_dirty;
    }
    divoom_system_get_lock(g_divoom_radio.lock);
    g_divoom_radio.cache_live = g_divoom_radio.cache != NULL;
    g_divoom_radio.cache_loads += g_divoom_radio.cache_live;
    divoom_system_free_lock(g_divoom_radio.lock);
}

/* 每次更新事务收尾必须调用；即使存储失败，也不能让JSON树跨事务/跨页面驻留。 */
static void divoom_radio_cache_release(void)
{
    if (!g_divoom_radio.cache)
    {
        return;
    }
    if (g_divoom_radio.cache_dirty)
    {
        char *text = g_divoom_radio.cache_apply_failed ? NULL : divoom_cJSON_PrintUnformatted(g_divoom_radio.cache);
        if (text)
        {
            divoom_system_free(g_divoom_radio.cache_pending_text);
            g_divoom_radio.cache_pending_text = NULL;
            if (!divoom_radio_write(text, strlen(text), 1))
            {
                g_divoom_radio.cache_pending_text = text;
                g_divoom_radio.cache_pending_version = g_divoom_radio.compat_version;
                g_divoom_radio.persist_retry = divoom_system_get_tickms() + 1000;
            }
            else
            {
                divoom_system_free(text);
                divoom_radio_compat_saved();
            }
        }
        else
        {
            /* 包含否定项和逐线路结果的紧凑记录仍在；释放DOM后完整重试，不从显示目录猜回。 */
            g_divoom_radio.compat_dirty = 1;
            g_divoom_radio.persist_retry = divoom_system_get_tickms() + 1000;
        }
    }
    divoom_cJSON_Delete(g_divoom_radio.cache);
    g_divoom_radio.cache = NULL;
    g_divoom_radio.cache_dirty = 0;
    divoom_system_get_lock(g_divoom_radio.lock);
    g_divoom_radio.cache_live = 0;
    g_divoom_radio.cache_frees++;
    divoom_system_free_lock(g_divoom_radio.lock);
}

/* 单轮目录事务：后台只保留播放记录，空闲和页面退出时都没有配置树。 */
/* 目录worker消费恢复意图；锁内只复制一台记录，不解析兼容JSON、不执行网络。 */
static uint8 divoom_radio_resume_pending(void)
{
    uint8 needs_directory = 0;
    divoom_system_get_lock(g_divoom_radio.lock);
    if (g_divoom_radio.resume_pending && !g_divoom_radio.player_task)
    {
        g_divoom_radio.resume_pending = 0;
        g_divoom_radio.state.play = DIVOOM_RADIO_PLAY_FAILED;
        g_divoom_radio.state.revision++;
    }
    else if (g_divoom_radio.resume_pending)
    {
        divoom_radio_compat *known = divoom_radio_compat_find(g_divoom_radio.compat_records,
            g_divoom_radio.saved.last_station.id);
        uint8 usable = g_divoom_radio.saved.last_magic == 0x52414432 &&
            g_divoom_radio.saved.last_station.id && (!known || known->status != 2);
        int32 index = usable ? divoom_radio_find(g_divoom_radio.saved.last_station.id) : -1;
        uint8 pin = usable && index < 0 && g_divoom_radio.browse_requested;
        if (usable && index < 0)
        {
            /* 恢复旧的手动URL必须复用现有手动槽，不能在失败/恢复循环中不断扩张目录。 */
            if (pin)
            {
                index = divoom_radio_find(g_divoom_radio.browse_pinned);
            }
            else if (g_divoom_radio.saved.last_station.id & 0x80000000u)
            {
                /* 尚未建立浏览来源时，保留原手动槽的复用方式。 */
                index = divoom_radio_find(g_divoom_radio.manual_id);
            }
            if (index < 0)
            {
                index = divoom_radio_directory_append();
                if (index >= 0 && pin)
                {
                    g_divoom_radio.target_count++;
                    if (g_divoom_radio.target_count > DIVOOM_RADIO_MAX + 1)
                    {
                        g_divoom_radio.target_count = DIVOOM_RADIO_MAX + 1;
                    }
                }
            }
        }
        if (usable && index >= 0)
        {
            *divoom_radio_station_at(index) = g_divoom_radio.saved.last_station;
            divoom_radio_station_at(index)->favorite = 0;
            divoom_radio_station_at(index)->retry_after = 0;
            for (int32 i = 0; i < DIVOOM_RADIO_MAX; i++)
            {
                divoom_radio_station_at(index)->favorite |=
                    g_divoom_radio.saved.ids[i] == g_divoom_radio.saved.last_station.id;
            }
            /* 显式URL的稳定ID最高位为1，恢复后仍只使用该URL，不查询目录PLS。 */
            if (g_divoom_radio.saved.last_station.id & 0x80000000u)
            {
                g_divoom_radio.manual_id = g_divoom_radio.saved.last_station.id;
            }
        }
        uint8 restored_saved = usable && index >= 0;
        if (index < 0 && g_divoom_radio.state.count > 0)
        {
            for (int32 i = 0; i < g_divoom_radio.state.count; i++)
            {
                if (divoom_radio_station_at(i)->compatibility != 2)
                {
                    index = i;
                    break;
                }
            }
        }
        if (index >= 0)
        {
            g_divoom_radio.resume_pending = 0;
            g_divoom_radio.resume_last = 0;
            if (pin && restored_saved)
            {
                /* 恢复记录并不自动属于当前筛选；保留音源槽并仍补足首段6台。 */
                g_divoom_radio.browse_pinned = divoom_radio_station_at(index)->id;
            }
            g_divoom_radio.state.selected = index;
            g_divoom_radio.request_id = divoom_radio_station_at(index)->id;
            /* 首次用默认目录起播时，后台左右键也按这个目录切台。 */
            if (!restored_saved && g_divoom_radio.browse_active.kind <= DIVOOM_RADIO_BROWSE_DOMESTIC)
            {
                divoom_radio_queue_capture(g_divoom_radio.request_id);
            }
            /* MODE恢复最后成功台时同步独立队列游标，取消离开前未消费的上下台意图。 */
            divoom_radio_play_queue *queue = &g_divoom_radio.play_queue;
            int32 queue_position = -1;
            for (int32 i = 0; i < queue->count; i++)
            {
                if (queue->ids[i] == g_divoom_radio.request_id)
                {
                    queue_position = i;
                    break;
                }
            }
            if (queue_position < 0)
            {
                /* 先恢复成功台；没有来源的队列等到首次上下台再按需加载。 */
                uint32 epoch = queue->epoch + 1;
                memset(queue, 0, sizeof(*queue));
                queue->epoch = epoch;
                queue->query.kind = DIVOOM_RADIO_BROWSE_AUTO;
                queue->ids[0] = g_divoom_radio.request_id;
                queue->count = 1;
                queue->total = DIVOOM_RADIO_SOURCE_MAX;
                queue_position = 0;
            }
            queue->position = queue_position;
            queue->steps = 0;
            queue->retry_direction = 0;
            queue->retry_queued = 0;
            queue->wait_for_commit = 0;
            queue->step_serial++;
            queue->step_generation = divoom_radio_generation();
            /* 只恢复一台也允许后台上下台按需补段，不能一直停在初始LOADING/total=0。 */
            g_divoom_radio.state.load = DIVOOM_RADIO_READY;
            g_divoom_radio.state.total = g_divoom_radio.exhausted ?
                g_divoom_radio.state.count : DIVOOM_RADIO_MAX + (g_divoom_radio.browse_pinned != 0);
            g_divoom_radio.state.play = DIVOOM_RADIO_CONNECTING;
            g_divoom_radio.state.revision++;
        }
        else
        {
            needs_directory = 1;
        }
    }
    divoom_system_free_lock(g_divoom_radio.lock);
    return needs_directory;
}

/* 目录worker完成在途事务后回收。保留当前台、播放队列和持久化记录。 */
static void divoom_radio_directory_release_idle(void)
{
    divoom_system_get_lock(g_divoom_radio.lock);
    if (!g_divoom_radio.menu_active &&
        g_divoom_radio.state.play != DIVOOM_RADIO_PLAYING &&
        g_divoom_radio.state.play != DIVOOM_RADIO_CONNECTING &&
        !g_divoom_radio.request_id && !g_divoom_radio.resume_pending &&
        g_divoom_radio.source_request != 1 && !g_divoom_radio.play_queue.steps &&
        !g_divoom_radio.favorite_id && !g_divoom_radio.background_more &&
        (g_divoom_radio.state.count > 1 || g_divoom_radio.pending ||
            g_divoom_radio.state.load != DIVOOM_RADIO_LOADING || g_divoom_radio.source_offset))
    {
        divoom_radio_directory_keep_selected();
        g_divoom_radio.source_offset = 0;
        g_divoom_radio.source_total = DIVOOM_RADIO_SOURCE_MAX;
        g_divoom_radio.exhausted = 0;
        g_divoom_radio.browse_domestic_loaded = 0;
        g_divoom_radio.target_count = DIVOOM_RADIO_PAGE_SIZE + g_divoom_radio.state.count;
        g_divoom_radio.state.total = DIVOOM_RADIO_MAX + g_divoom_radio.state.count;
        g_divoom_radio.state.load = DIVOOM_RADIO_LOADING;
        g_divoom_radio.state.loading_more = 0;
        g_divoom_radio.state.more_error = 0;
        g_divoom_radio.reload = 1;
        g_divoom_radio.more = 0;
        g_divoom_radio.state.revision++;
    }
    divoom_system_free_lock(g_divoom_radio.lock);
}

static void divoom_radio_update(void)
{
    uint8 active, reload, more, background;
    divoom_radio_persist();
    /* 收藏旧目录的提交必须早于替换；浏览代号不触碰切源/播放代号。 */
    divoom_radio_queue_update();
    /* 先恢复已成功的电台；没有记录时才初始化后台默认目录。 */
    uint8 needs_directory = divoom_radio_resume_pending();
    divoom_radio_browse_update();
    if (needs_directory &&
        (g_divoom_radio.browse_active.kind != DIVOOM_RADIO_BROWSE_AUTO || g_divoom_radio.state.load == DIVOOM_RADIO_LOAD_FAILED))
    {
        /* 首次没有历史时只补首段6条；失败结束本次意图，下一次切入仍可重试。 */
        if (g_divoom_radio.browse_active.kind != DIVOOM_RADIO_BROWSE_AUTO)
        {
            divoom_radio_load_directory(1);
        }
        if (divoom_radio_resume_pending())
        {
            divoom_system_get_lock(g_divoom_radio.lock);
            if (g_divoom_radio.resume_pending)
            {
                g_divoom_radio.resume_pending = 0;
                g_divoom_radio.state.play = DIVOOM_RADIO_PLAY_FAILED;
                g_divoom_radio.state.revision++;
            }
            divoom_system_free_lock(g_divoom_radio.lock);
        }
    }
    divoom_system_get_lock(g_divoom_radio.lock);
    active = g_divoom_radio.menu_active && g_divoom_radio.browse_active.kind != DIVOOM_RADIO_BROWSE_AUTO;
    background = !active && g_divoom_radio.background_more;
    g_divoom_radio.background_more = 0;
    reload = active && g_divoom_radio.reload;
    more = (active || background) && g_divoom_radio.more;
    if (active)
    {
        g_divoom_radio.reload = 0;
    }
    if (active || background)
    {
        g_divoom_radio.more = 0;
    }
    divoom_system_free_lock(g_divoom_radio.lock);
    if (reload || more)
    {
        divoom_radio_load_directory(background);
    }
    else if (active)
    {
        divoom_radio_probe();
    }
    divoom_radio_cache_release();
    divoom_radio_directory_release_idle();
}

/* 兼容旧804字节收藏文件；不完整/非法尾记录只丢弃恢复项，保留收藏与最近列表。 */
static void divoom_radio_store_load(void)
{
    char path[1032];
    divoom_radio_store saved = {0};
    void *file;
    size_t prefix = offsetof(divoom_radio_store, last_magic);
    for (int32 attempt = 0; attempt < 2; attempt++)
    {
        divoom_radio_path(path, 1024, 0, 0);
        if (attempt)
        {
            strcat(path, ".bak");
        }
        file = divoom_port_file_open(path, "rb");
        if (!file)
        {
            continue;
        }
        if (divoom_port_file_read(&saved, prefix, 1, file) != 1 || saved.magic != 0x52414431)
        {
            memset(&saved, 0, sizeof(saved));
            divoom_port_file_close(file);
            continue;
        }
        else if (divoom_port_file_read(&saved.last_magic, sizeof(saved) - prefix, 1, file) != 1 ||
            saved.last_magic != 0x52414432 || !saved.last_station.id ||
            !memchr(saved.last_station.name, 0, sizeof(saved.last_station.name)) ||
            !memchr(saved.last_station.genre, 0, sizeof(saved.last_station.genre)) ||
            !memchr(saved.last_station.track, 0, sizeof(saved.last_station.track)) ||
            !memchr(saved.last_station.mt, 0, sizeof(saved.last_station.mt)) ||
            !memchr(saved.last_station.url, 0, sizeof(saved.last_station.url)) ||
            (saved.last_station.url[0] && strncmp(saved.last_station.url, "http://", 7) &&
                strncmp(saved.last_station.url, "https://", 8)) ||
            saved.last_station.compatibility != 1 || divoom_radio_mime_rejected(saved.last_station.mt))
        {
            saved.last_magic = 0;
            memset(&saved.last_station, 0, sizeof(saved.last_station));
        }
        divoom_port_file_close(file);
        /* 合法旧前缀优先；尾部坏只清尾部，不回滚新收藏。主缺失/坏前缀才用bak。 */
        g_divoom_radio.store_from_backup = attempt != 0;
        break;
    }
    saved.magic = 0x52414431;
    divoom_system_get_lock(g_divoom_radio.lock);
    g_divoom_radio.saved = saved;
    g_divoom_radio.store_dirty |= g_divoom_radio.store_from_backup;
    g_divoom_radio.recent_clock = DIVOOM_RADIO_MAX;
    for (int32 i = 0; i < DIVOOM_RADIO_MAX; i++)
    {
        g_divoom_radio.state.favorite_total += saved.ids[i] != 0;
        g_divoom_radio.state.recent_total += saved.recent[i] != 0;
    }
    divoom_system_free_lock(g_divoom_radio.lock);
}

static void *divoom_radio_worker(void *unused)
{
    (void)unused;
    /* 任务可能抢先执行，等创建方发布同一把锁后再访问目录状态。 */
    while (!divoom_radio_has_lock())
    {
        divoom_system_delay_ms(1);
    }
    divoom_radio_store_load();
    divoom_system_get_lock(g_divoom_radio.lock);
    g_divoom_radio.player_task = divoom_system_create_task(divoom_radio_player_worker, "net_radio_player", NULL, DIVOOM_RADIO_WORKER_STACK);
    divoom_system_free_lock(g_divoom_radio.lock);
    for (;;)
    {
        divoom_radio_update();
        divoom_system_delay_ms(20);
    }
}

int32 divoom_radio_start(void)
{
    for (;;)
    {
        if (divoom_radio_has_lock())
        {
            return 1;
        }
        int32 idle = 0;
        if (__atomic_compare_exchange_n(&g_divoom_radio.initializing, &idle, 1, 0,
                __ATOMIC_ACQ_REL, __ATOMIC_RELAXED))
        {
            break;
        }
        divoom_system_delay_ms(1);
    }
    void *lock = divoom_radio_has_lock() ? NULL : divoom_system_create_lock();
    if (lock)
    {
        g_divoom_radio.state.selected = -1;
        g_divoom_radio.state.load = DIVOOM_RADIO_LOADING;
        g_divoom_radio.browse_active.kind = DIVOOM_RADIO_BROWSE_AUTO;
        g_divoom_radio.browse_desired.kind = DIVOOM_RADIO_BROWSE_AUTO;
        g_divoom_radio.source_total = DIVOOM_RADIO_SOURCE_MAX;
        g_divoom_radio.target_count = DIVOOM_RADIO_PAGE_SIZE;
        g_divoom_radio.reload = 1;
        g_divoom_radio.task = divoom_system_create_task(divoom_radio_worker, "net_radio", NULL, DIVOOM_RADIO_WORKER_STACK);
        if (g_divoom_radio.task)
        {
            __atomic_store_n(&g_divoom_radio.lock, lock, __ATOMIC_RELEASE);
        }
        else
        {
            divoom_system_destroy_lock(lock);
        }
    }
    __atomic_store_n(&g_divoom_radio.initializing, 0, __ATOMIC_RELEASE);
    return divoom_radio_has_lock();
}

void divoom_radio_set_menu_active(uint8 active)
{
    if (!divoom_radio_has_lock())
    {
        return;
    }
    divoom_system_get_lock(g_divoom_radio.lock);
    g_divoom_radio.menu_active = active != 0;
    if (!active)
    {
        g_divoom_radio.hint_id = 0;
    }
    divoom_system_free_lock(g_divoom_radio.lock);
}

void divoom_radio_poll(divoom_radio_snapshot *out)
{
    if (!divoom_radio_has_lock())
    {
        memset(out, 0, sizeof(*out));
        out->load = DIVOOM_RADIO_LOAD_FAILED;
        out->selected = -1;
        return;
    }
    divoom_system_get_lock(g_divoom_radio.lock);
    memset(out, 0, sizeof(*out));
    for (int32 i = 0; i < g_divoom_radio.state.count; i++)
    {
        out->stations[i] = *divoom_radio_station_at(i);
    }
    out->count = g_divoom_radio.state.count;
    out->total = g_divoom_radio.state.total;
    out->favorite_total = g_divoom_radio.state.favorite_total;
    out->recent_total = g_divoom_radio.state.recent_total;
    out->loading_more = g_divoom_radio.state.loading_more;
    out->more_error = g_divoom_radio.state.more_error;
    out->page_requests = g_divoom_radio.state.page_requests;
    out->scanned = g_divoom_radio.state.scanned;
    out->filtered = g_divoom_radio.state.filtered;
    out->probe_requests = g_divoom_radio.state.probe_requests;
    out->cache_hits = g_divoom_radio.state.cache_hits;
    out->selected = g_divoom_radio.state.selected;
    out->load = g_divoom_radio.state.load;
    out->play = g_divoom_radio.state.play;
    out->revision = g_divoom_radio.state.revision;
    out->favorite_error = g_divoom_radio.state.favorite_error;
    out->offline = g_divoom_radio.state.offline;
    memcpy(out->live_track, g_divoom_radio.state.live_track, sizeof(out->live_track));
    divoom_system_free_lock(g_divoom_radio.lock);
}

uint8 divoom_radio_poll_view(divoom_radio_view *out, uint32 revision, uint8 force)
{
    if (!divoom_radio_has_lock())
    {
        if (!force)
        {
            return 0;
        }
        memset(out, 0, sizeof(*out));
        out->load = DIVOOM_RADIO_LOAD_FAILED;
        out->selected = -1;
        return 1;
    }

    divoom_system_get_lock(g_divoom_radio.lock);
    const divoom_radio_state *state = &g_divoom_radio.state;
    if (!force && revision == state->revision)
    {
        divoom_system_free_lock(g_divoom_radio.lock);
        return 0;
    }

    memset(out, 0, sizeof(*out));
    out->count = state->count;
    out->total = state->total;
    out->favorite_total = state->favorite_total;
    out->recent_total = state->recent_total;
    out->loading_more = state->loading_more;
    out->more_error = state->more_error;
    out->page_requests = state->page_requests;
    out->selected = state->selected >= 0 && state->selected < state->count ? state->selected : -1;
    out->load = state->load;
    out->play = state->play;
    out->revision = state->revision;
    out->favorite_error = state->favorite_error;
    out->offline = state->offline;
    out->selected_name[0] = '\0';
    if (out->selected >= 0)
    {
        memcpy(out->selected_name, divoom_radio_station_at(out->selected)->name, sizeof(out->selected_name));
    }
    memcpy(out->live_track, state->live_track, sizeof(out->live_track));
    for (int32 i = 0; i < state->count; i++)
    {
        const divoom_radio_station *station = divoom_radio_station_at(i);
        out->stations[i].id = station->id;
        out->stations[i].recent = station->recent;
        out->stations[i].favorite = station->favorite;
    }
    divoom_system_free_lock(g_divoom_radio.lock);
    return 1;
}

uint8 divoom_radio_copy_rows(uint32 revision, const int32 *indices, int32 count, divoom_radio_row *out)
{
    if (!divoom_radio_has_lock() || count <= 0 || count > DIVOOM_RADIO_VISIBLE_ROWS)
    {
        return 0;
    }

    divoom_system_get_lock(g_divoom_radio.lock);
    const divoom_radio_state *state = &g_divoom_radio.state;
    if (revision != state->revision)
    {
        divoom_system_free_lock(g_divoom_radio.lock);
        return 0;
    }
    for (int32 i = 0; i < count; i++)
    {
        if (indices[i] < 0 || indices[i] >= state->count)
        {
            divoom_system_free_lock(g_divoom_radio.lock);
            return 0;
        }
        const divoom_radio_station *station = divoom_radio_station_at(indices[i]);
        memcpy(out[i].name, station->name, sizeof(out[i].name));
        memcpy(out[i].genre, station->genre, sizeof(out[i].genre));
        memcpy(out[i].track, station->track, sizeof(out[i].track));
        out[i].bitrate = station->bitrate;
    }
    divoom_system_free_lock(g_divoom_radio.lock);
    return 1;
}

char* divoom_radio_copy_selected_name(void)
{
    if (!divoom_radio_has_lock())
    {
        return NULL;
    }
    divoom_system_get_lock(g_divoom_radio.lock);
    int32 index = g_divoom_radio.state.selected;
    char *name = NULL;
    if (index >= 0 && index < g_divoom_radio.state.count)
    {
        name = g_divoom_radio.selected_name;
        memcpy(name, divoom_radio_station_at(index)->name, sizeof(g_divoom_radio.selected_name));
    }
    divoom_system_free_lock(g_divoom_radio.lock);
    return name;
}

static void divoom_radio_select_locked(uint32 id, uint8 new_request)
{
    int32 index = divoom_radio_find(id);
    if (!id || index >= 0)
    {
        if (id && new_request)
        {
            g_divoom_radio.startup_request++;
        }
        g_divoom_radio.resume_pending = 0;
        g_divoom_radio.resume_last = 0;
        divoom_radio_next_generation();
#if DIVOOM_RADIO_MUSIC_SOURCE
        g_divoom_radio.source_request = id ? 1 : 2;
#endif
        g_divoom_radio.request_id = index < 0 ? 0 : divoom_radio_station_at(index)->id;
        if (index >= 0)
        {
            g_divoom_radio.state.selected = index;
        }
        g_divoom_radio.state.play = index < 0 ? DIVOOM_RADIO_STOPPED :
            g_divoom_radio.player_task ? DIVOOM_RADIO_CONNECTING : DIVOOM_RADIO_PLAY_FAILED;
        g_divoom_radio.state.live_track[0] = '\0';
        g_divoom_radio.state.revision++;
    }
}

void divoom_radio_select_id(uint32 id)
{
    if (!divoom_radio_has_lock())
    {
        return;
    }
    divoom_system_get_lock(g_divoom_radio.lock);
    if (id && divoom_radio_find(id) >= 0)
    {
        divoom_radio_queue_capture(id);
    }
    g_divoom_radio.play_queue.steps = 0;
    divoom_radio_select_locked(id, 1);
    divoom_system_free_lock(g_divoom_radio.lock);
}

/* MODE/显式切源统一恢复最后成功电台；页面已排队的新选台优先，不被旧历史覆盖。 */
uint8 divoom_radio_request_current(int32 generation)
{
    uint8 current = 0;
    if (divoom_radio_has_lock())
    {
        divoom_system_get_lock(g_divoom_radio.lock);
        current = generation == divoom_radio_generation();
        divoom_system_free_lock(g_divoom_radio.lock);
    }
    return current;
}

int32 divoom_radio_get_request_generation(void)
{
    return divoom_radio_generation();
}

/* music锁→radio锁：条件校验、恢复目标和出声许可在一次短锁内提交，不再二次无条件起播。 */
uint8 divoom_radio_prepare_resume(uint8 allow_output, int32 generation)
{
    if (!divoom_radio_has_lock())
    {
        return 0;
    }
    divoom_system_get_lock(g_divoom_radio.lock);
    if (generation != divoom_radio_generation())
    {
        divoom_system_free_lock(g_divoom_radio.lock);
        return 0;
    }
    g_divoom_radio.resume_last = g_divoom_radio.request_id == 0;
    g_divoom_radio.resume_pending = allow_output && g_divoom_radio.resume_last;
    if (g_divoom_radio.resume_pending)
    {
        g_divoom_radio.startup_request++;
    }
    if (g_divoom_radio.resume_pending && g_divoom_radio.browse_active.kind == DIVOOM_RADIO_BROWSE_AUTO)
    {
        g_divoom_radio.browse_initial_checked = 0;
        g_divoom_radio.state.load = DIVOOM_RADIO_LOADING;
    }
    if (!allow_output || g_divoom_radio.resume_last)
    {
        divoom_radio_next_generation();
        g_divoom_radio.request_id = 0;
    }
    g_divoom_radio.source_request = 0;
    g_divoom_radio.state.play = allow_output ? DIVOOM_RADIO_CONNECTING : DIVOOM_RADIO_STOPPED;
    g_divoom_radio.state.live_track[0] = '\0';
    g_divoom_radio.state.revision++;
    divoom_system_free_lock(g_divoom_radio.lock);
    return 1;
}

void divoom_radio_resume(void)
{
    divoom_radio_prepare_resume(1, divoom_radio_generation());
}

/* 换台尚在取目录/尝试线路时也可暂停。调用方持radio锁。 */
static uint8 divoom_radio_is_playing_locked(void)
{
    const divoom_radio_play_queue *queue = &g_divoom_radio.play_queue;
    return g_divoom_radio.state.play == DIVOOM_RADIO_CONNECTING ||
        g_divoom_radio.state.play == DIVOOM_RADIO_PLAYING ||
        (queue->steps && queue->count && queue->step_generation == divoom_radio_generation());
}

/* 同一短锁内判断实际播放项，避免用UI旧快照重启当前台；切源仍由worker执行。 */
void divoom_radio_activate_id(uint32 id)
{
    if (!divoom_radio_has_lock() || !id)
    {
        return;
    }
    divoom_system_get_lock(g_divoom_radio.lock);
    int32 index = divoom_radio_find(id);
    if (index >= 0)
    {
        uint8 current = index == g_divoom_radio.state.selected;
        uint8 playing = current && divoom_radio_is_playing_locked();
        uint8 origin_changed = g_divoom_radio.play_queue.query.kind != g_divoom_radio.browse_active.kind ||
            g_divoom_radio.play_queue.query.classify_id != g_divoom_radio.browse_active.classify_id ||
            strcmp(g_divoom_radio.play_queue.query.genre, g_divoom_radio.browse_active.genre) != 0 ||
            (g_divoom_radio.browse_active.kind == DIVOOM_RADIO_BROWSE_RECENT &&
                g_divoom_radio.play_queue.browse_revision != g_divoom_radio.browse_applied);
        /* 同来源恢复保留快照；从另一可见列表重新起播，必须改用该列表的上下台队列。 */
        if (!current || !g_divoom_radio.play_queue.count ||
            (!playing && id != g_divoom_radio.browse_pinned && origin_changed))
        {
            divoom_radio_queue_capture(id);
        }
        /* 暂停保留selected及原始队列，恢复跟随直播；连接过程中也允许立即取消。 */
        g_divoom_radio.play_queue.steps = 0;
        divoom_radio_select_locked(playing ? 0 : id, 1);
    }
    divoom_system_free_lock(g_divoom_radio.lock);
}

/* 播放专键不等于重新选台：优先操作实际音源，未选台时才使用页面提供的有效焦点。 */
void divoom_radio_toggle_play(uint32 fallback_id)
{
    if (!divoom_radio_has_lock())
    {
        return;
    }
    divoom_system_get_lock(g_divoom_radio.lock);
    if (divoom_radio_is_playing_locked())
    {
        /* 格式探测可能已经移除当前项，播放暂停键仍先取消待续试。 */
        g_divoom_radio.play_queue.steps = 0;
        divoom_radio_select_locked(0, 1);
        divoom_system_free_lock(g_divoom_radio.lock);
        return;
    }
    int32 index = g_divoom_radio.state.selected;
    if (index < 0 || index >= g_divoom_radio.state.count)
    {
        index = divoom_radio_find(fallback_id);
    }
    if (index >= 0)
    {
        uint32 id = divoom_radio_station_at(index)->id;
        if (!g_divoom_radio.play_queue.count || index != g_divoom_radio.state.selected)
        {
            divoom_radio_queue_capture(id);
        }
        g_divoom_radio.play_queue.steps = 0;
        divoom_radio_select_locked(id, 1);
    }
    divoom_system_free_lock(g_divoom_radio.lock);
}

/* 调度器控制直播暂停/恢复：暂停关闭连接，保留selected，恢复跟随直播当前时刻。 */
static void divoom_radio_set_output(uint8 playing, uint8 keep_resume)
{
    int32 index;
    if (!divoom_radio_has_lock())
    {
        return;
    }
    divoom_system_get_lock(g_divoom_radio.lock);
    if (!playing && !keep_resume)
    {
        g_divoom_radio.resume_last = 0;
    }
    index = playing && g_divoom_radio.resume_last ? -1 : g_divoom_radio.state.selected;
    if (index < 0 && !g_divoom_radio.resume_last && g_divoom_radio.state.count > 0)
    {
        index = 0;
    }
    /* 音源控制器提交同一选台时沿用等待代号，不延长这次等待。 */
    if (playing && g_divoom_radio.state.play != DIVOOM_RADIO_CONNECTING)
    {
        g_divoom_radio.startup_request++;
    }
    divoom_radio_next_generation();
    g_divoom_radio.source_request = 0;
    /* 配置尚未读完/目录为空时保留播放意图；停止必须撤销，防止稍后worker跨音源起播。 */
    g_divoom_radio.resume_pending = playing && index < 0;
    if (g_divoom_radio.resume_pending && g_divoom_radio.browse_active.kind == DIVOOM_RADIO_BROWSE_AUTO)
    {
        g_divoom_radio.browse_initial_checked = 0;
        g_divoom_radio.state.load = DIVOOM_RADIO_LOADING;
    }
    g_divoom_radio.request_id = playing && index >= 0 ? divoom_radio_station_at(index)->id : 0;
    if (playing && index >= 0)
    {
        g_divoom_radio.state.selected = index;
    }
    g_divoom_radio.state.play = g_divoom_radio.request_id || g_divoom_radio.resume_pending ?
        DIVOOM_RADIO_CONNECTING : DIVOOM_RADIO_STOPPED;
    g_divoom_radio.state.live_track[0] = '\0';
    g_divoom_radio.state.revision++;
    divoom_system_free_lock(g_divoom_radio.lock);
}

void divoom_radio_set_play(uint8 playing)
{
    divoom_radio_set_output(playing, 0);
}

/* 语音轨暂停仅撤销出声，不丢失这次切源的恢复目标；显式停止使用set_play清除全部意图。 */
void divoom_radio_set_voice_output(uint8 playing)
{
    divoom_radio_set_output(playing, 1);
}

divoom_radio_play divoom_radio_get_play_state(void)
{
    divoom_radio_play state = DIVOOM_RADIO_STOPPED;
    if (divoom_radio_has_lock())
    {
        /* 与播放器同一把短锁；不分配快照、不访问 JSON、不回调音乐控制器。 */
        divoom_system_get_lock(g_divoom_radio.lock);
        state = g_divoom_radio.state.play;
        divoom_system_free_lock(g_divoom_radio.lock);
    }
    return state;
}

uint8 divoom_radio_is_starting(uint32 *request)
{
    uint8 starting = 0;
    if (divoom_radio_has_lock())
    {
        divoom_system_get_lock(g_divoom_radio.lock);
        int32 current = divoom_radio_generation();
        const divoom_radio_play_queue *queue = &g_divoom_radio.play_queue;
        starting = (g_divoom_radio.state.play == DIVOOM_RADIO_CONNECTING &&
            g_divoom_radio.started_generation != current) ||
            (queue->steps && queue->count && queue->step_generation == current);
        if (starting && request)
        {
            *request = g_divoom_radio.startup_request;
        }
        divoom_system_free_lock(g_divoom_radio.lock);
    }
    return starting;
}

uint8 divoom_radio_is_playing(void)
{
    uint8 playing = 0;
    if (divoom_radio_has_lock())
    {
        divoom_system_get_lock(g_divoom_radio.lock);
        playing = divoom_radio_is_playing_locked();
        divoom_system_free_lock(g_divoom_radio.lock);
    }
    return playing;
}

/* 上下台使用稳定id；明确不支持项不能成为目标，临时失败仍允许用户重试。 */
void divoom_radio_step(int32 direction)
{
    if (!divoom_radio_has_lock() || !direction)
    {
        return;
    }
    divoom_system_get_lock(g_divoom_radio.lock);
    divoom_radio_play_queue *queue = &g_divoom_radio.play_queue;
    if (!queue->count && g_divoom_radio.state.selected >= 0)
    {
        divoom_radio_queue_capture(divoom_radio_station_at(g_divoom_radio.state.selected)->id);
    }
    if (queue->step_generation != divoom_radio_generation() || queue->retry_queued)
    {
        queue->steps = 0;
    }
    if (!queue->steps && queue->count)
    {
        queue->retry_direction = 0;
        queue->retry_queued = 0;
        queue->wait_for_commit = g_divoom_radio.state.play == DIVOOM_RADIO_CONNECTING &&
            g_divoom_radio.started_generation != divoom_radio_generation();
        if (!queue->wait_for_commit)
        {
            g_divoom_radio.startup_request++;
        }
    }
    queue->steps += direction > 0 ? 1 : -1;
    if (queue->steps > DIVOOM_RADIO_MAX)
    {
        queue->steps = DIVOOM_RADIO_MAX;
    }
    if (queue->steps < -DIVOOM_RADIO_MAX)
    {
        queue->steps = -DIVOOM_RADIO_MAX;
    }
    queue->step_generation = divoom_radio_generation();
    queue->step_serial++;
    divoom_system_free_lock(g_divoom_radio.lock);
}

/* 显式URL复用同一独立音源和控制链；仅一个手动槽，不覆盖任何目录电台或写死默认台。 */
int32 divoom_radio_play_url(const char *url, const char *name)
{
    if (!url || (strncmp(url, "http://", 7) && strncmp(url, "https://", 8)) ||
        strlen(url) >= sizeof(g_divoom_radio.first_station.url) || strpbrk(url, "\r\n\t ") ||
        !divoom_radio_has_lock())
    {
        return 0;
    }
    uint32 id = 2166136261u;
    for (const uint8 *p = (const uint8 *)url; *p; p++)
    {
        id = (id ^ *p) * 16777619u;
    }
    id |= 0x80000000u;
    divoom_system_get_lock(g_divoom_radio.lock);
    int32 collision = divoom_radio_find(id);
    if (collision >= 0 && id != g_divoom_radio.manual_id)
    {
        divoom_system_free_lock(g_divoom_radio.lock);
        return 0;
    }
    int32 index = divoom_radio_find(g_divoom_radio.manual_id);
    if (index < 0)
    {
        if (g_divoom_radio.state.count >= DIVOOM_RADIO_MAX)
        {
            divoom_system_free_lock(g_divoom_radio.lock);
            return 0;
        }
        index = divoom_radio_directory_append();
        if (index < 0)
        {
            divoom_system_free_lock(g_divoom_radio.lock);
            return 0;
        }
    }
    divoom_radio_station *station = divoom_radio_station_at(index);
    uint8 favorite = station->id == id ? station->favorite : 0;
    uint32 recent = station->id == id ? station->recent : 0;
    memset(station, 0, sizeof(*station));
    station->id = id;
    station->favorite = favorite;
    station->recent = recent;
    /* 显式URL的稳定ID也读取已保存标记，重进或重启后重新添加时恢复收藏/最近。 */
    for (int32 i = 0; i < DIVOOM_RADIO_MAX; i++)
    {
        station->favorite |= g_divoom_radio.saved.ids[i] == id;
        if (!station->recent && g_divoom_radio.saved.recent[i] == id)
        {
            station->recent = DIVOOM_RADIO_MAX - i;
        }
    }
    snprintf(station->url, sizeof(station->url), "%s", url);
    snprintf(station->name, sizeof(station->name), "%s", name && name[0] ? name : url);
    g_divoom_radio.manual_id = id;
    if (g_divoom_radio.state.total < g_divoom_radio.state.count)
    {
        g_divoom_radio.state.total = g_divoom_radio.state.count;
    }
    g_divoom_radio.state.revision++;
    divoom_system_free_lock(g_divoom_radio.lock);
    divoom_radio_select_id(id);
    return 1;
}

void divoom_radio_select(int32 index)
{
    uint32 id = 0;
    if (!divoom_radio_has_lock())
    {
        return;
    }
    divoom_system_get_lock(g_divoom_radio.lock);
    if (index >= 0 && index < g_divoom_radio.state.count)
    {
        id = divoom_radio_station_at(index)->id;
    }
    divoom_system_free_lock(g_divoom_radio.lock);
    divoom_radio_select_id(id);
}

void divoom_radio_favorite_id(uint32 id)
{
    if (!divoom_radio_has_lock())
    {
        return;
    }
    divoom_system_get_lock(g_divoom_radio.lock);
    if (divoom_radio_find(id) >= 0 && !g_divoom_radio.favorite_id)
    {
        g_divoom_radio.favorite_id = id;
    }
    divoom_system_free_lock(g_divoom_radio.lock);
}

void divoom_radio_favorite(int32 index)
{
    if (!divoom_radio_has_lock())
    {
        return;
    }
    divoom_system_get_lock(g_divoom_radio.lock);
    if (index >= 0 && index < g_divoom_radio.state.count && !g_divoom_radio.favorite_id)
    {
        g_divoom_radio.favorite_id = divoom_radio_station_at(index)->id;
    }
    divoom_system_free_lock(g_divoom_radio.lock);
}

void divoom_radio_hint(int32 index)
{
    if (!divoom_radio_has_lock())
    {
        return;
    }
    divoom_system_get_lock(g_divoom_radio.lock);
    g_divoom_radio.hint_id = index >= 0 && index < g_divoom_radio.state.count ?
        divoom_radio_station_at(index)->id : 0;
    divoom_system_free_lock(g_divoom_radio.lock);
}

void divoom_radio_hint_id(uint32 id)
{
    if (!divoom_radio_has_lock())
    {
        return;
    }
    divoom_system_get_lock(g_divoom_radio.lock);
    g_divoom_radio.hint_id = id;
    divoom_system_free_lock(g_divoom_radio.lock);
}

void divoom_radio_reload(void)
{
    if (!divoom_radio_has_lock())
    {
        return;
    }
    divoom_system_get_lock(g_divoom_radio.lock);
    if (g_divoom_radio.state.load != DIVOOM_RADIO_LOADING)
    {
        if (g_divoom_radio.browse_active.kind == DIVOOM_RADIO_BROWSE_AUTO)
        {
            /* 首次分类超时允许用户重试，不能把瞬时网络失败固化为默认分类。 */
            g_divoom_radio.browse_initial_checked = 0;
            g_divoom_radio.browse_class_request = 1;
            g_divoom_radio.browse_class_load = DIVOOM_RADIO_LOADING;
        }
        g_divoom_radio.reload = 1;
        g_divoom_radio.state.load = DIVOOM_RADIO_LOADING;
        g_divoom_radio.state.revision++;
    }
    divoom_system_free_lock(g_divoom_radio.lock);
}

static void divoom_radio_more_request(uint8 background)
{
    if (!divoom_radio_has_lock())
    {
        return;
    }
    divoom_system_get_lock(g_divoom_radio.lock);
    if (background && !g_divoom_radio.menu_active &&
        (g_divoom_radio.more || g_divoom_radio.state.loading_more))
    {
        /* 已排队或正在HTTP中的同一段都可升级；当前前台事务退出后沿用pending，不重复取段。 */
        g_divoom_radio.more = 1;
        g_divoom_radio.background_more = 1;
    }
    else if (g_divoom_radio.state.load == DIVOOM_RADIO_READY && !g_divoom_radio.state.loading_more &&
        g_divoom_radio.state.count < g_divoom_radio.state.total)
    {
        g_divoom_radio.target_count = g_divoom_radio.state.count + DIVOOM_RADIO_PAGE_SIZE;
        g_divoom_radio.more = 1;
        g_divoom_radio.background_more = background && !g_divoom_radio.menu_active;
        g_divoom_radio.state.loading_more = 1;
        g_divoom_radio.state.more_error = 0;
        g_divoom_radio.state.revision++;
    }
    divoom_system_free_lock(g_divoom_radio.lock);
}

void divoom_radio_more(void)
{
    divoom_radio_more_request(0);
}

#if !defined(_WIN32) && defined(CONFIG_DIVOOM_MPP_NET_AUDIO_CLI)
#include "divoom_main.h"
#include "divoom_astrootoo_source.h"
#include "divoom_menu_control.h"
#include "divoom_audio_buf_pool.h"
#include <FreeRTOS.h>
#include <portable.h>
#include "lwip/tcpip.h"
#include "lwip/priv/tcp_priv.h"
#include "lwip/stats.h"
/* 只在lwIP线程读取PCB链表，避免UART并发遍历释放中的连接。
 * 仅按需输出电台端口，观察接收窗口与乱序队列；不改变协议栈状态或缓存。 */
static void divoom_radio_tcp_diagnostic(void *user)
{
    struct tcp_pcb *pcb;
    uint32 count = 0;
    (void)user;
    for (pcb = tcp_active_pcbs; pcb; pcb = pcb->next)
    {
        if (pcb->remote_port == 8000)
        {
            uint32 queued = 0, bytes = 0, sack = 0;
#if LWIP_TCP_SACK_OUT
            sack = (pcb->flags & TF_SACK) != 0;
#endif
#if TCP_QUEUE_OOSEQ
            struct tcp_seg *segment;
            for (segment = pcb->ooseq; segment; segment = segment->next)
            {
                queued++;
                bytes += segment->len;
            }
#endif
            printf("[radio_tcp] tick=%u local=%u state=%u next=%u window=%u announced=%u ooseq=%u ooseq_bytes=%u refused=%u sack=%u\n",
                divoom_system_get_tickms(), (uint32)pcb->local_port, (uint32)pcb->state,
                (uint32)pcb->rcv_nxt, (uint32)pcb->rcv_wnd, (uint32)pcb->rcv_ann_wnd,
                queued, bytes, pcb->refused_data ? (uint32)pcb->refused_data->tot_len : 0u, sack);
            count++;
        }
    }
    if (!count)
    {
        printf("[radio_tcp] none\n");
    }
#if LWIP_STATS && TCP_STATS
    printf("[radio_lwip] recv=%u drop=%u memerr=%u chkerr=%u proterr=%u\n",
        (uint32)lwip_stats.tcp.recv, (uint32)lwip_stats.tcp.drop,
        (uint32)lwip_stats.tcp.memerr, (uint32)lwip_stats.tcp.chkerr, (uint32)lwip_stats.tcp.proterr);
#endif
#if LWIP_STATS && MEM_STATS
    printf("[radio_heap] used=%u max=%u avail=%u err=%u\n",
        (uint32)lwip_stats.mem.used, (uint32)lwip_stats.mem.max,
        (uint32)lwip_stats.mem.avail, (uint32)lwip_stats.mem.err);
#endif
#if LWIP_STATS && MEMP_STATS
    {
        static const uint32 pools[] = { MEMP_PBUF_POOL, MEMP_TCP_SEG, MEMP_NETBUF };
        static const char *names[] = { "pbuf", "tcpseg", "netbuf" };
        for (uint32 i = 0; i < 3u; i++)
        {
            const struct stats_mem *pool = lwip_stats.memp[pools[i]];
            if (pool)
            {
                printf("[radio_pool] kind=%s used=%u max=%u avail=%u err=%u\n", names[i],
                    (uint32)pool->used, (uint32)pool->max, (uint32)pool->avail, (uint32)pool->err);
            }
        }
    }
#endif
}

/* 仅提供只读诊断，禁止经串口模拟按键、强制切源或改写系统音量。 */
static int32 divoom_radio_command(int32 argc, char **argv)
{
    if (argc == 2 && !strcmp(argv[1], "mem"))
    {
        unsigned int pages = 0, pending = 0;
        HeapStats_t heap = {0};
        if (divoom_radio_has_lock())
        {
            divoom_system_get_lock(g_divoom_radio.lock);
            for (int32 i = 0; i < DIVOOM_RADIO_DIRECTORY_PAGES; i++)
            {
                pages += g_divoom_radio.directory[i] != NULL;
            }
            pending = g_divoom_radio.pending != NULL;
            divoom_system_free_lock(g_divoom_radio.lock);
        }
        vPortGetHeapStats(&heap);
        printf("[RADIO_MEM] pages=%u pending=%u page_bytes=%u sleep_ring=%u radio_ring=%u free=%lu largest=%lu\n",
            pages, pending, (unsigned)(DIVOOM_RADIO_PAGE_SIZE * sizeof(divoom_radio_station)),
            divoom_audio_buf_pool_is_leased(DIVOOM_AUDIO_BUS_SLEEP) ? DIVOOM_AUDIO_BUF_POOL_RING_BYTES : 0u,
            divoom_audio_buf_pool_is_leased(DIVOOM_AUDIO_BUS_NETWORK) ? DIVOOM_AUDIO_BUF_POOL_NETWORK_RING_BYTES : 0u,
            (unsigned long)heap.xAvailableHeapSpaceInBytes, (unsigned long)heap.xSizeOfLargestFreeBlockInBytes);
        return 0;
    }
    if (argc == 2 && !strcmp(argv[1], "genres"))
    {
        divoom_radio_browse_genre genre;
        divoom_radio_browse_state state;
        divoom_radio_browse_poll(&state);
        int32 count = divoom_radio_browse_genre_count(0);
        printf("[RADIO_GENRES] count=%d load=%d kind=%d class=%d\n", count, state.genre_load, state.active_query.kind, state.active_query.classify_id);
        for (int32 i = 0; i < count; i++)
        {
            if (divoom_radio_browse_genre_copy(0, i, &genre))
            {
                printf("[RADIO_GENRE] id=%d parent=%d name=%s\n", genre.id, genre.parent, genre.name);
            }
        }
        return 0;
    }
    if (argc == 2 && !strcmp(argv[1], "browse"))
    {
        long parent = -1, visible = -1;
        divoom_get_lvgl_lock();
        divoom_astrootoo_source_whitebox_radio_probe("parent", &parent);
        divoom_astrootoo_source_whitebox_radio_probe("count", &visible);
        divoom_free_lvgl_lock();
        if (!divoom_radio_has_lock())
        {
            return -1;
        }
        /* 仅按需复制标量诊断，不加载JSON、不模拟按键、不改变系统音量。 */
        divoom_system_get_lock(g_divoom_radio.lock);
        divoom_radio_play_queue *queue = &g_divoom_radio.play_queue;
        printf("[RADIO_BROWSE] kind=%d class=%d requested=%u applied=%u parent=%ld visible=%ld classes=%d pinned=%u queue_kind=%d queue_class=%d queue_count=%d queue_pos=%d queue_id=%u queue_steps=%d json=%u menu_active=%u\n",
            g_divoom_radio.browse_active.kind, g_divoom_radio.browse_active.classify_id,
            g_divoom_radio.browse_requested, g_divoom_radio.browse_applied, parent, visible,
            g_divoom_radio.browse_classes ? g_divoom_radio.browse_classes->count : 0,
            g_divoom_radio.browse_pinned, queue->query.kind, queue->query.classify_id,
            queue->count, queue->position, queue->position >= 0 && queue->position < queue->count ? queue->ids[queue->position] : 0,
            queue->steps, g_divoom_radio.cache != NULL, g_divoom_radio.menu_active);
        divoom_system_free_lock(g_divoom_radio.lock);
        return 0;
    }

    if (argc == 2 && !strcmp(argv[1], "metadata"))
    {
        divoom_print_err("[RADIO_META] name=%s\n", divoom_main_get_music_name());
        divoom_print_err("[RADIO_META] singer=%s\n", divoom_main_get_music_singer());
        divoom_print_err("[RADIO_META] lyric=%s\n", divoom_main_get_music_lyric());
        return 0;
    }
    if (argc == 2 && !strcmp(argv[1], "tcp"))
    {
        return tcpip_callback(divoom_radio_tcp_diagnostic, NULL);
    }
    if (argc == 2 && !strcmp(argv[1], "audio"))
    {
        uint32 id = 0;
        if (divoom_radio_has_lock())
        {
            divoom_system_get_lock(g_divoom_radio.lock);
            int32 index = g_divoom_radio.state.selected;
            if (index >= 0 && index < g_divoom_radio.state.count)
            {
                id = divoom_radio_station_at(index)->id;
            }
            divoom_system_free_lock(g_divoom_radio.lock);
        }
        printf("[RADIO_AUDIO] mode=%u playing=%u transport=%u volume=%u selected_id=%u menu=%u\n",
            divoom_music_ctrl_get_mode(), divoom_music_ctrl_is_playing(),
            divoom_music_ctrl_get_transport_state(), divoom_music_ctrl_get_volume(), id,
            divoom_menu_control_get_active_menu_id());
        return 0;
    }
    if (argc == 2 && !strcmp(argv[1], "list"))
    {
        divoom_radio_station *station = divoom_system_malloc(sizeof(*station));
        int32 count;
        if (!station || !divoom_radio_has_lock())
        {
            divoom_system_free(station);
            return -1;
        }
        divoom_system_get_lock(g_divoom_radio.lock);
        count = g_divoom_radio.state.count;
        divoom_system_free_lock(g_divoom_radio.lock);
        for (int32 i = 0; i < count; i++)
        {
            divoom_system_get_lock(g_divoom_radio.lock);
            *station = *divoom_radio_station_at(i);
            divoom_system_free_lock(g_divoom_radio.lock);
            printf("[RADIO_ITEM] index=%d id=%u mt=%s status=%u favorite=%u recent=%u\n",
                i, station->id, station->mt, station->compatibility, station->favorite, station->recent);
        }
        divoom_system_free(station);
        return 0;
    }
    if (argc == 2 && !strcmp(argv[1], "state"))
    {
        long focus = -1, tab = -1, layout = -1, column = -1, pane = -1, focus_favorite = -1, focus_visible = -1, focus_marked = -1;
        divoom_get_lvgl_lock();
        divoom_astrootoo_source_whitebox_radio_probe("focus", &focus);
        divoom_astrootoo_source_whitebox_radio_probe("tab", &tab);
        divoom_astrootoo_source_whitebox_radio_probe("layout", &layout);
        divoom_astrootoo_source_whitebox_radio_probe("column", &column);
        divoom_astrootoo_source_whitebox_radio_probe("pane", &pane);
        divoom_astrootoo_source_whitebox_radio_probe("focus_favorite", &focus_favorite);
        divoom_astrootoo_source_whitebox_radio_probe("focus_visible", &focus_visible);
        divoom_astrootoo_source_whitebox_radio_probe("focus_marked", &focus_marked);
        divoom_free_lvgl_lock();
        if (!divoom_radio_has_lock())
        {
            printf("[RADIO] not started menu=%d\n", divoom_menu_control_get_active_menu_id());
            return 0;
        }
        divoom_system_get_lock(g_divoom_radio.lock);
        printf("[RADIO] count=%d total=%d load=%d play=%d selected=%d pages=%u scanned=%u filtered=%u probes=%u hits=%u fav=%d recent=%d focus=%ld tab=%ld layout=%ld disk=%u engine=%d column=%ld pane=%ld focus_favorite=%ld focus_visible=%ld focus_marked=%ld\n",
            g_divoom_radio.state.count, g_divoom_radio.state.total, g_divoom_radio.state.load,
            g_divoom_radio.state.play, g_divoom_radio.state.selected, g_divoom_radio.state.page_requests,
            g_divoom_radio.state.scanned, g_divoom_radio.state.filtered, g_divoom_radio.state.probe_requests,
            g_divoom_radio.state.cache_hits, g_divoom_radio.state.favorite_total,
            g_divoom_radio.state.recent_total, focus, tab, layout, g_divoom_radio.cache_from_disk, divoom_net_audio_get_state(),
            column, pane, focus_favorite, focus_visible, focus_marked);
        printf("[RADIO_CACHE] menu=%u live=%u loads=%u frees=%u\n", g_divoom_radio.menu_active,
            g_divoom_radio.cache_live, g_divoom_radio.cache_loads, g_divoom_radio.cache_frees);
        divoom_system_free_lock(g_divoom_radio.lock);
        return 0;
    }
    printf("radio list | audio | metadata | tcp | state | mem\n");
    return -1;
}
FINSH_FUNCTION_EXPORT_CMD(divoom_radio_command, radio, paged radio diagnostics);
#endif


uint8 divoom_radio_browse_view_copy(divoom_radio_browse_view *out, uint32 revision, uint8 force)
{
    if (!divoom_radio_has_lock())
    {
        return 0;
    }
    divoom_system_get_lock(g_divoom_radio.lock);
    if (!force && revision == g_divoom_radio.state.revision)
    {
        divoom_system_free_lock(g_divoom_radio.lock);
        return 0;
    }
    const divoom_radio_state *state = &g_divoom_radio.state;
    memset(out, 0, sizeof(*out));
    out->count = state->count;
    out->total = state->total;
    out->selected = state->selected;
    out->revision = state->revision;
    out->page_requests = state->page_requests;
    out->favorite_error = state->favorite_error;
    out->loading_more = state->loading_more;
    out->more_error = state->more_error;
    out->load = state->load;
    out->play = state->play;
    if (state->selected >= 0 && state->selected < state->count)
    {
        snprintf(out->selected_name, sizeof(out->selected_name), "%s", divoom_radio_station_at(state->selected)->name);
    }
    snprintf(out->live_track, sizeof(out->live_track), "%s", state->live_track);
    for (int32 i = 0; i < state->count; i++)
    {
        out->items[i].id = divoom_radio_station_at(i)->id;
        out->items[i].favorite = divoom_radio_station_at(i)->favorite;
        out->items[i].recent = divoom_radio_station_at(i)->recent;
    }
    divoom_system_free_lock(g_divoom_radio.lock);
    return 1;
}

uint8 divoom_radio_browse_rows_copy(uint32 revision, const int32 *indices, int32 count, divoom_radio_station *out)
{
    if (!divoom_radio_has_lock() || count < 0 || count > 5)
    {
        return 0;
    }
    divoom_system_get_lock(g_divoom_radio.lock);
    if (revision != g_divoom_radio.state.revision)
    {
        divoom_system_free_lock(g_divoom_radio.lock);
        return 0;
    }
    for (int32 i = 0; i < count; i++)
    {
        if (indices[i] < 0 || indices[i] >= g_divoom_radio.state.count)
        {
            divoom_system_free_lock(g_divoom_radio.lock);
            return 0;
        }
        out[i] = *divoom_radio_station_at(indices[i]);
    }
    divoom_system_free_lock(g_divoom_radio.lock);
    return 1;
}

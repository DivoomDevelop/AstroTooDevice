#ifndef DIVOOM_RADIO_BROWSE_H
#define DIVOOM_RADIO_BROWSE_H
#include "divoom_radio.h"

/* 浏览请求与播放器generation独立：切类目/页签只能替换目录，不得中断音源。 */
typedef enum
{
    DIVOOM_RADIO_BROWSE_TOP,
    DIVOOM_RADIO_BROWSE_GENRE,
    DIVOOM_RADIO_BROWSE_DOMESTIC,
    DIVOOM_RADIO_BROWSE_FAVORITE,
    DIVOOM_RADIO_BROWSE_RECENT,
    DIVOOM_RADIO_BROWSE_AUTO /* 读取列表配置；首次国内央广、国外TOP100。 */
} divoom_radio_browse_kind;

typedef struct
{
    divoom_radio_browse_kind kind;
    int32 classify_id;
    char genre[96];
} divoom_radio_browse_query;

typedef struct
{
    uint32 requested;
    uint32 applied;
    uint32 pinned_id;
    int32 classify_count;
    divoom_radio_load classify_load;
    divoom_radio_load genre_load;
    divoom_radio_browse_query active_query;
} divoom_radio_browse_state;

typedef struct
{
    uint32 id;
    uint8 favorite;
    uint32 recent;
} divoom_radio_browse_index;

typedef struct
{
    divoom_radio_browse_index items[DIVOOM_RADIO_MAX + 1];
    int32 count;
    int32 total;
    int32 selected;
    uint32 revision;
    uint32 page_requests;
    uint32 favorite_error;
    uint8 loading_more;
    uint8 more_error;
    divoom_radio_load load;
    divoom_radio_play play;
    char selected_name[DIVOOM_RADIO_NAME_MAX];
    char live_track[192];
} divoom_radio_browse_view;

uint8 divoom_radio_browse_view_copy(divoom_radio_browse_view *out, uint32 revision, uint8 force);
uint8 divoom_radio_browse_rows_copy(uint32 revision, const int32 *indices, int32 count, divoom_radio_station *out);

/* 目录服务原始分类；parent=0为根，其余是服务返回的父分类ID。名称禁止走翻译表。 */
typedef struct
{
    int32 id;
    int32 parent;
    char name[96];
} divoom_radio_browse_genre;

void divoom_radio_browse_genre_request(void);
int32 divoom_radio_browse_genre_count(int32 parent);
uint8 divoom_radio_browse_genre_copy(int32 parent, int32 row, divoom_radio_browse_genre *out);

/* 以下接口只投递请求/短锁复制；HTTP、MQTT和文件操作均由目录worker执行。 */
void divoom_radio_browse_request(const divoom_radio_browse_query *query);
void divoom_radio_browse_poll(divoom_radio_browse_state *out);
void divoom_radio_browse_classify_request(void);
uint8 divoom_radio_browse_classify_copy(int32 index, int32 *id, char *name, uint32 capacity);
void divoom_radio_set_menu_active(uint8 active);

#endif

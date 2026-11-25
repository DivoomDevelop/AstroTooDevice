#ifndef divoom_sleep_aid_config_h
#define divoom_sleep_aid_config_h

#include "divoom_type.h"
#include "cJSON.h"
#include "divoom_sleep_aid_json.h"

enum{
    DIVOOM_SLEEP_AID_TYPE_NOISE,        //噪音
    DIVOOM_SLEEP_AID_TYPE_WHITE,        //自然环境音
    DIVOOM_SLEEP_AID_TYPE_MUSIC,        //乐曲
    DIVOOM_SLEEP_AID_TYPE_FAV,          //收藏
    DIVOOM_SLEEP_AID_TYPE_DIY,          //DIY混合
    DIVOOM_SLEEP_AID_TYPE_RECORD,       //录音
};


typedef struct _divoom_sleep_aid_info
{
    int before_time;    //睡前时间
    int before_vol;     //睡前音量 0~100 级
    int before_bright;     //睡前亮度 0~100 级
    int after_on_off;   //入睡后开关
    int after_vol;      //入睡后音量 0~100 级
    int after_music;    //入睡后音源ID
    int after_time;     //入睡后时间关机
    int after_bright;     //睡后亮度 0~100 级
    int volume[8];    //混合音量 0~100 级
    int last_voice_type; /* 最后一次正式播放的助眠音源分类 */
    int last_voice_id;   /* 最后一次正式播放的助眠音源 ID */
}divoom_sleep_aid_info, *Pdivoom_sleep_aid_info;


/** 收藏信息：助眠分类 audio_type + 音源 ID + 歌词动效 video_type（cfg 字段）。 */
typedef struct _divoom_sleep_aid_fav_item
{
    uint32 audio_id;
    int audio_type;
    int video_type;
} divoom_sleep_aid_fav_item, *Pdivoom_sleep_aid_fav_item;

#define DIVOOM_SLEEP_AID_VOICE_NAME_LEN 112

/** voice_get_list 返回项：audio_id + 助眠分类 + 歌词动效 video_type + 当前语言显示名。 */
typedef struct _divoom_sleep_aid_voice_list_item
{
    uint32 audio_id;
    int audio_type;
    int video_type;
    char name[DIVOOM_SLEEP_AID_VOICE_NAME_LEN];
} divoom_sleep_aid_voice_list_item, *Pdivoom_sleep_aid_voice_list_item;

/** Local persisted fields for one sleep-aid voice (maps API SleepList row). */
typedef struct _divoom_sleep_aid_config_item
{
    uint8 add_flag;
    uint32 voice_id;
    int video_type;
    int update_time;
    char file_id[56];
} divoom_sleep_aid_config_item, *Pdivoom_sleep_aid_config_item;


/** Merge to cfg / language files / image manager. Memory is freed by net unpack free_handle after callback. */
void divoom_sleep_aid_handle_get_all_list_resp(void *para);

/**
 * 依次发送 AidSleep/GetAllList（Type 0..DIVOOM_SLEEP_AID_STORED_TYPE_NUM-1）。
 * 各 Type 的 `UpdateTime` 来自 `DIVOOM_SLEEP_AID_UPDATE_TIME_PATH`：`SleepUpdateList` 中仅包含有本地缓存的
 * `AUDIO_TYPE` / `UPDATE_TIME` 行；未出现的 Type 按 0 发送。不加载完整 SleepList。
 */
void divoom_sleep_aid_send_get_all_list(void);

/**
 * 读取全部收藏（堆内存）。
 * @param items 输出个数；可为 NULL（则不写出个数）。
 * @return 动态分配的数组指针；无数据时返回 NULL 且 *items==0。用完请 divoom_system_free。
 */
divoom_sleep_aid_fav_item *divoom_sleep_aid_fav_get_list(int *items);

/** 添加收藏；已存在相同 audio_id + audio_type 时不重复添加（视为成功）。失败返回 -1。 */
int divoom_sleep_aid_fav_add(uint32 audio_id, int audio_type, int video_type);

/** 取消收藏（按 audio_id + audio_type 匹配一项）；不存在则视为成功。失败返回 -1。 */
int divoom_sleep_aid_fav_remove(uint32 audio_id, int audio_type);

/**
 * 按助眠分类返回音源列表（动态分配）。
 * @param aid_type `NOISE`/`WHITE`/`MUSIC`：读 cfg；`FAV`：读收藏；`RECORD`/`DIY` 等同原语义。
 * @param items 输出个数（可为 NULL）。
 * @return `divoom_sleep_aid_voice_list_item` 数组；无数据返回 NULL。用完 divoom_system_free。
 */
divoom_sleep_aid_voice_list_item *divoom_sleep_aid_voice_get_list(int aid_type, int *items, int add_flag);

/**
 * 分段读取分类音源列表（仅 NOISE/WHITE/MUSIC）：只解析并返回 [offset, offset+count) 区间，
 * 用于列表分批加载，避免一次构建过多行。
 * @param aid_type   NOISE/WHITE/MUSIC。
 * @param offset     起始下标（基于 add_flag 过滤后的序列；<0 视为 0）。
 * @param count      期望返回条数（<=0 返回 NULL）。
 * @param add_flag   add_flag 过滤：-1 全部、0 未加入、1 已加入。
 * @param out_n      实际返回条数（可为 NULL）。
 * @param total      过滤后总条数（可为 NULL），供 UI 判断是否还有下一批。
 * @return 动态数组（最多 count 条），无数据返回 NULL；用完 divoom_system_free。
 */
divoom_sleep_aid_voice_list_item *divoom_sleep_aid_voice_get_list_page(int aid_type, int offset, int count,
    int add_flag, int *out_n, int *total);

/**
 * 查询音源分类（cfg 中 `video_type` 字段实为 API 的 AudioType/分类；收藏为 `audio_type`）。
 * @param aid_type `NOISE`/`WHITE`/`MUSIC`：读对应 `sleep_aid_%02d.cfg`；`FAV`：读收藏项 `audio_type`。
 * @param audio_id 音源 ID（cfg 中为 voice_id）。
 * @return 分类枚举；未找到或参数无效返回 -1。
 */
int divoom_sleep_aid_voice_get_video_type(int aid_type, uint32 audio_id);

/** 修改本地睡眠音频 cfg 中指定条目的 AddFlag（按 voice_id + video_type 匹配首条）。 */
void divoom_sleep_aid_voice_change_add_flag(int video_type, uint32 voice_id, int add_flag);

/**
 * 从 cfg 中选取最多 @p max_pick 条 add_flag==0 且 file_id 非空的音源，将其 add_flag 置 1 并写回 cfg。
 * @param aid_type `DIVOOM_SLEEP_AID_TYPE_NOISE` / `WHITE` / `MUSIC`。
 * @param voice_ids 输出 voice_id，长度至少 max_pick。
 * @param file_ids 输出 file_id，每项 56 字节，长度至少 max_pick。
 * @return 实际选取条数（0..max_pick）。
 */
int divoom_sleep_aid_voice_pick_pending_add(int aid_type, uint32 *voice_ids, char file_ids[][56], int max_pick);

/**
 * 从 `DIVOOM_SLEEP_AID_LANGUAGE_PATH`（sleep_lang_<audio_type>_<locale>.cfg）的 `Names` 数组
 * 按 AUDIO_ID 查显示名；无命中时 `name_out` 为空串。列表 UI 应优先用 `voice_get_list` 返回的 name。
 */
void divoom_sleep_aid_send_get_voice_name(int voice_type, uint32 voice_id, char *name_out);

/**
 * 按 voice_id 在本地 sleep_aid 配置中查找下载地址（cfg 项 file_id）。
 * @param url_addr 输出缓冲区，长度建议 >= DIVOOM_MANAGE_FILE_ADDR_LEN。
 * @return 找到且 file_id 非空返回 1，否则返回 0。
 */
int divoom_sleep_aid_get_url_addr(uint32 voice_id, char *url_addr);

#endif

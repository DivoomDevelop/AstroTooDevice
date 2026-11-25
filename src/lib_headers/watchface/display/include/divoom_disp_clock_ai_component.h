#ifndef _divoom_disp_clock_ai_component_h_
#define _divoom_disp_clock_ai_component_h_

/*
 * AI 表盘动态内容窗口 — 组件选取与事件控制模块
 *
 * 职责：
 * 1. 从固件 pcomp_list（内容表盘）与运行时事件（电量/闹钟/音乐等）合并选取展示项
 * 2. 拉取式 poll：查询系统状态 + 快照边沿检测「第一次触发」；冷却/去重在模块内完成
 * 3. select：优先级 + 运行时长 + 切换时机（是否到期、是否被 P0/P1 抢占）
 *
 * 调用链：divoom_disp_clock_main_component_control(1s) → poll → select → update_component_disp
 * 设计依据：Buddy 狗狗 AI 表盘规则 — 触发条件与优先级、频率方案、防烦机制
 */

#include "divoom_type.h"

/* 事件/内容类型；component_type 与触发事件共用此枚举 */
enum
{
    DIVOOM_AI_CLOCK_EVENT_NONE = 0,
    DIVOOM_AI_CLOCK_EVENT_TEMP_HIGH,        /* 温度高 ≥32°C */
    DIVOOM_AI_CLOCK_EVENT_TEMP_LOW,         /* 温度低 ≤8°C */
    DIVOOM_AI_CLOCK_EVENT_WEATHER,          /* 天气变化 / 温差≥5°C */
    DIVOOM_AI_CLOCK_EVENT_DIAL_WEATHER,     /* 天气内容表盘 */
    DIVOOM_AI_CLOCK_EVENT_MUSIC,            /* 蓝牙音乐开始播放 */
    DIVOOM_AI_CLOCK_EVENT_DIAL_STOCK,       /* 股票内容表盘 */
    DIVOOM_AI_CLOCK_EVENT_DIAL_BTC,         /* 虚拟货币内容表盘 */
    DIVOOM_AI_CLOCK_EVENT_DIAL_SOCIAL,       /* 社交/通用内容表盘 */
    DIVOOM_AI_CLOCK_EVENT_DIAL_PHOTO,        /* 相册内容表盘 */
    DIVOOM_AI_CLOCK_EVENT_DIAL_PIXEL,        /* 像素社区内容表盘 */
    DIVOOM_AI_CLOCK_EVENT_BAT_LOW,           /* 电量 ≤15% */
    DIVOOM_AI_CLOCK_EVENT_ALARM,             /* 闹钟临近（默认提前 15 分钟） */
    DIVOOM_AI_CLOCK_EVENT_DIALY_30M,         /* 日历 T-30 分钟 */
    DIVOOM_AI_CLOCK_EVENT_DIALY_5M,          /* 日历 T-5 分钟 */
    DIVOOM_AI_CLOCK_EVENT_STOCK_BIG,         /* 股票大幅波动 ±5% */
    DIVOOM_AI_CLOCK_EVENT_CHARGING,          /* 插入充电 */
    DIVOOM_AI_CLOCK_EVENT_WIFI,              /* Wi-Fi 连接状态变化 */
    DIVOOM_AI_CLOCK_EVENT_SPORTS_LIVE,       /* 关注赛事进入实时赛况 */
    DIVOOM_AI_CLOCK_EVENT_BIRTHDAY,          /* 生日或纪念日 */
    DIVOOM_AI_CLOCK_EVENT_ENTRY_FEEDBACK,    /* 进入表盘后的轻量反馈 */
    DIVOOM_AI_CLOCK_EVENT_SURPRISE,          /* 长时间无事件时的惊喜 */
    DIVOOM_AI_CLOCK_EVENT_NUM
};

enum
{
    DIVOOM_AI_CLOCK_TEXT_STATE_NON_SENSE = 1,         /*胡话*/
    DIVOOM_AI_CLOCK_TEXT_STATE_DIALY_LIFE = 2,         /*旧协议兼容，运行时统一映射到类型1*/
    DIVOOM_AI_CLOCK_TEXT_STATE_ANIMAL_KNOW = 3,         /*动物知识*/
    DIVOOM_AI_CLOCK_TEXT_STATE_NATURE_KNOW = 4,         /*自然知识*/
    DIVOOM_AI_CLOCK_TEXT_STATE_WEATHER = 5,             /*天气*/
    DIVOOM_AI_CLOCK_TEXT_STATE_TIME = 6,         /*时间*/
    DIVOOM_AI_CLOCK_TEXT_STATE_MUSIC = 7,         /*音乐*/
    DIVOOM_AI_CLOCK_TEXT_STATE_PHOTO = 8,         /*相册*/
    DIVOOM_AI_CLOCK_TEXT_STATE_PIXEL_ART = 9,         /*像素*/
    DIVOOM_AI_CLOCK_TEXT_STATE_CALENDAR = 10,         /*日历*/
    DIVOOM_AI_CLOCK_TEXT_STATE_STOCK = 11,         /*股票*/
    DIVOOM_AI_CLOCK_TEXT_STATE_SYSTEM_STATUS = 12,         /*系统状态*/
    DIVOOM_AI_CLOCK_TEXT_STATE_SURPRISE = 13,         /*惊讶*/
    DIVOOM_AI_CLOCK_TEXT_STATE_SOCIAL = 32,         /*社交媒体*/
};

/*
 * AI emoji 表情模式 ID。业务层只传递整型 ID，资源层在最终显示前映射到 states/<mode>.json。
 */
typedef enum
{
    DIVOOM_AI_EMOJI_MODE_ID_IDLE_LIFE = 0,       /* 日常待机/生活状态，默认循环表情 */
    DIVOOM_AI_EMOJI_MODE_ID_HAPPY,               /* 开心/正向反馈状态 */
    DIVOOM_AI_EMOJI_MODE_ID_ALERT,               /* 提醒/警觉状态，用于闹钟、电量低等强提示 */
    DIVOOM_AI_EMOJI_MODE_ID_NIGHT,               /* 夜间/睡眠氛围状态 */
    DIVOOM_AI_EMOJI_MODE_ID_PRONE_IDLE,          /* 趴着待机状态 */
    DIVOOM_AI_EMOJI_MODE_ID_SIT_IDLE,            /* 坐着待机状态 */
    DIVOOM_AI_EMOJI_MODE_ID_SCENE_COLD,          /* 场景：寒冷/低温 */
    DIVOOM_AI_EMOJI_MODE_ID_SING,                /* 音乐播放专用唱歌动作，Music Mode 只允许此模式 */
    DIVOOM_AI_EMOJI_MODE_ID_SCENE_MUSIC = 7,      /* 旧协议名，仅作 SING=7 的同值兼容 */
    DIVOOM_AI_EMOJI_MODE_ID_SCENE_HOT,           /* 场景：炎热/高温 */
    DIVOOM_AI_EMOJI_MODE_ID_SCENE_LIFE,          /* 场景：日常生活内容 */
    DIVOOM_AI_EMOJI_MODE_ID_SCENE_NIGHT,         /* 场景：夜晚/睡前 */
    DIVOOM_AI_EMOJI_MODE_ID_SCENE_PHOTO,         /* 场景：相册/照片内容 */
    DIVOOM_AI_EMOJI_MODE_ID_SCENE_PIXEL,         /* 场景：像素社区/像素内容 */
    DIVOOM_AI_EMOJI_MODE_ID_SCENE_RAINY,         /* 场景：雨天/阴雨天气 */
    DIVOOM_AI_EMOJI_MODE_ID_SCENE_STOCK_DOWN,    /* 场景：股票/行情下跌 */
    DIVOOM_AI_EMOJI_MODE_ID_SCENE_STOCK_UP,      /* 场景：股票/行情上涨 */
    DIVOOM_AI_EMOJI_MODE_ID_SCENE_SUNNY,         /* 场景：晴天/好天气 */
    DIVOOM_AI_EMOJI_MODE_ID_SCENE_WORRIED,       /* 场景：担心/负向状态 */
    DIVOOM_AI_EMOJI_MODE_ID_ENTRY_STRETCH,       /* Buddy 首次进入或长时间离开后的伸懒腰反馈 */
    DIVOOM_AI_EMOJI_MODE_ID_ENTRY_TURN_BACK,     /* Buddy 中短时间返回时的回头反馈 */
    DIVOOM_AI_EMOJI_MODE_ID_MAX                  /* 枚举边界，不能作为有效模式下发 */
} divoom_ai_emoji_mode_id_t;

/*
 * 独立动作意图与表情模式分离：表情可持续循环，动作只在场景边沿触发一次。
 * 调度层只给出业务意图，具体角色资源名由显示层按表盘 ID 映射。
 */
typedef enum
{
    DIVOOM_AI_ACTION_INTENT_NONE = 0,
    DIVOOM_AI_ACTION_INTENT_ENTRY_FULL,       /* 当天首次或离开超过 30 分钟后的完整回场 */
    DIVOOM_AI_ACTION_INTENT_ENTRY_RETURN,     /* 短暂离开或 2~30 分钟内返回 */
    DIVOOM_AI_ACTION_INTENT_KNOWLEDGE,        /* 角色核心知识、讲解或日常观察 */
    DIVOOM_AI_ACTION_INTENT_HEALTH,           /* 健康生活内容 */
    DIVOOM_AI_ACTION_INTENT_CARE,             /* 家庭生活、关怀或鼓励内容 */
    DIVOOM_AI_ACTION_INTENT_PHOTO,            /* 相册内容进入 */
    DIVOOM_AI_ACTION_INTENT_PIXEL,            /* 像素作品内容进入 */
    DIVOOM_AI_ACTION_INTENT_CALENDAR,         /* 日历临近提醒 */
    DIVOOM_AI_ACTION_INTENT_LOW_BATTERY,      /* 低电量提醒 */
    DIVOOM_AI_ACTION_INTENT_MARKET,           /* 无明确涨跌方向的中性市场观察 */
    DIVOOM_AI_ACTION_INTENT_MARKET_UP,        /* 仅用于带明确上涨方向的数据 */
    DIVOOM_AI_ACTION_INTENT_MARKET_DOWN,      /* 仅用于带明确下跌方向的数据 */
    DIVOOM_AI_ACTION_INTENT_SLEEP,            /* 夜间或睡眠提醒 */
    DIVOOM_AI_ACTION_INTENT_NUM
} divoom_ai_action_intent_t;

/*
 * 表盘调度大类。普通轮播先按大类抽取，再在大类内按子类和具体表盘抽取，
 * 候选表盘数量不会直接放大所在大类的命中概率。
 */
typedef enum
{
    DIVOOM_AI_SCHEDULE_CLASS_GENERAL = 0,     /* 旧配置兼容或角色核心内容 */
    DIVOOM_AI_SCHEDULE_CLASS_FINANCE,         /* 金融数据 */
    DIVOOM_AI_SCHEDULE_CLASS_SOCIAL,          /* 社交数据 */
    DIVOOM_AI_SCHEDULE_CLASS_GAME,            /* 游戏数据 */
    DIVOOM_AI_SCHEDULE_CLASS_SMART_HARDWARE,  /* 智能硬件数据 */
    DIVOOM_AI_SCHEDULE_CLASS_ALBUM,           /* 相册 */
    DIVOOM_AI_SCHEDULE_CLASS_COUNTDOWN,       /* 倒计时 */
    DIVOOM_AI_SCHEDULE_CLASS_PIXEL,           /* 像素图/像素社区 */
    DIVOOM_AI_SCHEDULE_CLASS_SPORTS,          /* 体育数据 */
    DIVOOM_AI_SCHEDULE_CLASS_AI_CONTENT,      /* AI 文字与知识内容 */
    DIVOOM_AI_SCHEDULE_CLASS_POETRY,          /* 诗词内容，目前仅包含 2303 */
    DIVOOM_AI_SCHEDULE_CLASS_NUM
} divoom_ai_schedule_class_t;

/*
 * AiScheduleType 保存到表盘配置。AUTO 表示旧表盘按 DeviceType/PixelDialType
 * 兼容推导；已发布数值保持稳定，旧的细分类型在入口处归一化为合并后的业务类型。
 */
#define DIVOOM_AI_SCHEDULE_TYPE_LEGACY_FINANCE_OTHER_VALUE 4
#define DIVOOM_AI_SCHEDULE_TYPE_LEGACY_SOCIAL_ACCOUNT_VALUE 6
#define DIVOOM_AI_SCHEDULE_TYPE_LEGACY_GAME_RESULT_VALUE 8
#define DIVOOM_AI_SCHEDULE_TYPE_LEGACY_SMART_HARDWARE_SENSOR_VALUE 10
#define DIVOOM_AI_SCHEDULE_TYPE_LEGACY_SPORTS_SCHEDULE_VALUE 15
#define DIVOOM_AI_SCHEDULE_TYPE_LEGACY_SPORTS_NEWS_VALUE 16

typedef enum
{
    DIVOOM_AI_SCHEDULE_TYPE_AUTO = 0,                  /* 旧配置自动识别 */
    DIVOOM_AI_SCHEDULE_TYPE_GENERAL = 1,               /* 角色核心/通用内容 */
    DIVOOM_AI_SCHEDULE_TYPE_FINANCE = 2,               /* 股票、基金、期货、贵金属与汇率 */
    DIVOOM_AI_SCHEDULE_TYPE_FINANCE_CRYPTO = 3,        /* 数字货币 */
    DIVOOM_AI_SCHEDULE_TYPE_SOCIAL = 5,                /* 社交账号、社区动态与作品 */
    DIVOOM_AI_SCHEDULE_TYPE_GAME = 7,                  /* 游戏状态、进度、成绩与排行 */
    DIVOOM_AI_SCHEDULE_TYPE_SMART_HARDWARE = 9,        /* 设备状态与传感器数据 */
    DIVOOM_AI_SCHEDULE_TYPE_ALBUM = 11,                /* 相册内容 */
    DIVOOM_AI_SCHEDULE_TYPE_COUNTDOWN = 12,            /* 倒计时，默认由条件触发 */
    DIVOOM_AI_SCHEDULE_TYPE_PIXEL_ART = 13,            /* 像素图/像素社区 */
    DIVOOM_AI_SCHEDULE_TYPE_SPORTS = 14,               /* 实况、赛程、排名与赛事资讯 */
    DIVOOM_AI_SCHEDULE_TYPE_MUSIC = 17,                /* 歌词/EQ，仅音乐事件使用 */
    DIVOOM_AI_SCHEDULE_TYPE_CALENDAR = 18,             /* Google/Microsoft 日历事件 */
    DIVOOM_AI_SCHEDULE_TYPE_AI_CONTENT = 19,           /* AI 生成文字与知识库 */
    DIVOOM_AI_SCHEDULE_TYPE_POETRY = 20,               /* 诗词表盘，目前仅用于 2303 */
    DIVOOM_AI_SCHEDULE_TYPE_NUM = 21
} divoom_ai_schedule_type_t;

/* 固件组件列表单项；由 divoom_disp_clock_main_load_all_component_list 填充 */
typedef struct _divoom_disp_clock_main_compent_info
{
    uint8 component_type;   /* DIVOOM_AI_CLOCK_EVENT_*，来自表盘 dial_type/clock_type */
    uint8 self_flag;        /* 1=替代表盘(compent_clock_id)，0=256 子表盘 */
    uint8 sub_clock_pos;    /* 子表盘在父表盘 disp_item 中的位置 */
    uint8 schedule_class;   /* DIVOOM_AI_SCHEDULE_CLASS_*，由 schedule_type 策略推导 */
    uint16 schedule_type;   /* DIVOOM_AI_SCHEDULE_TYPE_*，优先读取 AiScheduleType */
    uint16 run_time;        /* 本次展示时长(秒)，select 后写入 end_play_time */
    int parent_id;          /* 父表盘 clock_id */
    int sub_clock_id;       /* 实际播放的子表盘/组件 clock_id */
    int data_clock_id;      /* 仅指定数据源；底图和显示元素仍使用 sub_clock_id */
    uint16 slot_mask;       /* 固定 10 槽可用位；0 表示由 schedule_class 自动推导 */
    uint16 cooldown_sec;    /* 同类内部候选冷却，0 使用类型默认值 */
    uint8 weight;           /* 同类内部权重，0 按 100 处理 */
    uint8 priority;         /* 事件候选优先级，普通槽不使用 */
    uint8 trigger_id;       /* 持久化触发器编号，禁止保存函数指针 */
    uint8 normal_enabled;   /* 1 可进入 NORMAL；0 只允许事件触发 */
    uint8 event_only;       /* 2212 等事件容器必须为 1 */
}divoom_disp_clock_main_compent_info, *Pdivoom_disp_clock_main_compent_info;

/*
  Function:         divoom_disp_clock_ai_component_init
  Description:      模块初始化；表盘 init 时调用一次
*/
void divoom_disp_clock_ai_component_init(void);

/*
  Function:         divoom_disp_clock_ai_component_reset
  Description:      清空事件表与冷却状态；切换表盘或销毁组件列表时调用
*/
void divoom_disp_clock_ai_component_reset(void);

/*
  Function:         divoom_disp_clock_ai_component_detect_type
  Description:      根据表盘自带 DeviceType/PixelDialType 映射 component_type
  Input:            dial_type — DeviceType；clock_type — PixelDialType
  Return:           DIVOOM_AI_CLOCK_EVENT_DIAL_*
*/
uint8 divoom_disp_clock_ai_component_detect_type(int dial_type, uint8 clock_type);

/* 旧表盘元数据转换为 AiScheduleType；显式配置非 AUTO 时不走此兼容入口。 */
uint16 divoom_disp_clock_ai_component_detect_schedule_type(int dial_type,
    uint8 clock_type, uint8 component_type);

/* 将外部配置转换为当前枚举；旧 FINANCE_OTHER 数值统一折叠到 FINANCE。 */
uint16 divoom_disp_clock_ai_component_normalize_schedule_type(uint16 schedule_type);

/* 查询固化的表盘 ID 类型；未知 ID 返回 DIVOOM_AI_SCHEDULE_TYPE_AUTO。 */
uint16 divoom_disp_clock_ai_component_get_clock_schedule_type(int32 clock_id);

/* AiScheduleType 对应的大类，用于两级调度。 */
uint8 divoom_disp_clock_ai_component_schedule_class(uint16 schedule_type);

/* 调度前检查候选依赖的数据资源是否已经可显示。 */
uint8 divoom_disp_clock_ai_component_candidate_ready(
    Pdivoom_disp_clock_main_compent_info candidate);

/*
  Function:         divoom_disp_clock_ai_component_default_run_time
  Description:      按内容/事件类型返回默认展示时长(秒)；模拟器偏短便于验收
*/
uint16 divoom_disp_clock_ai_component_default_run_time(uint8 component_type);

/*
  Function:         divoom_disp_clock_ai_component_poll
  Description:      拉取系统状态并边沿检测；由 main 的 1s 定时器调用。
                    系统侧只返回当前值，首次触发与冷却均在本模块内处理。
*/
void divoom_disp_clock_ai_component_poll(void);

/* 事件抢占白名单：普通槽类型不受影响，仅 MUSIC、DIALY_30M、DIALY_5M 返回 1。 */
uint8 divoom_disp_clock_ai_component_event_trigger_enabled(uint8 event_type);

/*
  Function:         divoom_disp_clock_ai_component_set_trigger
  Description:      可选推送入口：无法轮询的事件源主动登记/清除
  Input:            event_type — DIVOOM_AI_CLOCK_EVENT_*；
                    sub_clock_id — 指定展示 dial，0 表示由 select 在 list 中匹配；
                    active — 1 激活，0 清除
*/
void divoom_disp_clock_ai_component_set_trigger(uint8 event_type, int sub_clock_id, uint8 active);

/*
  Function:         divoom_disp_clock_ai_component_select
  Description:      冷却 + 运行选择 + 切换时机。合并固件 list 与活跃事件，选出下标。
  Input:            list/num — pcomp_list；cur_index — 当前展示下标；
                    cur_end_tick — 当前项结束 tick（divoom_system_get_ticks 单位）
  Output:           out_switch — 1 表示应调用 update_component_disp 切换
  Return:           选中下标；未到切换时机且保持当前项时返回 cur_index；-1 表示无效
*/
int divoom_disp_clock_ai_component_select(Pdivoom_disp_clock_main_compent_info list, int num,
    int cur_index, uint32 cur_end_tick, int *out_switch);

/*
  Function:         divoom_disp_clock_ai_component_on_selected
  Description:      切换完成后更新冷却/连播计数；由 update_component_disp 调用
*/
void divoom_disp_clock_ai_component_on_selected(int index, uint8 component_type, uint16 run_time);

/*
  Function:         divoom_disp_clock_ai_component_mood_mode
  Description:      场景事件 → DIVOOM_AI_EMOJI_MODE_ID_*，不在业务层传递字符串
*/
divoom_ai_emoji_mode_id_t divoom_disp_clock_ai_component_mood_mode(uint8 event_type);

/*
  Function:         divoom_disp_clock_ai_component_text_type
  Description:      场景事件直接映射语料分类，不应用文本节奏与冷却。
  Return:           DIVOOM_AI_CLOCK_TEXT_STATE_*；无专用分类时返回日常生活。
*/
int divoom_disp_clock_ai_component_text_type(uint8 event_type);

/* 取出并清除本 tick 新触发的运行时事件；没有新事件时返回 NONE。 */
uint8 divoom_disp_clock_ai_component_take_trigger_event(void);

/*
  Function:         divoom_disp_clock_get_ai_text_type
  Description:      按当前场景、时间段和 Buddy 频率规则返回 AI 文案分类。
  Return:           DIVOOM_AI_CLOCK_TEXT_STATE_*；0 表示当前不需要更换文案
*/
int divoom_disp_clock_get_ai_text_type(void);

/*
  Function:         divoom_disp_clock_ai_component_on_ai_text_shown
  Description:      498 文案真实显示后调用，用于提交分类/触发源冷却。
*/
void divoom_disp_clock_ai_component_on_ai_text_shown(int text_type);

/*
  Function:         divoom_disp_clock_ai_component_get_scene_event
  Description:      取当前应驱动 497/498 的场景事件（活跃 P0~P3 或最近内容类型）
*/
uint8 divoom_disp_clock_ai_component_get_scene_event(void);


/*
  Function:         divoom_disp_clock_ai_component_music_dial
  Description:      是否在音乐界面
*/

uint8 divoom_disp_clock_ai_component_music_dial(void);

/* 播放状态不变但曲目已变化时刷新 Music Mode，不退出或重建歌词表盘。 */
void divoom_disp_clock_ai_component_notify_music_track_changed(void);

#if defined(DIVOOM_SIMULATOR)
/* 仿真探针：用于确认换歌只刷新 Music Mode，不触发退出/重进。 */
uint32 divoom_disp_clock_ai_component_whitebox_get_music_track_generation(void);
#endif

/*
  Function:         divoom_disp_clock_ai_component_debug_dump
  Description:      打印当前事件表与选取状态，便于模拟器调试
*/
void divoom_disp_clock_ai_component_debug_dump(void);

#endif

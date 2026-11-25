#ifndef _divoom_ai_talk_h_
#define _divoom_ai_talk_h_

struct _divoom_device_ai_classfy_list;
struct _divoom_device_ai_talk_list;

/* MsgType 与服务端及多语言角色语料分类一一对应，已有数值不得重排。 */
typedef enum _divoom_ai_talk_msg_type
{
	DIVOOM_AI_TALK_MSG_TYPE_INVALID = 0,
	DIVOOM_AI_TALK_MSG_TYPE_NONSENSE = 1,          /* 胡话/角色闲聊 */
	DIVOOM_AI_TALK_MSG_TYPE_DAILY_LIFE = 2,         /* 旧协议兼容，运行时统一映射到类型1 */
	DIVOOM_AI_TALK_MSG_TYPE_ANIMAL_KNOWLEDGE = 3,   /* 动物知识 */
	DIVOOM_AI_TALK_MSG_TYPE_NATURE_KNOWLEDGE = 4,   /* 植物与自然 */
	DIVOOM_AI_TALK_MSG_TYPE_WEATHER = 5,            /* 天气 */
	DIVOOM_AI_TALK_MSG_TYPE_TIME_RHYTHM = 6,        /* 时间节奏 */
	DIVOOM_AI_TALK_MSG_TYPE_MUSIC = 7,              /* 音乐 */
	DIVOOM_AI_TALK_MSG_TYPE_PHOTO = 8,              /* 照片 */
	DIVOOM_AI_TALK_MSG_TYPE_PIXEL_COMMUNITY = 9,    /* 像素社区 */
	DIVOOM_AI_TALK_MSG_TYPE_CALENDAR = 10,          /* 日历 */
	DIVOOM_AI_TALK_MSG_TYPE_STOCK = 11,             /* 中性股票信息 */
	DIVOOM_AI_TALK_MSG_TYPE_SYSTEM_STATUS = 12,     /* 系统状态 */
	DIVOOM_AI_TALK_MSG_TYPE_SURPRISE = 13,          /* 惊喜事件 */
	DIVOOM_AI_TALK_MSG_TYPE_PORTRY_SHORT = 14,      /* 诗词/名言正文与短评配对文本 */
	DIVOOM_AI_TALK_TODAY_WORD = 15,                 /* 兼容旧版今日一句 */
	DIVOOM_AI_TALK_MSG_TYPE_LITERATURE = 16,        /* 妈妈：文学与阅读 */
	DIVOOM_AI_TALK_MSG_TYPE_HEALTHY_LIVING = 17,    /* 妈妈：健康生活 */
	DIVOOM_AI_TALK_MSG_TYPE_KITCHEN = 18,           /* 妈妈：厨房知识 */
	DIVOOM_AI_TALK_MSG_TYPE_SEASONAL_CULTURE = 19,  /* 妈妈：节气与文化 */
	DIVOOM_AI_TALK_MSG_TYPE_HOME_LIVING = 20,       /* 妈妈：居家生活 */
	DIVOOM_AI_TALK_MSG_TYPE_SOLAR_SYSTEM = 21,      /* Divo：太阳系 */
	DIVOOM_AI_TALK_MSG_TYPE_UNIVERSE = 22,          /* Divo：宇宙 */
	DIVOOM_AI_TALK_MSG_TYPE_SPACEFLIGHT = 23,       /* Divo：航天 */
	DIVOOM_AI_TALK_MSG_TYPE_FINANCE_KNOWLEDGE = 24, /* 大富翁：中性金融知识 */
	DIVOOM_AI_TALK_MSG_TYPE_PHYSICS = 25,           /* 教授：物理 */
	DIVOOM_AI_TALK_MSG_TYPE_MATHEMATICS = 26,       /* 教授：数学 */
	DIVOOM_AI_TALK_MSG_TYPE_SCIENCE = 27,           /* 教授：综合科学 */
	DIVOOM_AI_TALK_MSG_TYPE_AI_BASICS = 28,         /* 教授：AI 基础 */
	/* 29-31 保留旧协议数值并映射到 NONSENSE；32 使用独立社交语料。 */
	DIVOOM_AI_TALK_MSG_TYPE_CHARACTER_NEWS = 29,
	DIVOOM_AI_TALK_MSG_TYPE_SPORTS = 30,
	DIVOOM_AI_TALK_MSG_TYPE_SMART_HARDWARE = 31,
	DIVOOM_AI_TALK_MSG_TYPE_SOCIAL = 32,             /* 社交媒体状态与动态感叹 */
	DIVOOM_AI_TALK_MSG_TYPE_GREETING = 33,          /* 每次进入 AI 表盘的正文与短评问候 */
	DIVOOM_AI_TALK_MSG_TYPE_NEWS = 34,              /* 新闻表盘的正文与短评配对文本 */
	DIVOOM_AI_TALK_MSG_TYPE_GUIDE_CARD = 35,        /* 图鉴卡：大框图片与小框说明 */
	DIVOOM_AI_TALK_MSG_TYPE_NUM = 36
} divoom_ai_talk_msg_type;

#define DIVOOM_AI_TALK_CARD_TEXT_MAX_LEN 512

typedef struct _divoom_ai_talk_acquire_req
{
	uint32 session_id;
	uint32 request_sequence;
	int clock_id;
	int msg_type;
	const char *language;
	const char *content_key;
	/* 非零只恢复缓存中指定 MsgId；找不到返回 0，不消费、不触发补充请求。 */
	int restore_msg_id;
	/* 2303 知识场景置 1；Buddy 3/4 只改变本次投影，磁盘仍保留 V2。 */
	uint8 require_big;
} divoom_ai_talk_acquire_req_t, *Pdivoom_ai_talk_acquire_req_t;

/* 指针由内容模块双槽缓冲持有，commit/cancel 前有效，调用方不得释放。 */
typedef struct _divoom_ai_talk_card_view
{
	uint32 generation;
	uint32 revision;
	int msg_id;
	int msg_type;
	uint32 guide_image_id;           /* 本地图片ID=MsgId*5+角色序号，避免五角色文件冲突。 */
	uint8 fallback_only;             /* 当前卡来自程序内置问候，不代表服务端批次已消费。 */
	const char *big_text;
	const char *small_text;           /* 小框前30秒内容，单段语料始终使用此项。 */
	const char *small_text_secondary; /* <SEP> 后的小框内容；没有第二段时为 NULL。 */
} divoom_ai_talk_card_view_t, *Pdivoom_ai_talk_card_view_t;

/* 类型 14 在一个协议字符串内保存正文和短评，服务端与设备必须使用同一分隔标记。 */
#define DIVOOM_AI_TALK_PORTRY_SHORT_SEPARATOR "<DIVOOM_AI_COMMENT>"
#define DIVOOM_AI_TALK_SMALL_TEXT_SEPARATOR "<SEP>"
#define DIVOOM_AI_TALK_PORTRY_SHORT_TEXT_MAX_LEN 512
/*
  Function:         divoom_ai_talk_init
  Description:      初始化 AI talk 内存缓存，并读取本地保存的分类和消息缓存。
*/
void divoom_ai_talk_init(void);

/*
  Function:         divoom_ai_talk_set_request_enabled
  Description:      设置 AI 语料网络请求总开关；仅当前表盘含 AI 元素时开启。
  Input:            enabled 1 开启，0 关闭。
*/
void divoom_ai_talk_set_request_enabled(uint8 enabled);

/* AI 角色或语言变化时切换 RAM 作用域，旧角色批次只保留在磁盘。 */
void divoom_ai_talk_set_active_scope(int clock_id);

/*
  Function:         divoom_ai_talk_get_pixel_community_author_name
  Description:      获取当前像素社区作品的作者名，用于替换语料中的 {author}。
                    作者数据源由上层后续实现。
  Return:           有效作者名，或者 NULL/空字符串。返回指针由上层持有。
*/
const char *divoom_ai_talk_get_pixel_community_author_name(void);

/* 显示前替换 {author}；缓存中的原始语料始终保留占位符。 */
char *divoom_ai_talk_prepare_pixel_community_msg(const char *msg);

/*
  Function:         divoom_ai_talk_get_random_msg
  Description:      按表盘、当前语言和 MsgType 获取一条 AI 文本；本地批次耗尽时异步刷新。
  Input:            clock_id 角色表盘 ID；category_id 对应语料 MsgType。
  Return:           成功返回模块内部缓存字符串；失败返回 NULL。
                    返回指针由模块持有，调用方不能修改或释放。
*/
char *divoom_ai_talk_get_random_msg(int clock_id,int category_id);

/* 返回当前语言的程序内置角色问候；仅作无缓存显示兜底，不改变 used_num。 */
const char *divoom_ai_talk_get_default_greeting(int clock_id);

/* 事务式取数：返回 1 成功、0 无数据、-1 参数/内容非法，失败时清空 card/token。
 * acquire 不消耗；显示成功 commit，失败、抢占或退出 cancel。
 * restore_msg_id 非零仅恢复仍在缓存的已消费卡；其 commit 不推进 used_num，
 * 上层也应跳过知识游标推进。旧批次被替换时返回 0，由上层正常取新卡。
 */
int32 divoom_ai_talk_acquire(const divoom_ai_talk_acquire_req_t *req,
	divoom_ai_talk_card_view_t *card, uint32 *token);
int32 divoom_ai_talk_commit(uint32 session_id, uint32 token);
void divoom_ai_talk_cancel(uint32 session_id, uint32 token);
void divoom_ai_talk_cancel_session(uint32 session_id);

/* 返回指定请求已经落入缓存且可读取的批次 generation；0 表示仍未就绪。
 * 该接口只读缓存，不消费内容，供 LVGL 控制循环及时唤醒异步显示重试。 */
uint32 divoom_ai_talk_get_ready_generation(int clock_id, int msg_type,
	uint32 request_sequence);

/*
  Function:         divoom_ai_talk_split_portry_short
  Description:      拆分类型 14 的配对文本；正文写入调用方缓冲区，短评指向原组合字符串。
  Input:            combined 组合文本；portry_buf 正文缓冲区；buf_len 缓冲区字节数；comment 短评输出。
  Return:           1 拆分成功，0 格式非法或缓冲区不足。
*/
uint8 divoom_ai_talk_split_portry_short(const char *combined, char *portry_buf,
	int32 buf_len, const char **comment);

/*
  Function:         divoom_ai_talk_refresh_classfy_list
  Description:      兼容旧调用；当前不再请求分类数量列表，只确保模块已初始化。
  Return:           0 成功，-1 初始化失败。
*/
int divoom_ai_talk_refresh_classfy_list(void);

/*
  Function:         divoom_ai_talk_first_update_classfy_list
  Description:      兼容旧页面初始化入口；当前只确保模块已初始化。
  Return:           0 成功，-1 初始化失败。
*/
int divoom_ai_talk_first_update_classfy_list(void);

/*
  Function:         divoom_ai_talk_save_info
  Description:      保存缓存格式迁移版本；不保存服务端分类数量。
  Return:           0 成功，-1 失败。
*/
int divoom_ai_talk_save_info(void);

void divoom_ai_talk_classfy_list_respone(struct _divoom_device_ai_classfy_list *pclassfy);
void divoom_ai_talk_msg_list_respone(struct _divoom_device_ai_talk_list *plist);

#if defined(DIVOOM_SIMULATOR)
typedef struct _divoom_ai_talk_probe
{
	uint16 pending_tokens;
	uint16 cache_used;
	uint8 cache_request_pending;
	uint32 stale_drop_count;
} divoom_ai_talk_probe_t;

/* 仿真只读探针及故障注入入口；生产取数流程不依赖这些接口。 */
int divoom_ai_talk_get_probe(divoom_ai_talk_probe_t *probe);
int divoom_ai_talk_get_category_probe(int clock_id, const char *language,
	int msg_type, divoom_ai_talk_probe_t *probe);
const char *divoom_ai_talk_whitebox_get_language(void);
int divoom_ai_talk_whitebox_expire_category(int clock_id, const char *language,
	int msg_type);
int divoom_ai_talk_whitebox_revoke_card(int clock_id, const char *language,
	int msg_type, int msg_id);
int divoom_ai_talk_whitebox_small_text_separator(void);
#endif

#endif

#include "divoom.h"
#include "divoom_menu_control.h"
#if defined(DIVOOM_SIMULATOR)
#include <stdlib.h>
#endif

#define DIVOOM_AI_TALK_SAVE_VERSION 5
#define DIVOOM_AI_TALK_PATH_MAX_LEN 192
#define DIVOOM_AI_TALK_LEGACY_CLOCK_ID 255
#define DIVOOM_AI_TALK_CLOCK_ID_MOM 1721
#define DIVOOM_AI_TALK_CLOCK_ID_DIVO 1817
#define DIVOOM_AI_TALK_CLOCK_ID_RICH_GUY 1896
#define DIVOOM_AI_TALK_CLOCK_ID_PROFESSOR 1991
#define DIVOOM_AI_TALK_PIXEL_MSG_MAX_LEN 768
#define DIVOOM_AI_TALK_PIXEL_AUTHOR_TOKEN "{author}"
/* divoom_system_get_ticks() 单位为秒；超时后允许同一分片重新请求。 */
#define DIVOOM_AI_TALK_REQUEST_TIMEOUT_SEC 15
#define DIVOOM_AI_TALK_SIM_PATH_MAX_LEN 512
#define DIVOOM_AI_TALK_TRANSACTION_SLOT_NUM 2
#define DIVOOM_AI_TALK_GUIDE_ROLE_NUM 5

typedef struct _divoom_ai_talk_clock_dir
{
	int clock_id;
	const char *dir_name;
} divoom_ai_talk_clock_dir;

typedef struct _divoom_ai_talk_scene_info
{
	int msg_type;
	const char *file_name;
} divoom_ai_talk_scene_info;

/* 目录名与 all_corpus 的五个角色目录保持一致。 */
static const divoom_ai_talk_clock_dir divoom_ai_talk_clock_dir_list[] =
{
	{255, "buddy"},
	{1721, "mom"},
	{1817, "divo"},
	{1896, "monopoly"},
	{1991, "professor"}
};

/* 仅维护仿真本地语料的文件映射，服务端批次数量由服务端自行决定。 */
static const divoom_ai_talk_scene_info divoom_ai_talk_scene_list[] =
{
	{1, "01_nonsense.json"},
	{3, "03_animal_knowledge.json"},
	{4, "04_nature_knowledge.json"},
	{5, "05_weather.json"},
	{6, "06_time_rhythm.json"},
	{7, "07_music.json"},
	{8, "08_photo.json"},
	{9, "09_pixel_community.json"},
	{10, "10_calendar.json"},
	{11, "11_stock.json"},
	{12, "12_system_status.json"},
	{13, "13_surprise.json"},
	{14, "14_poetry_short.json"},
	{16, "16_literature.json"},
	{17, "17_healthy_living.json"},
	{18, "18_kitchen.json"},
	{19, "19_seasonal_culture.json"},
	{20, "20_home_living.json"},
	{21, "21_solar_system.json"},
	{22, "22_universe.json"},
	{23, "23_spaceflight.json"},
	{24, "24_finance_knowledge.json"},
	{25, "25_physics.json"},
	{26, "26_mathematics.json"},
	{27, "27_science.json"},
	{28, "28_ai_basics.json"},
	{32, "32_social.json"},
	{33, "33_greeting.json"},
	{34, "34_news.json"}
};

/* 类型2及旧29-31统一落到角色闲聊；类型32社交语料保持独立。 */
static int divoom_ai_talk_normalize_category_id(int category_id)
{
	switch (category_id)
	{
	case DIVOOM_AI_TALK_MSG_TYPE_DAILY_LIFE:
	case DIVOOM_AI_TALK_MSG_TYPE_CHARACTER_NEWS:
	case DIVOOM_AI_TALK_MSG_TYPE_SPORTS:
	case DIVOOM_AI_TALK_MSG_TYPE_SMART_HARDWARE:
	{
		return DIVOOM_AI_TALK_MSG_TYPE_NONSENSE;
	}
	default:
	{
		return category_id;
	}
	}
}

typedef struct _divoom_ai_talk_category_cache
{
	int clock_id;
	int category_id;
	int msg_num;
	int used_num;
	int request_pending;
	uint8 persistence_pending; /* 同一分片写盘期间禁止获取或提交，避免MsgList与UsedNum互相覆盖。 */
	uint32 request_tick;
	uint32 request_sequence;
	uint32 revision;
	uint32 generation; /* RAM批次身份；锁外读文件回锁时用于阻断同游标ABA回灌。 */
	char language[DIVOOM_AI_TALK_LANGUAGE_MAX_LEN];
	int msg_id[DIVOOM_AI_TALK_MSG_BATCH_NUM];
	char file_id[DIVOOM_AI_TALK_MSG_BATCH_NUM][DIVOOM_AI_TALK_FILE_ID_MAX_LEN];
	char *msg[DIVOOM_AI_TALK_MSG_BATCH_NUM];
	struct _divoom_ai_talk_category_cache *next;
} divoom_ai_talk_category_cache, *Pdivoom_ai_talk_category_cache;

typedef struct _divoom_ai_talk_transaction_slot
{
	uint8 active;
	uint8 restore_only; /* 恢复显示只确认 token，不重复消费已显示卡片。 */
	uint8 fallback_only; /* 空缓存问候只完成显示事务，不写入或消费正式缓存。 */
	uint32 token;
	uint32 session_id;
	uint32 request_sequence;
	int clock_id;
	int category_id;
	int msg_id;
	char language[DIVOOM_AI_TALK_LANGUAGE_MAX_LEN];
	divoom_ai_talk_card_view_t view;
	char big_text[DIVOOM_AI_TALK_CARD_TEXT_MAX_LEN];
	char small_text[DIVOOM_AI_TALK_CARD_TEXT_MAX_LEN];
	char small_text_secondary[DIVOOM_AI_TALK_CARD_TEXT_MAX_LEN];
} divoom_ai_talk_transaction_slot;

typedef struct _divoom_ai_talk_ctrl
{
	void *lock_handle;
	Pdivoom_ai_talk_category_cache cache_list;
	uint32 rand_seed;
	uint32 transaction_seed;
	uint32 stale_drop_count;
	uint8 request_enabled; /* 由当前表盘 AI 元素状态控制，关闭时禁止发送两个 AI 语料接口 */
	int active_clock_id;
	char active_language[DIVOOM_AI_TALK_LANGUAGE_MAX_LEN];
	divoom_ai_talk_transaction_slot transaction[DIVOOM_AI_TALK_TRANSACTION_SLOT_NUM];
	char pixel_community_msg[DIVOOM_AI_TALK_PIXEL_MSG_MAX_LEN];
} divoom_ai_talk_ctrl, *Pdivoom_ai_talk_ctrl;

static Pdivoom_ai_talk_ctrl pdivoom_ai_talk_handle = NULL;

static void divoom_ai_talk_lock(void);
static void divoom_ai_talk_unlock(void);
static void divoom_ai_talk_clear_all_cache(void);
static int divoom_ai_talk_ensure_handle(void);
static Pdivoom_ai_talk_category_cache divoom_ai_talk_load_cache(int clock_id,
	const char *language, int category_id);
static void divoom_ai_talk_queue_guide_images(int clock_id,
	const char *language);
static int divoom_ai_talk_request_category(int clock_id, const char *language,
	int category_id, uint32 request_sequence);
static void divoom_ai_talk_prefetch_guide_card(int clock_id,
	const char *language);

/*
 * HOT 云图把作者名写入当前图片名缓冲，自定义像素表盘则把本地文件路径写入同一缓冲。
 * 路径不能作为作者插入话术，否则 498 小框会直接暴露 /userdata 或 Windows 路径。
 */
static uint8 divoom_ai_talk_pixel_author_name_is_valid(const char *author_name)
{
	if (author_name == NULL || author_name[0] == '\0')
	{
		return 0;
	}
	if (strchr(author_name, '/') != NULL || strchr(author_name, '\\') != NULL)
	{
		return 0;
	}
	return 1;
}

/* 保持像素云显示元素的作者获取链路；自定义表盘没有作者时用空串替换占位符。 */
const char *divoom_ai_talk_get_pixel_community_author_name(void)
{
	char *image_name;

	image_name = divoom_light_app_get_cur_image_name();
	if (divoom_ai_talk_pixel_author_name_is_valid(image_name) == 0)
	{
		return "";
	}
	return image_name;
}

static int divoom_ai_talk_format_pixel_community_msg(const char *msg,
	const char *author_name, char *dest, int dest_size)
{
	const char *token_pos;
	const char *suffix;
	int prefix_len, author_len, suffix_len, total_len;

	if (msg == NULL || dest == NULL || dest_size <= 0 || msg[0] == 0)
	{
		return 0;
	}
	author_name = author_name ? author_name : "";
	token_pos = strstr(msg, DIVOOM_AI_TALK_PIXEL_AUTHOR_TOKEN);
	if (token_pos == NULL)
	{
		total_len = (int)strlen(msg);
		if (total_len >= dest_size)
		{
			return 0;
		}
		memcpy(dest, msg, (size_t)total_len + 1);
		return 1;
	}
	if (strstr(token_pos + strlen(DIVOOM_AI_TALK_PIXEL_AUTHOR_TOKEN),
			DIVOOM_AI_TALK_PIXEL_AUTHOR_TOKEN) != NULL)
	{
		return 0;
	}
	suffix = token_pos + strlen(DIVOOM_AI_TALK_PIXEL_AUTHOR_TOKEN);
	prefix_len = (int)(token_pos - msg);
	author_len = (int)strlen(author_name);
	suffix_len = (int)strlen(suffix);
	total_len = prefix_len + author_len + suffix_len;
	if (total_len >= dest_size)
	{
		return 0;
	}
	memcpy(dest, msg, (size_t)prefix_len);
	memcpy(dest + prefix_len,
		author_name, (size_t)author_len);
	memcpy(dest + prefix_len + author_len,
		suffix, (size_t)suffix_len + 1);
	return 1;
}

static char *divoom_ai_talk_build_pixel_community_msg(char *msg, const char *author_name)
{
	if (pdivoom_ai_talk_handle == NULL || msg == NULL || msg[0] == 0)
	{
		return NULL;
	}
	divoom_ai_talk_lock();
	if (divoom_ai_talk_format_pixel_community_msg(msg, author_name,
		pdivoom_ai_talk_handle->pixel_community_msg,
		(int)sizeof(pdivoom_ai_talk_handle->pixel_community_msg)) == 0)
	{
		divoom_ai_talk_unlock();
		return NULL;
	}
	divoom_ai_talk_unlock();
	return pdivoom_ai_talk_handle->pixel_community_msg;
}

char *divoom_ai_talk_prepare_pixel_community_msg(const char *msg)
{
	char msg_copy[DIVOOM_AI_TALK_PIXEL_MSG_MAX_LEN];
	const char *author_name;

	if (msg == NULL || msg[0] == '\0' ||
		divoom_ai_talk_ensure_handle() != 0)
	{
		return NULL;
	}
	strncpy(msg_copy, msg, sizeof(msg_copy) - 1);
	msg_copy[sizeof(msg_copy) - 1] = '\0';
	author_name = divoom_ai_talk_get_pixel_community_author_name();
	return divoom_ai_talk_build_pixel_community_msg(msg_copy, author_name);
}

static uint32 divoom_ai_talk_rand(void)
{
	if (pdivoom_ai_talk_handle == NULL)
	{
		return 1;
	}
	if (pdivoom_ai_talk_handle->rand_seed == 0)
	{
		pdivoom_ai_talk_handle->rand_seed = divoom_system_get_ticks() ^ 0xA1A17A1Bu;
		if (pdivoom_ai_talk_handle->rand_seed == 0)
		{
			pdivoom_ai_talk_handle->rand_seed = 1;
		}
	}
	pdivoom_ai_talk_handle->rand_seed =
		pdivoom_ai_talk_handle->rand_seed * 1103515245u + 12345u;
	return pdivoom_ai_talk_handle->rand_seed;
}

/* 返回与语料目录、服务器 Language 字段一致的 BCP-47 简写。 */
static const char *divoom_ai_talk_get_language(void)
{
	switch (divoom_main_set_lang_flag(-1))
	{
	case DIVOOM_LVGL_MENU_LANG_SC:
	{
		return "zh-hans";
	}
	case DIVOOM_LVGL_MENU_LANG_TC:
	{
		return "zh-hant";
	}
	case DIVOOM_LVGL_MENU_LANG_JP:
	{
		return "ja";
	}
	case DIVOOM_LVGL_MENU_LANG_Germany:
	{
		return "de";
	}
	case DIVOOM_LVGL_MENU_LANG_Thai:
	{
		return "th";
	}
	case DIVOOM_LVGL_MENU_LANG_Russian:
	{
		return "ru";
	}
	case DIVOOM_LVGL_MENU_LANG_Korean:
	{
		return "ko";
	}
	case DIVOOM_LVGL_MENU_LANG_Vietnam:
	{
		return "vi";
	}
	case DIVOOM_LVGL_MENU_LANG_French:
	{
		return "fr";
	}
	case DIVOOM_LVGL_MENU_LANG_Turkey:
	{
		return "tr";
	}
	case DIVOOM_LVGL_MENU_LANG_Spanish:
	{
		return "es";
	}
	case DIVOOM_LVGL_MENU_LANG_Portuguese:
	{
		return "pt";
	}
	case DIVOOM_LVGL_MENU_LANG_Indonesian:
	{
		return "id";
	}
	case DIVOOM_LVGL_MENU_LANG_Italy:
	{
		return "it";
	}
	case DIVOOM_LVGL_MENU_LANG_Ukraine:
	{
		return "uk";
	}
	case DIVOOM_LVGL_MENU_LANG_Swedish:
	{
		return "sv";
	}
	case DIVOOM_LVGL_MENU_LANG_Danish:
	{
		return "da";
	}
	case DIVOOM_LVGL_MENU_LANG_Poland:
	{
		return "pl";
	}
	case DIVOOM_LVGL_MENU_LANG_ENGLISH:
	default:
	{
		return "en";
	}
	}
}

static const divoom_ai_talk_scene_info *divoom_ai_talk_find_scene(int msg_type)
{
	int i;

	for (i = 0; i < (int)(sizeof(divoom_ai_talk_scene_list) /
		sizeof(divoom_ai_talk_scene_list[0])); i++)
	{
		if (divoom_ai_talk_scene_list[i].msg_type == msg_type)
		{
			return &divoom_ai_talk_scene_list[i];
		}
	}
	return NULL;
}

/* 旧服务器响应不回显 ClockId/Language 时，只能绑定到同 MsgType 的在途请求。 */
static Pdivoom_ai_talk_category_cache divoom_ai_talk_find_response_cache(
	Pdivoom_device_ai_talk_list plist)
{
	Pdivoom_ai_talk_category_cache cache, match_cache;

	if (pdivoom_ai_talk_handle == NULL || plist == NULL)
	{
		return NULL;
	}
	match_cache = NULL;
	cache = pdivoom_ai_talk_handle->cache_list;
	while (cache)
	{
		if (cache->request_pending && cache->category_id == plist->category_id &&
			(plist->clock_id <= 0 || cache->clock_id == plist->clock_id) &&
			(plist->language[0] == 0 || strcmp(cache->language, plist->language) == 0) &&
			(plist->request_sequence == 0 ||
			 cache->request_sequence == plist->request_sequence))
		{
			/* 身份字段缺失且存在多个同类在途请求时丢弃响应，不能把角色语料写错分片。 */
			if (match_cache)
			{
				return NULL;
			}
			match_cache = cache;
		}
		cache = cache->next;
	}
	return match_cache;
}

static const char *divoom_ai_talk_find_clock_dir(int clock_id)
{
	int i;

	for (i = 0; i < (int)(sizeof(divoom_ai_talk_clock_dir_list) /
		sizeof(divoom_ai_talk_clock_dir_list[0])); i++)
	{
		if (divoom_ai_talk_clock_dir_list[i].clock_id == clock_id)
		{
			return divoom_ai_talk_clock_dir_list[i].dir_name;
		}
	}
	return NULL;
}

static int divoom_ai_talk_get_role_index(int clock_id)
{
	int i;

	for (i = 0; i < (int)(sizeof(divoom_ai_talk_clock_dir_list) /
		sizeof(divoom_ai_talk_clock_dir_list[0])); i++)
	{
		if (divoom_ai_talk_clock_dir_list[i].clock_id == clock_id)
		{
			return i;
		}
	}
	return -1;
}

/* 五个角色共用图片模块目录，本地ID必须带角色维度，不能直接使用服务器MsgId。 */
static uint32 divoom_ai_talk_build_guide_image_id(int clock_id, int msg_id)
{
	int role_index;

	role_index = divoom_ai_talk_get_role_index(clock_id);
	if (msg_id <= 0 || role_index < 0 ||
		(uint32)msg_id > (0xFFFFFFFFU - (uint32)role_index) /
			DIVOOM_AI_TALK_GUIDE_ROLE_NUM)
	{
		return 0;
	}
	return (uint32)msg_id * DIVOOM_AI_TALK_GUIDE_ROLE_NUM +
		(uint32)role_index;
}

/* V2 与 V3 使用独立目录；旧路径只用于兼容读取，后续写入不会覆盖 V2。 */
static int divoom_ai_talk_build_cache_path_internal(char *path, int path_len,
	int clock_id, const char *language, int category_id, int create_dir,
	uint8 use_v3_path)
{
	char dir_path[DIVOOM_AI_TALK_PATH_MAX_LEN];
	char clock_dir[24];
	char file_name[32];
	const char *clock_dir_name;
	const divoom_ai_talk_scene_info *scene;

	if (path == NULL || path_len <= 0 || clock_id <= 0 || language == NULL || language[0] == 0)
	{
		return -1;
	}
	clock_dir_name = divoom_ai_talk_find_clock_dir(clock_id);
	if (clock_dir_name == NULL)
	{
		snprintf(clock_dir, sizeof(clock_dir), "clock_%d", clock_id);
		clock_dir_name = clock_dir;
	}
	scene = divoom_ai_talk_find_scene(category_id);
	if (scene)
	{
		strncpy(file_name, scene->file_name, sizeof(file_name) - 1);
	}
	else
	{
		snprintf(file_name, sizeof(file_name), "%02d.json", category_id);
	}
	if (use_v3_path)
	{
		snprintf(dir_path, sizeof(dir_path), "%s/%s/%s/v3/evergreen",
			DIVOOM_AI_TALK_CACHE_DIR, clock_dir_name, language);
	}
	else
	{
		snprintf(dir_path, sizeof(dir_path), "%s/%s/%s",
			DIVOOM_AI_TALK_CACHE_DIR, clock_dir_name, language);
	}
	if (create_dir)
	{
		/* 使用逻辑路径逐级创建，兼容仿真映射后的 Windows 反斜杠路径。 */
		(void)divoom_app_com_check_and_mk_dir(dir_path);
		if (divoom_port_file_exists(dir_path) == 0)
		{
			return -1;
		}
	}
	if (snprintf(path, (size_t)path_len, "%s/%s", dir_path, file_name) >= path_len)
	{
		return -1;
	}
	return 0;
}

static int divoom_ai_talk_build_cache_path(char *path, int path_len, int clock_id,
	const char *language, int category_id, int create_dir)
{
	return divoom_ai_talk_build_cache_path_internal(path, path_len, clock_id,
		language, category_id, create_dir,
		category_id >= DIVOOM_AI_TALK_MSG_TYPE_LITERATURE &&
		category_id <= DIVOOM_AI_TALK_MSG_TYPE_AI_BASICS ? 1 : 0);
}

/* 临时文件写完后再替换正式文件，掉电或写失败时保留上一份有效缓存。 */
static int divoom_ai_talk_write_file_atomic(const char *path, const uint8 *data, uint32 data_len)
{
	char temp_path[DIVOOM_AI_TALK_PATH_MAX_LEN + 8];
	char backup_path[DIVOOM_AI_TALK_PATH_MAX_LEN + 8];
	void *file_handle;
	size_t write_len;
	int had_old_file;

	if (path == NULL || data == NULL || data_len == 0)
	{
		return -1;
	}
	snprintf(temp_path, sizeof(temp_path), "%s.tmp", path);
	snprintf(backup_path, sizeof(backup_path), "%s.bak", path);
	file_handle = divoom_port_file_open(temp_path, "wb");
	if (file_handle == NULL)
	{
		return -1;
	}
	write_len = divoom_port_file_write(data, 1, data_len, file_handle);
	if (divoom_port_file_close(file_handle) != 0 || write_len != data_len)
	{
		(void)divoom_port_remove_file(temp_path);
		return -1;
	}

	had_old_file = divoom_port_file_exists(path);
	(void)divoom_port_remove_file(backup_path);
	if (had_old_file && divoom_port_rename_file(path, backup_path) != 0)
	{
		(void)divoom_port_remove_file(temp_path);
		return -1;
	}
	if (divoom_port_rename_file(temp_path, path) != 0)
	{
		if (had_old_file)
		{
			(void)divoom_port_rename_file(backup_path, path);
		}
		(void)divoom_port_remove_file(temp_path);
		return -1;
	}
	(void)divoom_port_remove_file(backup_path);
	return 0;
}

/* 格式升级后连同掉电保护副本一起删除，避免旧批次被恢复后再次命中。 */
static void divoom_ai_talk_remove_cache_file(const char *path)
{
	char temp_path[DIVOOM_AI_TALK_PATH_MAX_LEN + 8];
	char backup_path[DIVOOM_AI_TALK_PATH_MAX_LEN + 8];

	if (path == NULL || path[0] == '\0')
	{
		return;
	}
	snprintf(temp_path, sizeof(temp_path), "%s.tmp", path);
	snprintf(backup_path, sizeof(backup_path), "%s.bak", path);
	(void)divoom_port_remove_file(temp_path);
	(void)divoom_port_remove_file(backup_path);
	(void)divoom_port_remove_file(path);
}

/* 若掉电发生在正式文件改名窗口，优先读取并恢复上一份 .bak。 */
static char *divoom_ai_talk_read_file(const char *path)
{
	char backup_path[DIVOOM_AI_TALK_PATH_MAX_LEN + 8];
	char *file_buf;

	file_buf = divoom_app_com_read_file((char *)path, NULL);
	if (file_buf)
	{
		return file_buf;
	}
	snprintf(backup_path, sizeof(backup_path), "%s.bak", path);
	file_buf = divoom_app_com_read_file(backup_path, NULL);
	if (file_buf)
	{
		(void)divoom_port_rename_file(backup_path, path);
	}
	return file_buf;
}

#if defined(DIVOOM_SIMULATOR)
/*
 * 服务端新增语料尚未全部部署时，从工程 all_corpus 随机装入十条作为仿真批次。
 * 只要分类表中存在对应文件就允许回退；真机不编译此分支。
 */
static uint8 divoom_ai_talk_simulator_corpus_supported(int category_id)
{
	return divoom_ai_talk_find_scene(category_id) != NULL;
}

static const char *divoom_ai_talk_simulator_json_object_end(const char *object_start)
{
	const char *cursor;
	int depth;
	uint8 escaped;
	uint8 in_string;

	if (object_start == NULL || object_start[0] != '{')
	{
		return NULL;
	}
	depth = 0;
	escaped = 0;
	in_string = 0;
	for (cursor = object_start; *cursor; cursor++)
	{
		if (in_string)
		{
			if (escaped)
			{
				escaped = 0;
			}
			else if (*cursor == '\\')
			{
				escaped = 1;
			}
			else if (*cursor == '"')
			{
				in_string = 0;
			}
			continue;
		}
		if (*cursor == '"')
		{
			in_string = 1;
		}
		else if (*cursor == '{')
		{
			depth++;
		}
		else if (*cursor == '}' && --depth == 0)
		{
			return cursor + 1;
		}
	}
	return NULL;
}

/* 仿真语料数量以文件实际内容为准，不再维护分类数量表。 */
static int divoom_ai_talk_simulator_json_array_count(const char *array_start)
{
	const char *scan;
	const char *object_end;
	int count;

	if (array_start == NULL || array_start[0] != '[')
	{
		return -1;
	}
	count = 0;
	scan = array_start + 1;
	while (*scan && *scan != ']')
	{
		while (*scan == ' ' || *scan == '\t' || *scan == '\r' ||
			*scan == '\n' || *scan == ',')
		{
			scan++;
		}
		if (*scan == ']')
		{
			break;
		}
		if (*scan != '{')
		{
			return -1;
		}
		object_end = divoom_ai_talk_simulator_json_object_end(scan);
		if (object_end == NULL)
		{
			return -1;
		}
		count++;
		scan = object_end;
	}
	return count;
}

/* 只构造十条记录的小 JSON，再交给 cJSON；避免在 LVGL 节拍中解析 3000 个对象。 */
static char *divoom_ai_talk_build_simulator_batch(char *source,
	int clock_id, const char *language, const divoom_ai_talk_scene_info *scene,
	int *batch_start)
{
	char header[160];
	char *batch;
	char *write_pos;
	const char *list_key;
	const char *list_start;
	const char *scan;
	const char *object_end;
	const char *object_list[DIVOOM_AI_TALK_MSG_BATCH_NUM];
	size_t object_len[DIVOOM_AI_TALK_MSG_BATCH_NUM];
	size_t header_len;
	size_t output_len;
	int max_start;
	int source_num;
	int source_index;
	int start_index;
	int selected;
	int i;

	if (source == NULL || language == NULL || scene == NULL)
	{
		return NULL;
	}
	list_key = strstr(source, "\"MsgList\"");
	list_start = list_key ? strchr(list_key, '[') : NULL;
	if (list_start == NULL)
	{
		divoom_system_free(source);
		return NULL;
	}
	source_num = divoom_ai_talk_simulator_json_array_count(list_start);
	if (source_num < DIVOOM_AI_TALK_MSG_BATCH_NUM)
	{
		divoom_system_free(source);
		return NULL;
	}
	max_start = source_num - DIVOOM_AI_TALK_MSG_BATCH_NUM + 1;
	start_index = (int)(divoom_ai_talk_rand() % (uint32)max_start);
	source_index = 0;
	selected = 0;
	scan = list_start + 1;
	while (*scan && *scan != ']' && selected < DIVOOM_AI_TALK_MSG_BATCH_NUM)
	{
		while (*scan == ' ' || *scan == '\t' || *scan == '\r' ||
			*scan == '\n' || *scan == ',')
		{
			scan++;
		}
		if (*scan == ']')
		{
			break;
		}
		if (*scan != '{')
		{
			divoom_system_free(source);
			return NULL;
		}
		object_end = divoom_ai_talk_simulator_json_object_end(scan);
		if (object_end == NULL)
		{
			divoom_system_free(source);
			return NULL;
		}
		if (source_index >= start_index)
		{
			object_list[selected] = scan;
			object_len[selected] = (size_t)(object_end - scan);
			selected++;
		}
		source_index++;
		scan = object_end;
	}
	if (selected != DIVOOM_AI_TALK_MSG_BATCH_NUM)
	{
		divoom_system_free(source);
		return NULL;
	}

	header_len = (size_t)snprintf(header, sizeof(header),
		"{\"ClockId\":%d,\"Lanugage\":\"%s\",\"MsgType\":%d,\"MsgList\":[",
		clock_id, language, scene->msg_type);
	output_len = header_len + 2;
	for (i = 0; i < selected; i++)
	{
		output_len += object_len[i] + (i > 0 ? 1 : 0);
	}
	batch = divoom_system_malloc(output_len + 1);
	if (batch == NULL)
	{
		divoom_system_free(source);
		return NULL;
	}
	write_pos = batch;
	memcpy(write_pos, header, header_len);
	write_pos += header_len;
	for (i = 0; i < selected; i++)
	{
		if (i > 0)
		{
			*write_pos++ = ',';
		}
		memcpy(write_pos, object_list[i], object_len[i]);
		write_pos += object_len[i];
	}
	*write_pos++ = ']';
	*write_pos++ = '}';
	*write_pos = '\0';
	divoom_system_free(source);
	if (batch_start)
	{
		*batch_start = start_index;
	}
	return batch;
}

static char *divoom_ai_talk_read_simulator_corpus(int clock_id,
	const char *language, int category_id)
{
	char path[DIVOOM_AI_TALK_SIM_PATH_MAX_LEN];
	char *file_buf;
	const char *clock_dir_name;
	const char *resource_root;
	const divoom_ai_talk_scene_info *scene;
	int batch_start;

	if (divoom_ai_talk_simulator_corpus_supported(category_id) == 0)
	{
		return NULL;
	}
	clock_dir_name = divoom_ai_talk_find_clock_dir(clock_id);
	scene = divoom_ai_talk_find_scene(category_id);
	if (clock_dir_name == NULL || scene == NULL || language == NULL || language[0] == 0)
	{
		return NULL;
	}

	/* run_sim_f5 的 CWD 是 LvglAstroTooSimulator，先走最短稳定路径。 */
	snprintf(path, sizeof(path), "all_corpus/%s/%s/%s", clock_dir_name,
		language, scene->file_name);
	file_buf = divoom_ai_talk_read_file(path);
	if (file_buf == NULL)
	{
		/* 兼容直接从仓库根目录启动模拟器。 */
		snprintf(path, sizeof(path), "LvglAstroTooSimulator/all_corpus/%s/%s/%s",
			clock_dir_name, language, scene->file_name);
		file_buf = divoom_ai_talk_read_file(path);
	}
	if (file_buf == NULL)
	{
		/* 兼容任意 CWD：DIVOOM_SIM_RESOURCE 的同级目录就是 all_corpus。 */
		resource_root = getenv("DIVOOM_SIM_RESOURCE");
		if (resource_root && resource_root[0])
		{
			snprintf(path, sizeof(path), "%s/../all_corpus/%s/%s/%s",
				resource_root, clock_dir_name, language, scene->file_name);
			file_buf = divoom_ai_talk_read_file(path);
		}
	}
	if (file_buf)
	{
		batch_start = 0;
		file_buf = divoom_ai_talk_build_simulator_batch(file_buf, clock_id,
			language, scene, &batch_start);
		if (file_buf)
		{
			divoom_print("[ai_talk] simulator corpus clock=%d language=%s type=%d "
				"start=%d count=%d path=%s\n", clock_id, language, category_id,
				batch_start + 1, DIVOOM_AI_TALK_MSG_BATCH_NUM, path);
		}
	}
	return file_buf;
}
#endif

static void divoom_ai_talk_lock(void)
{
	if (pdivoom_ai_talk_handle && pdivoom_ai_talk_handle->lock_handle)
	{
		divoom_system_get_lock(pdivoom_ai_talk_handle->lock_handle);
	}
}

static void divoom_ai_talk_unlock(void)
{
	if (pdivoom_ai_talk_handle && pdivoom_ai_talk_handle->lock_handle)
	{
		divoom_system_free_lock(pdivoom_ai_talk_handle->lock_handle);
	}
}

static int divoom_ai_talk_ensure_handle(void)
{
	if (pdivoom_ai_talk_handle)
	{
		return 0;
	}

	pdivoom_ai_talk_handle = (Pdivoom_ai_talk_ctrl)divoom_system_malloc(sizeof(divoom_ai_talk_ctrl));
	if (pdivoom_ai_talk_handle == NULL)
	{
		return -1;
	}
	pdivoom_ai_talk_handle->lock_handle = divoom_system_create_lock();
	if (pdivoom_ai_talk_handle->lock_handle == NULL)
	{
		divoom_system_free(pdivoom_ai_talk_handle);
		pdivoom_ai_talk_handle = NULL;
		return -1;
	}
	return 0;
}

static uint8 divoom_ai_talk_request_is_enabled(void)
{
	uint8 enabled;

	if (pdivoom_ai_talk_handle == NULL)
	{
		return 0;
	}
	divoom_ai_talk_lock();
	enabled = pdivoom_ai_talk_handle->request_enabled;
	divoom_ai_talk_unlock();
	return enabled;
}

void divoom_ai_talk_set_request_enabled(uint8 enabled)
{
	if (divoom_ai_talk_ensure_handle() != 0)
	{
		return;
	}
	divoom_ai_talk_lock();
	pdivoom_ai_talk_handle->request_enabled = enabled ? 1 : 0;
	if (enabled == 0)
	{
		/* 退出 AI 表盘后磁盘分片仍保留，RAM 批次和未提交事务必须回到零。 */
		memset(pdivoom_ai_talk_handle->transaction, 0,
			sizeof(pdivoom_ai_talk_handle->transaction));
		divoom_ai_talk_clear_all_cache();
		pdivoom_ai_talk_handle->active_clock_id = 0;
		pdivoom_ai_talk_handle->active_language[0] = '\0';
	}
	divoom_ai_talk_unlock();
}

void divoom_ai_talk_set_active_scope(int clock_id)
{
	const char *language;

	if (clock_id <= 0 || divoom_ai_talk_ensure_handle() != 0)
	{
		return;
	}
	language = divoom_ai_talk_get_language();
	divoom_ai_talk_lock();
	if (pdivoom_ai_talk_handle->active_clock_id != clock_id ||
		strcmp(pdivoom_ai_talk_handle->active_language, language) != 0)
	{
		/* RAM 只保留当前角色/语言批次；磁盘分片仍可在下次进入时复用。 */
		memset(pdivoom_ai_talk_handle->transaction, 0,
			sizeof(pdivoom_ai_talk_handle->transaction));
		divoom_ai_talk_clear_all_cache();
		pdivoom_ai_talk_handle->active_clock_id = clock_id;
		strncpy(pdivoom_ai_talk_handle->active_language, language,
			sizeof(pdivoom_ai_talk_handle->active_language) - 1);
	}
	divoom_ai_talk_unlock();

	/* 图鉴卡位于后段槽位，进入表盘时预取可给语料与图片下载留下充足时间。 */
	divoom_ai_talk_prefetch_guide_card(clock_id, language);
}

static void divoom_ai_talk_free_cache_messages(Pdivoom_ai_talk_category_cache cache)
{
	int i;

	if (cache == NULL)
	{
		return;
	}
	for (i = 0; i < DIVOOM_AI_TALK_MSG_BATCH_NUM; i++)
	{
		if (cache->msg[i])
		{
			divoom_free(cache->msg[i]);
			cache->msg[i] = NULL;
		}
	}
}

static void divoom_ai_talk_free_cache_item(Pdivoom_ai_talk_category_cache cache)
{
	if (cache == NULL)
	{
		return;
	}
	divoom_ai_talk_free_cache_messages(cache);
	memset(cache->msg_id, 0, sizeof(cache->msg_id));
	memset(cache->file_id, 0, sizeof(cache->file_id));
	cache->msg_num = 0;
	cache->used_num = 0;
	cache->request_pending = 0;
	cache->persistence_pending = 0;
	cache->request_tick = 0;
}

static void divoom_ai_talk_free_cache_list(Pdivoom_ai_talk_category_cache cache)
{
	Pdivoom_ai_talk_category_cache next;

	while (cache)
	{
		next = cache->next;
		divoom_ai_talk_free_cache_item(cache);
		divoom_system_free(cache);
		cache = next;
	}
}

static void divoom_ai_talk_clear_all_cache(void)
{
	if (pdivoom_ai_talk_handle == NULL)
	{
		return;
	}
	divoom_ai_talk_free_cache_list(pdivoom_ai_talk_handle->cache_list);
	pdivoom_ai_talk_handle->cache_list = NULL;
}

static Pdivoom_ai_talk_category_cache divoom_ai_talk_find_cache(int clock_id,
	const char *language, int category_id)
{
	Pdivoom_ai_talk_category_cache cache;

	if (pdivoom_ai_talk_handle == NULL || language == NULL)
	{
		return NULL;
	}
	cache = pdivoom_ai_talk_handle->cache_list;
	while (cache)
	{
		if (cache->clock_id == clock_id && cache->category_id == category_id &&
			strcmp(cache->language, language) == 0)
		{
			return cache;
		}
		cache = cache->next;
	}
	return NULL;
}

static Pdivoom_ai_talk_category_cache divoom_ai_talk_get_or_create_cache(int clock_id,
	const char *language, int category_id)
{
	Pdivoom_ai_talk_category_cache cache;

	cache = divoom_ai_talk_find_cache(clock_id, language, category_id);
	if (cache)
	{
		return cache;
	}

	cache = (Pdivoom_ai_talk_category_cache)divoom_system_malloc(sizeof(divoom_ai_talk_category_cache));
	if (cache == NULL)
	{
		return NULL;
	}
	cache->clock_id = clock_id;
	cache->category_id = category_id;
	strncpy(cache->language, language, sizeof(cache->language) - 1);
	cache->next = pdivoom_ai_talk_handle->cache_list;
	pdivoom_ai_talk_handle->cache_list = cache;
	return cache;
}

/* 图鉴卡额外保存 FileId；其他分类仍只读取 id 和 content。 */
static void divoom_ai_talk_load_msg_array(Pdivoom_ai_talk_category_cache cache,
	divoom_cJSON *msg_list)
{
	int i, source_num, start_index, source_index;
	divoom_cJSON *msg_item, *msg_obj;

	if (cache == NULL || msg_list == NULL ||
		(msg_list->type & 0xFF) != divoom_cJSON_Array)
	{
		return;
	}
	source_num = divoom_cJSON_GetArraySize(msg_list);
	start_index = source_num > DIVOOM_AI_TALK_MSG_BATCH_NUM ?
		(int)(divoom_ai_talk_rand() % (uint32)source_num) : 0;
	for (i = 0; i < source_num && cache->msg_num < DIVOOM_AI_TALK_MSG_BATCH_NUM; i++)
	{
		source_index = (start_index + i) % source_num;
		msg_item = divoom_cJSON_GetArrayItem(msg_list, source_index);
		if (msg_item == NULL)
		{
			continue;
		}
		msg_obj = divoom_cJSON_GetObjectItem(msg_item, "content");
		if (msg_obj == NULL || msg_obj->valuestring == NULL || msg_obj->valuestring[0] == 0)
		{
			continue;
		}
		cache->msg_id[cache->msg_num] = divoom_cJSON_GetObjectInt(msg_item, "id");
		if (cache->category_id == DIVOOM_AI_TALK_MSG_TYPE_GUIDE_CARD)
		{
			divoom_cJSON_GetObjectStringN(msg_item, "FileId",
				cache->file_id[cache->msg_num],
				sizeof(cache->file_id[cache->msg_num]));
			if (cache->msg_id[cache->msg_num] <= 0 ||
				cache->file_id[cache->msg_num][0] == '\0')
			{
				continue;
			}
		}
		cache->msg[cache->msg_num] = divoom_app_com_copy_string(msg_obj->valuestring, 0);
		if (cache->msg[cache->msg_num])
		{
			cache->msg_num++;
		}
	}
}

/* 33分类只把当前顺序项放入RAM，其余内容继续留在文件中。 */
static void divoom_ai_talk_load_greeting_current(
	Pdivoom_ai_talk_category_cache cache, divoom_cJSON *msg_list, int used_num)
{
	divoom_cJSON *msg_item;
	divoom_cJSON *msg_obj;
	int source_num;

	if (cache == NULL || msg_list == NULL ||
		(msg_list->type & 0xFF) != divoom_cJSON_Array)
	{
		return;
	}
	source_num = divoom_cJSON_GetArraySize(msg_list);
	if (source_num > DIVOOM_AI_TALK_MSG_BATCH_NUM)
	{
		source_num = DIVOOM_AI_TALK_MSG_BATCH_NUM;
	}
	if (used_num < 0)
	{
		used_num = 0;
	}
	if (used_num > source_num)
	{
		used_num = source_num;
	}
	cache->msg_num = source_num;
	cache->used_num = used_num;
	if (used_num >= source_num)
	{
		return;
	}
	msg_item = divoom_cJSON_GetArrayItem(msg_list, used_num);
	msg_obj = msg_item ? divoom_cJSON_GetObjectItem(msg_item, "content") : NULL;
	if (msg_obj == NULL || msg_obj->valuestring == NULL ||
		msg_obj->valuestring[0] == '\0')
	{
		return;
	}
	cache->msg_id[used_num] = divoom_cJSON_GetObjectInt(msg_item, "id");
	cache->msg[used_num] = divoom_app_com_copy_string(msg_obj->valuestring, 0);
}

static void divoom_ai_talk_cache_response_text(Pdivoom_ai_talk_category_cache cache,
	const char *text, int32 msg_id, const char *file_id)
{
	int32 index = cache->msg_num;
	if (text == NULL || index >= DIVOOM_AI_TALK_MSG_BATCH_NUM)
	{
		return;
	}
	cache->msg[index] = divoom_app_com_copy_string(text, 0);
	if (cache->msg[index])
	{
		cache->msg_id[index] = msg_id;
		if (file_id)
		{
			strncpy(cache->file_id[index], file_id,
				sizeof(cache->file_id[index]) - 1);
		}
		cache->msg_num++;
	}
}

static Pdivoom_ai_talk_category_cache divoom_ai_talk_load_legacy_cache_list(
	divoom_cJSON *root, const char *language)
{
	int i, cache_num, used_num;
	divoom_cJSON *list, *item, *msg_list;
	Pdivoom_ai_talk_category_cache cache, head;

	list = divoom_cJSON_GetObjectItem(root, "MsgCacheList");
	if (list == NULL || (list->type & 0xFF) != divoom_cJSON_Array)
	{
		return NULL;
	}
	head = NULL;
	cache_num = divoom_cJSON_GetArraySize(list);
	for (i = 0; i < cache_num; i++)
	{
		item = divoom_cJSON_GetArrayItem(list, i);
		if (item == NULL)
		{
			continue;
		}
		cache = (Pdivoom_ai_talk_category_cache)divoom_system_malloc(
			sizeof(divoom_ai_talk_category_cache));
		if (cache == NULL)
		{
			divoom_ai_talk_free_cache_list(head);
			return NULL;
		}
		cache->clock_id = DIVOOM_AI_TALK_LEGACY_CLOCK_ID;
		cache->category_id = divoom_cJSON_GetObjectInt(item, "CategoryId");
		strncpy(cache->language, language, sizeof(cache->language) - 1);
		msg_list = divoom_cJSON_GetObjectItem(item, "MsgList");
		divoom_ai_talk_load_msg_array(cache, msg_list);
		used_num = divoom_cJSON_GetObjectInt(item, "UsedNum");
		if (used_num < 0)
		{
			used_num = 0;
		}
		if (used_num > cache->msg_num)
		{
			used_num = cache->msg_num;
		}
		cache->used_num = used_num;
		cache->next = head;
		head = cache;
	}
	return head;
}

static Pdivoom_ai_talk_category_cache divoom_ai_talk_load_cache_file(int clock_id,
	const char *language, int category_id)
{
	char path[DIVOOM_AI_TALK_PATH_MAX_LEN];
	char *file_buf;
	const char *saved_language;
	divoom_cJSON *root, *msg_list, *language_obj;
	Pdivoom_ai_talk_category_cache cache;
	int saved_clock_id, saved_category_id, saved_version, used_num;
#if defined(DIVOOM_SIMULATOR)
	uint8 simulator_source;
#endif

	if (divoom_ai_talk_build_cache_path(path, sizeof(path), clock_id, language,
		category_id, 0) != 0)
	{
		return NULL;
	}
#if defined(DIVOOM_SIMULATOR)
	simulator_source = 0;
#endif
	file_buf = divoom_ai_talk_read_file(path);
	if (file_buf == NULL && category_id >= DIVOOM_AI_TALK_MSG_TYPE_LITERATURE &&
		category_id <= DIVOOM_AI_TALK_MSG_TYPE_AI_BASICS &&
		divoom_ai_talk_build_cache_path_internal(path, sizeof(path), clock_id,
			language, category_id, 0, 0) == 0)
	{
		/* 兼容早期把 V3 批次写进 V2 目录的版本，只读不回写旧路径。 */
		file_buf = divoom_ai_talk_read_file(path);
	}
#if defined(DIVOOM_SIMULATOR)
	if (file_buf == NULL)
	{
		file_buf = divoom_ai_talk_read_simulator_corpus(clock_id,
			language, category_id);
		if (file_buf)
		{
			simulator_source = 1;
		}
	}
	if (file_buf && simulator_source &&
		category_id == DIVOOM_AI_TALK_MSG_TYPE_GREETING &&
		divoom_ai_talk_build_cache_path(path, sizeof(path), clock_id, language,
			category_id, 1) == 0)
	{
		/* 仿真语料先落到运行缓存文件，后续每次只顺序读取当前项。 */
		(void)divoom_ai_talk_write_file_atomic(path, (uint8 *)file_buf,
			(uint32)strlen(file_buf));
	}
#endif
	if (file_buf == NULL)
	{
		return NULL;
	}
	root = divoom_cJSON_Parse(file_buf);
	divoom_system_free(file_buf);
	if (root == NULL)
	{
		return NULL;
	}
	/* all_corpus 是仿真只读源，不要求运行缓存版本；设备缓存必须严格匹配。 */
#if defined(DIVOOM_SIMULATOR)
	if (simulator_source == 0)
#endif
	{
		saved_version = divoom_cJSON_GetObjectInt(root, "Version");
		if (saved_version != DIVOOM_AI_TALK_SAVE_VERSION)
		{
			divoom_cJSON_Delete(root);
			if (saved_version < DIVOOM_AI_TALK_SAVE_VERSION)
			{
				divoom_print("[ai_talk] remove old cache version=%d current=%d path=%s\n",
					saved_version, DIVOOM_AI_TALK_SAVE_VERSION, path);
				divoom_ai_talk_remove_cache_file(path);
			}
			return NULL;
		}
	}
	saved_clock_id = divoom_cJSON_GetObjectInt(root, "ClockId");
	saved_category_id = divoom_cJSON_GetObjectInt(root, "MsgType");
	language_obj = divoom_cJSON_GetObjectItem(root, "Lanugage");
	if (language_obj == NULL)
	{
		language_obj = divoom_cJSON_GetObjectItem(root, "Language");
	}
	saved_language = language_obj && language_obj->valuestring ? language_obj->valuestring : NULL;
	if ((saved_clock_id > 0 && saved_clock_id != clock_id) ||
		(saved_category_id > 0 && saved_category_id != category_id) ||
		(saved_language && strcmp(saved_language, language) != 0))
	{
		divoom_cJSON_Delete(root);
		return NULL;
	}

	cache = (Pdivoom_ai_talk_category_cache)divoom_system_malloc(
		sizeof(divoom_ai_talk_category_cache));
	if (cache == NULL)
	{
		divoom_cJSON_Delete(root);
		return NULL;
	}
	cache->clock_id = clock_id;
	cache->category_id = category_id;
	cache->revision = (uint32)divoom_cJSON_GetObjectInt(root, "Revision");
	strncpy(cache->language, language, sizeof(cache->language) - 1);
	msg_list = divoom_cJSON_GetObjectItem(root, "MsgList");
	used_num = divoom_cJSON_GetObjectInt(root, "UsedNum");
	if (category_id == DIVOOM_AI_TALK_MSG_TYPE_GREETING)
	{
		divoom_ai_talk_load_greeting_current(cache, msg_list, used_num);
	}
	else
	{
		divoom_ai_talk_load_msg_array(cache, msg_list);
		if (used_num < 0)
		{
			used_num = 0;
		}
		if (used_num > cache->msg_num)
		{
			used_num = cache->msg_num;
		}
		cache->used_num = used_num;
	}
	divoom_cJSON_Delete(root);
#if defined(DIVOOM_SIMULATOR)
	if ((cache->msg_num <= 0 || cache->used_num >= cache->msg_num) &&
		simulator_source == 0 &&
		divoom_ai_talk_simulator_corpus_supported(category_id))
	{
		/* 空旧格式缓存或已耗尽批次改从当前 MsgList/content 工程语料装入。 */
		divoom_ai_talk_free_cache_item(cache);
		divoom_system_free(cache);
		file_buf = divoom_ai_talk_read_simulator_corpus(clock_id,
			language, category_id);
		if (file_buf == NULL)
		{
			return NULL;
		}
		simulator_source = 1;
		if (category_id == DIVOOM_AI_TALK_MSG_TYPE_GREETING &&
			divoom_ai_talk_build_cache_path(path, sizeof(path), clock_id,
				language, category_id, 1) == 0)
		{
			(void)divoom_ai_talk_write_file_atomic(path, (uint8 *)file_buf,
				(uint32)strlen(file_buf));
		}
		root = divoom_cJSON_Parse(file_buf);
		divoom_system_free(file_buf);
		if (root == NULL)
		{
			return NULL;
		}
		cache = (Pdivoom_ai_talk_category_cache)divoom_system_malloc(
			sizeof(divoom_ai_talk_category_cache));
		if (cache == NULL)
		{
			divoom_cJSON_Delete(root);
			return NULL;
		}
		cache->clock_id = clock_id;
		cache->category_id = category_id;
		strncpy(cache->language, language, sizeof(cache->language) - 1);
		msg_list = divoom_cJSON_GetObjectItem(root, "MsgList");
		if (category_id == DIVOOM_AI_TALK_MSG_TYPE_GREETING)
		{
			divoom_ai_talk_load_greeting_current(cache, msg_list, 0);
		}
		else
		{
			divoom_ai_talk_load_msg_array(cache, msg_list);
		}
		divoom_cJSON_Delete(root);
	}
#endif
	if (cache->msg_num <= 0)
	{
		divoom_ai_talk_free_cache_item(cache);
		divoom_system_free(cache);
		return NULL;
	}
	return cache;
}

#if defined(DIVOOM_SIMULATOR)
/*
 * RAM 中的十条批次耗尽后，重新从工程旧语料装入当前分类。
 * 保留原缓存节点地址，避免并发保存快照时观察到悬空节点。
 */
static int divoom_ai_talk_refill_simulator_cache(int clock_id,
	const char *language, int category_id)
{
	Pdivoom_ai_talk_category_cache cache;
	Pdivoom_ai_talk_category_cache fresh_cache;
	int i;
	int refilled;

	if (divoom_ai_talk_simulator_corpus_supported(category_id) == 0)
	{
		return 0;
	}
	fresh_cache = divoom_ai_talk_load_cache_file(clock_id, language, category_id);
	if (fresh_cache == NULL)
	{
		return 0;
	}

	refilled = 0;
	divoom_ai_talk_lock();
	cache = divoom_ai_talk_find_cache(clock_id, language, category_id);
	if (cache != NULL && cache->used_num >= cache->msg_num)
	{
		divoom_ai_talk_free_cache_item(cache);
		cache->revision = fresh_cache->revision;
		cache->generation++;
		if (cache->generation == 0)
		{
			cache->generation++;
		}
		cache->msg_num = fresh_cache->msg_num;
		cache->used_num = fresh_cache->used_num;
		for (i = 0; i < fresh_cache->msg_num; i++)
		{
			cache->msg_id[i] = fresh_cache->msg_id[i];
			strncpy(cache->file_id[i], fresh_cache->file_id[i],
				sizeof(cache->file_id[i]) - 1);
			cache->msg[i] = fresh_cache->msg[i];
			fresh_cache->msg[i] = NULL;
		}
		fresh_cache->msg_num = 0;
		fresh_cache->used_num = 0;
		refilled = 1;
	}
	divoom_ai_talk_unlock();

	divoom_ai_talk_free_cache_item(fresh_cache);
	divoom_system_free(fresh_cache);
	return refilled;
}
#endif

/* 下载管理器会复制任务内容；锁内只制作固定大小快照，不跨模块持锁。 */
static void divoom_ai_talk_queue_guide_images(int clock_id,
	const char *language)
{
	divoom_manage_image_item_info image_list[DIVOOM_AI_TALK_MSG_BATCH_NUM];
	Pdivoom_ai_talk_category_cache cache;
	uint32 image_id;
	int i;
	int image_num;

	if (language == NULL || language[0] == '\0')
	{
		return;
	}
	memset(image_list, 0, sizeof(image_list));
	image_num = 0;
	divoom_ai_talk_lock();
	cache = divoom_ai_talk_find_cache(clock_id, language,
		DIVOOM_AI_TALK_MSG_TYPE_GUIDE_CARD);
	if (cache)
	{
		for (i = 0; i < cache->msg_num &&
			image_num < DIVOOM_AI_TALK_MSG_BATCH_NUM; i++)
		{
			if (cache->msg_id[i] <= 0 || cache->file_id[i][0] == '\0')
			{
				continue;
			}
			image_id = divoom_ai_talk_build_guide_image_id(clock_id,
				cache->msg_id[i]);
			if (image_id == 0)
			{
				continue;
			}
			image_list[image_num].image_id = image_id;
			strncpy(image_list[image_num].file_addr, cache->file_id[i],
				sizeof(image_list[image_num].file_addr) - 1);
			image_num++;
		}
	}
	divoom_ai_talk_unlock();
	if (image_num > 0)
	{
		divoom_manage_image_add_font(DIVOOM_MANAGE_IMAGE_AI_GUIDE_CARD_IMG,
			image_list, (uint32)image_num);
	}
}

static void divoom_ai_talk_delete_stale_guide_images(const uint32 *old_ids,
	int old_num, const uint32 *new_ids, int new_num)
{
	uint32 stale_ids[DIVOOM_AI_TALK_MSG_BATCH_NUM];
	int i;
	int j;
	int stale_num;

	stale_num = 0;
	for (i = 0; i < old_num && i < DIVOOM_AI_TALK_MSG_BATCH_NUM; i++)
	{
		for (j = 0; j < new_num; j++)
		{
			if (old_ids[i] == new_ids[j])
			{
				break;
			}
		}
		if (old_ids[i] != 0 && j >= new_num)
		{
			stale_ids[stale_num++] = old_ids[i];
		}
	}
	if (stale_num > 0)
	{
		/* 图片目录没有独立索引，删除文件时不触发额外列表写盘。 */
		divoom_manage_image_del(DIVOOM_MANAGE_IMAGE_AI_GUIDE_CARD_IMG,
			stale_ids, (uint32)stale_num, 1);
	}
}

/* 文件读取在锁外完成；插入时再次查键，保证并发首次读取只保留一份缓存。 */
static Pdivoom_ai_talk_category_cache divoom_ai_talk_load_cache(int clock_id,
	const char *language, int category_id)
{
	Pdivoom_ai_talk_category_cache cache, old_cache;

	cache = divoom_ai_talk_load_cache_file(clock_id, language, category_id);
	if (cache == NULL)
	{
		return NULL;
	}
	divoom_ai_talk_lock();
	old_cache = divoom_ai_talk_find_cache(clock_id, language, category_id);
	if (old_cache == NULL)
	{
		if (cache->generation == 0)
		{
			cache->generation = 1;
		}
		cache->next = pdivoom_ai_talk_handle->cache_list;
		pdivoom_ai_talk_handle->cache_list = cache;
		old_cache = cache;
		cache = NULL;
	}
	divoom_ai_talk_unlock();
	if (cache)
	{
		divoom_ai_talk_free_cache_item(cache);
		divoom_system_free(cache);
	}
	if (old_cache && category_id == DIVOOM_AI_TALK_MSG_TYPE_GUIDE_CARD)
	{
		/* 重启或RAM作用域切换后，磁盘缓存仍要补齐未完成的图片下载。 */
		divoom_ai_talk_queue_guide_images(clock_id, language);
	}
	return old_cache;
}

/* 当前问候提交或取消后，按文件中的UsedNum重新装入下一条。 */
static int divoom_ai_talk_reload_greeting_cache(int clock_id,
	const char *language)
{
	Pdivoom_ai_talk_category_cache cache;
	Pdivoom_ai_talk_category_cache fresh_cache;
	uint32 expected_generation;
	int expected_used_num;
	int index;
	int loaded;
	uint8 expected_cache;

	expected_cache = 0;
	expected_generation = 0;
	expected_used_num = 0;
	divoom_ai_talk_lock();
	cache = divoom_ai_talk_find_cache(clock_id, language,
		DIVOOM_AI_TALK_MSG_TYPE_GREETING);
	if (cache)
	{
		if (cache->persistence_pending)
		{
			divoom_ai_talk_unlock();
			return 0;
		}
		expected_cache = 1;
		expected_generation = cache->generation;
		expected_used_num = cache->used_num;
	}
	divoom_ai_talk_unlock();
	fresh_cache = divoom_ai_talk_load_cache_file(clock_id, language,
		DIVOOM_AI_TALK_MSG_TYPE_GREETING);
	if (fresh_cache == NULL)
	{
		return 0;
	}
	loaded = 0;
	divoom_ai_talk_lock();
	cache = divoom_ai_talk_find_cache(clock_id, language,
		DIVOOM_AI_TALK_MSG_TYPE_GREETING);
	if (cache == NULL && expected_cache == 0)
	{
		fresh_cache->generation = 1;
		fresh_cache->next = pdivoom_ai_talk_handle->cache_list;
		pdivoom_ai_talk_handle->cache_list = fresh_cache;
		fresh_cache = NULL;
		loaded = 1;
	}
	else if (cache && expected_cache && cache->persistence_pending == 0 &&
		cache->generation == expected_generation &&
		cache->used_num == expected_used_num &&
		fresh_cache->used_num == expected_used_num)
	{
		divoom_ai_talk_free_cache_messages(cache);
		cache->msg_num = fresh_cache->msg_num;
		cache->used_num = fresh_cache->used_num;
		cache->revision = fresh_cache->revision;
		index = fresh_cache->used_num;
		if (index >= 0 && index < fresh_cache->msg_num)
		{
			cache->msg_id[index] = fresh_cache->msg_id[index];
			cache->msg[index] = fresh_cache->msg[index];
			fresh_cache->msg[index] = NULL;
		}
		loaded = 1;
	}
	divoom_ai_talk_unlock();
	if (fresh_cache)
	{
		divoom_ai_talk_free_cache_item(fresh_cache);
		divoom_system_free(fresh_cache);
	}
	return loaded;
}

static int divoom_ai_talk_save_cache(Pdivoom_ai_talk_category_cache cache)
{
	int i, ret;
	char path[DIVOOM_AI_TALK_PATH_MAX_LEN];
	char *json_info;
	divoom_cJSON *root, *msg_list, *msg_item;

	if (cache == NULL || cache->msg_num <= 0 ||
		divoom_ai_talk_build_cache_path(path, sizeof(path), cache->clock_id,
		cache->language, cache->category_id, 1) != 0)
	{
		return -1;
	}
	root = divoom_cJSON_CreateObject();
	msg_list = divoom_cJSON_CreateArray();
	if (root == NULL || msg_list == NULL)
	{
		if (root)
		{
			divoom_cJSON_Delete(root);
		}
		if (msg_list)
		{
			divoom_cJSON_Delete(msg_list);
		}
		return -1;
	}
	divoom_cJSON_AddNumberToObject(root, "Version", DIVOOM_AI_TALK_SAVE_VERSION);
	divoom_cJSON_AddNumberToObject(root, "ClockId", cache->clock_id);
	divoom_cJSON_AddStringToObject(root, "Lanugage", cache->language);
	divoom_cJSON_AddNumberToObject(root, "MsgType", cache->category_id);
	divoom_cJSON_AddNumberToObject(root, "Revision", cache->revision);
	divoom_cJSON_AddItemToObject(root, "MsgList", msg_list);

	divoom_ai_talk_lock();
	divoom_cJSON_AddNumberToObject(root, "UsedNum", cache->used_num);
	for (i = 0; i < cache->msg_num && i < DIVOOM_AI_TALK_MSG_BATCH_NUM; i++)
	{
		if (cache->msg[i] == NULL)
		{
			continue;
		}
		msg_item = divoom_cJSON_CreateObject();
		if (msg_item == NULL)
		{
			continue;
		}
		divoom_cJSON_AddItemToArray(msg_list, msg_item);
		divoom_cJSON_AddNumberToObject(msg_item, "id", cache->msg_id[i]);
		if (cache->category_id == DIVOOM_AI_TALK_MSG_TYPE_GUIDE_CARD &&
			cache->file_id[i][0] != '\0')
		{
			divoom_cJSON_AddStringToObject(msg_item, "FileId", cache->file_id[i]);
		}
		divoom_cJSON_AddStringToObject(msg_item, "content", cache->msg[i]);
	}
	divoom_ai_talk_unlock();
	json_info = divoom_cJSON_Print(root);
	divoom_cJSON_Delete(root);
	if (json_info == NULL)
	{
		return -1;
	}
	ret = divoom_ai_talk_write_file_atomic(path, (uint8 *)json_info,
		(uint32)strlen(json_info));
	divoom_system_free(json_info);
	return ret;
}

/* 33分类只更新文件游标，不把完整MsgList复制回RAM。 */
static int divoom_ai_talk_update_greeting_used_file(int clock_id,
	const char *language, int used_num)
{
	char path[DIVOOM_AI_TALK_PATH_MAX_LEN];
	char *file_buf;
	char *json_info;
	divoom_cJSON *root;
	int ret;

	if (divoom_ai_talk_build_cache_path(path, sizeof(path), clock_id, language,
		DIVOOM_AI_TALK_MSG_TYPE_GREETING, 1) != 0)
	{
		return -1;
	}
	file_buf = divoom_ai_talk_read_file(path);
	if (file_buf == NULL)
	{
		return -1;
	}
	root = divoom_cJSON_Parse(file_buf);
	divoom_system_free(file_buf);
	if (root == NULL)
	{
		return -1;
	}
	divoom_cJSON_DeleteItemFromObject(root, "UsedNum");
	divoom_cJSON_AddNumberToObject(root, "UsedNum", used_num);
	json_info = divoom_cJSON_Print(root);
	divoom_cJSON_Delete(root);
	if (json_info == NULL)
	{
		return -1;
	}
	ret = divoom_ai_talk_write_file_atomic(path, (uint8 *)json_info,
		(uint32)strlen(json_info));
	divoom_system_free(json_info);
	return ret;
}

/* 链表节点只允许在锁内访问；写盘使用私有快照，避免退出清缓存时悬空。 */
static Pdivoom_ai_talk_category_cache divoom_ai_talk_clone_cache_locked(
	Pdivoom_ai_talk_category_cache cache)
{
	Pdivoom_ai_talk_category_cache snapshot;
	int i;

	if (cache == NULL || cache->msg_num <= 0)
	{
		return NULL;
	}
	snapshot = (Pdivoom_ai_talk_category_cache)divoom_system_malloc(
		sizeof(divoom_ai_talk_category_cache));
	if (snapshot == NULL)
	{
		return NULL;
	}
	snapshot->clock_id = cache->clock_id;
	snapshot->category_id = cache->category_id;
	snapshot->msg_num = cache->msg_num;
	snapshot->used_num = cache->used_num;
	snapshot->revision = cache->revision;
	strncpy(snapshot->language, cache->language, sizeof(snapshot->language) - 1);
	for (i = 0; i < cache->msg_num && i < DIVOOM_AI_TALK_MSG_BATCH_NUM; i++)
	{
		snapshot->msg_id[i] = cache->msg_id[i];
		strncpy(snapshot->file_id[i], cache->file_id[i],
			sizeof(snapshot->file_id[i]) - 1);
		if (cache->msg[i])
		{
			snapshot->msg[i] = divoom_app_com_copy_string(cache->msg[i], 0);
			if (snapshot->msg[i] == NULL)
			{
				divoom_ai_talk_free_cache_item(snapshot);
				divoom_system_free(snapshot);
				return NULL;
			}
		}
	}
	return snapshot;
}

static int divoom_ai_talk_save_cache_by_key(int clock_id, const char *language,
	int category_id)
{
	Pdivoom_ai_talk_category_cache cache;
	Pdivoom_ai_talk_category_cache snapshot;
	int ret;

	if (pdivoom_ai_talk_handle == NULL || language == NULL)
	{
		return -1;
	}
	divoom_ai_talk_lock();
	cache = divoom_ai_talk_find_cache(clock_id, language, category_id);
	snapshot = divoom_ai_talk_clone_cache_locked(cache);
	divoom_ai_talk_unlock();
	if (snapshot == NULL)
	{
		return -1;
	}
	ret = divoom_ai_talk_save_cache(snapshot);
	divoom_ai_talk_free_cache_item(snapshot);
	divoom_system_free(snapshot);
	return ret;
}

static int divoom_ai_talk_load_info(void)
{
	char *file_buf;
	const char *language;
	divoom_cJSON *root;
	Pdivoom_ai_talk_category_cache new_cache_list;
	int saved_version;

	file_buf = divoom_ai_talk_read_file(DIVOOM_AI_TALK_SAVE_PATH);
	if (file_buf == NULL)
	{
		/* 没有版本索引时无法证明分类文件兼容，清空后建立当前版本索引。 */
		(void)divoom_port_remove_tree(DIVOOM_AI_TALK_CACHE_DIR);
		divoom_manage_image_remove(DIVOOM_MANAGE_IMAGE_AI_GUIDE_CARD_IMG, 0);
		return -1;
	}
	root = divoom_cJSON_Parse(file_buf);
	divoom_system_free(file_buf);
	if (root == NULL)
	{
		divoom_ai_talk_remove_cache_file(DIVOOM_AI_TALK_SAVE_PATH);
		(void)divoom_port_remove_tree(DIVOOM_AI_TALK_CACHE_DIR);
		divoom_manage_image_remove(DIVOOM_MANAGE_IMAGE_AI_GUIDE_CARD_IMG, 0);
		return -1;
	}
	saved_version = divoom_cJSON_GetObjectInt(root, "Version");
	if (saved_version != DIVOOM_AI_TALK_SAVE_VERSION)
	{
		divoom_cJSON_Delete(root);
		if (saved_version < DIVOOM_AI_TALK_SAVE_VERSION)
		{
			divoom_print("[ai_talk] remove old cache index version=%d current=%d\n",
				saved_version, DIVOOM_AI_TALK_SAVE_VERSION);
			divoom_ai_talk_remove_cache_file(DIVOOM_AI_TALK_SAVE_PATH);
			/* 格式升级后语料和图鉴图片身份都失效，统一删除后按需重新获取。 */
			(void)divoom_port_remove_tree(DIVOOM_AI_TALK_CACHE_DIR);
			divoom_manage_image_remove(DIVOOM_MANAGE_IMAGE_AI_GUIDE_CARD_IMG, 0);
		}
		return -1;
	}
	language = divoom_ai_talk_get_language();
	new_cache_list = divoom_ai_talk_load_legacy_cache_list(root, language);

	divoom_ai_talk_lock();
	divoom_ai_talk_clear_all_cache();
	pdivoom_ai_talk_handle->cache_list = new_cache_list;
	divoom_ai_talk_unlock();
	divoom_cJSON_Delete(root);
	return 0;
}

int divoom_ai_talk_save_info(void)
{
	int ret;
	char *json_info;
	divoom_cJSON *root;

	if (divoom_ai_talk_ensure_handle() != 0)
	{
		return -1;
	}
	root = divoom_cJSON_CreateObject();
	if (root == NULL)
	{
		return -1;
	}
	/* V5 索引只标记缓存格式，分类数量及修订号由消息批次响应管理。 */
	divoom_cJSON_AddNumberToObject(root, "Version", DIVOOM_AI_TALK_SAVE_VERSION);
	json_info = divoom_cJSON_Print(root);
	divoom_cJSON_Delete(root);
	if (json_info == NULL)
	{
		return -1;
	}
	ret = divoom_ai_talk_write_file_atomic(DIVOOM_AI_TALK_SAVE_PATH,
		(uint8 *)json_info, (uint32)strlen(json_info));
	divoom_system_free(json_info);
	return ret;
}

int divoom_ai_talk_refresh_classfy_list(void)
{
	/* 兼容旧调用；服务端已按场景直接分发消息，不再请求分类数量。 */
	return divoom_ai_talk_ensure_handle();
}

int divoom_ai_talk_first_update_classfy_list(void)
{
	/* 兼容旧页面初始化入口，只初始化模块，不产生网络请求。 */
	return divoom_ai_talk_ensure_handle();
}

static int divoom_ai_talk_request_category(int clock_id, const char *language,
	int category_id, uint32 request_sequence)
{
	int need_send;
	uint32 revision;
	uint32 now_tick;
	divoom_device_ai_talk_request_info req = {0};
	Pdivoom_ai_talk_category_cache cache;

	/* 3499 使用独立语音会话，不请求旧版 AI 语料分类。 */
	if (clock_id == 3499)
	{
		return -1;
	}
	category_id = divoom_ai_talk_normalize_category_id(category_id);
	if (divoom_ai_talk_request_is_enabled() == 0)
	{
		return -1;
	}
	if (divoom_net_app_net_is_ok() == 0)
	{
		return -1;
	}

	req.clock_id = clock_id;
	req.category_id = category_id;
	req.request_sequence = request_sequence;
	strncpy(req.language, language, sizeof(req.language) - 1);
	divoom_ai_talk_lock();
	cache = divoom_ai_talk_get_or_create_cache(clock_id, language, category_id);
	if (cache == NULL)
	{
		divoom_ai_talk_unlock();
		return -1;
	}
	revision = cache->revision;
	now_tick = divoom_system_get_ticks();
	need_send = cache->request_pending &&
		(uint32)(now_tick - cache->request_tick) < DIVOOM_AI_TALK_REQUEST_TIMEOUT_SEC ? 0 : 1;
	if (need_send)
	{
		cache->request_pending = 1;
		cache->request_tick = now_tick;
		cache->request_sequence = request_sequence;
	}
	divoom_ai_talk_unlock();

	if (need_send)
	{
		req.revision = revision;
		divoom_net_app_send_packet(DIVOOM_NET_COMM_GET_AI_TALK_LIST, DIVOOM_SERVER_TOPIC, &req);
	}
	return 0;
}

static void divoom_ai_talk_prefetch_guide_card(int clock_id,
	const char *language)
{
	Pdivoom_ai_talk_category_cache cache;
	int cache_loaded;
	int cache_ready;
	int msg_num;
	int used_num;
	int request_pending;
	int request_result;

	if (clock_id <= 0 || language == NULL || language[0] == '\0' ||
		divoom_ai_talk_request_is_enabled() == 0)
	{
		return;
	}

	cache_loaded = 0;
	divoom_ai_talk_lock();
	cache = divoom_ai_talk_find_cache(clock_id, language,
		DIVOOM_AI_TALK_MSG_TYPE_GUIDE_CARD);
	if (cache)
	{
		cache_loaded = 1;
	}
	divoom_ai_talk_unlock();

	if (cache_loaded == 0)
	{
		/* RAM 为空时先读磁盘；命中后 load_cache 会同步排队缺失图片。 */
		(void)divoom_ai_talk_load_cache(clock_id, language,
			DIVOOM_AI_TALK_MSG_TYPE_GUIDE_CARD);
	}

	cache_ready = 0;
	msg_num = 0;
	used_num = 0;
	request_pending = 0;
	divoom_ai_talk_lock();
	cache = divoom_ai_talk_find_cache(clock_id, language,
		DIVOOM_AI_TALK_MSG_TYPE_GUIDE_CARD);
	if (cache)
	{
		msg_num = cache->msg_num;
		used_num = cache->used_num;
		request_pending = cache->request_pending;
		cache_ready = cache->persistence_pending == 0 &&
			cache->used_num >= 0 && cache->used_num < cache->msg_num &&
			cache->msg[cache->used_num] != NULL &&
			cache->msg_id[cache->used_num] > 0 &&
			cache->file_id[cache->used_num][0] != '\0';
	}
	divoom_ai_talk_unlock();

	if (cache_ready)
	{
		/* 缓存有效也要重入下载队列，下载管理器负责对已存在文件去重。 */
		divoom_ai_talk_queue_guide_images(clock_id, language);
		divoom_print("[ai_guide_prefetch] clock=%d language=%s cache=ready used=%d/%d\n",
			clock_id, language, used_num, msg_num);
		return;
	}

	request_result = divoom_ai_talk_request_category(clock_id, language,
		DIVOOM_AI_TALK_MSG_TYPE_GUIDE_CARD, 0);
	divoom_print("[ai_guide_prefetch] clock=%d language=%s cache=empty used=%d/%d "
		"pending=%d request=%d\n", clock_id, language, used_num, msg_num,
		request_pending, request_result);
}

void divoom_ai_talk_classfy_list_respone(Pdivoom_device_ai_classfy_list pclassfy)
{
	/* 兼容旧命令分发表；新流程不会主动请求或保存分类数量。 */
	(void)pclassfy;
}

void divoom_ai_talk_msg_list_respone(Pdivoom_device_ai_talk_list plist)
{
	int i, valid_num;
	int save_cache;
	int save_result;
	int cache_clock_id;
	int cache_category_id;
	int old_guide_num;
	int new_guide_num;
	uint32 old_guide_ids[DIVOOM_AI_TALK_MSG_BATCH_NUM];
	uint32 new_guide_ids[DIVOOM_AI_TALK_MSG_BATCH_NUM];
	uint32 guide_image_id;
	const char *language;
	char cache_language[DIVOOM_AI_TALK_LANGUAGE_MAX_LEN];
	Pdivoom_ai_talk_category_cache cache;

	if (divoom_ai_talk_ensure_handle() != 0)
	{
		return;
	}
	if (plist == NULL || plist->category_id <= 0)
	{
		return;
	}
	memset(old_guide_ids, 0, sizeof(old_guide_ids));
	memset(new_guide_ids, 0, sizeof(new_guide_ids));
	old_guide_num = 0;
	new_guide_num = 0;

	language = plist->language[0] ? plist->language : divoom_ai_talk_get_language();
	divoom_ai_talk_lock();
	if (pdivoom_ai_talk_handle->request_enabled == 0)
	{
		pdivoom_ai_talk_handle->stale_drop_count++;
		divoom_ai_talk_unlock();
		return;
	}
	if (pdivoom_ai_talk_handle->active_clock_id > 0 &&
		((plist->clock_id > 0 &&
		  plist->clock_id != pdivoom_ai_talk_handle->active_clock_id) ||
		 strcmp(language, pdivoom_ai_talk_handle->active_language) != 0))
	{
		/* 旧角色/旧语言异步响应允许落到网络日志，但不能重建当前 RAM 缓存。 */
		pdivoom_ai_talk_handle->stale_drop_count++;
		divoom_ai_talk_unlock();
		return;
	}
	cache = divoom_ai_talk_find_response_cache(plist);
	if (cache == NULL && plist->clock_id > 0 && plist->request_sequence == 0)
	{
		cache = divoom_ai_talk_get_or_create_cache(plist->clock_id, language,
			plist->category_id);
	}
	if (cache == NULL)
	{
		pdivoom_ai_talk_handle->stale_drop_count++;
		divoom_ai_talk_unlock();
		return;
	}
	if (cache->persistence_pending)
	{
		/* 同一分片的提交正在更新UsedNum，不能让新批次并发覆盖文件。 */
		pdivoom_ai_talk_handle->stale_drop_count++;
		divoom_ai_talk_unlock();
		return;
	}

	valid_num = 0;
	for (i = 0; i < plist->msg_num; i++)
	{
		if (plist->plist && plist->plist[i].content && plist->plist[i].content[0] &&
			(cache->category_id != DIVOOM_AI_TALK_MSG_TYPE_GUIDE_CARD ||
			 (plist->plist[i].msg_id > 0 && plist->plist[i].file_id[0] != '\0')))
		{
			valid_num++;
		}
	}
	if (valid_num <= 0)
	{
		/* 空响应不清掉已用批次；15 秒后仍可按同一角色、语言和场景重试。 */
		cache->request_pending = 1;
		cache->request_tick = divoom_system_get_ticks();
		divoom_ai_talk_unlock();
		return;
	}

	if (cache->category_id == DIVOOM_AI_TALK_MSG_TYPE_GUIDE_CARD)
	{
		for (i = 0; i < cache->msg_num &&
			old_guide_num < DIVOOM_AI_TALK_MSG_BATCH_NUM; i++)
		{
			if (cache->msg_id[i] > 0)
			{
				guide_image_id = divoom_ai_talk_build_guide_image_id(
					cache->clock_id, cache->msg_id[i]);
				if (guide_image_id != 0)
				{
					old_guide_ids[old_guide_num++] = guide_image_id;
				}
			}
		}
	}
	divoom_ai_talk_free_cache_item(cache);
	for (i = 0; i < plist->msg_num && cache->msg_num < DIVOOM_AI_TALK_MSG_BATCH_NUM; i++)
	{
		if (plist->plist == NULL || plist->plist[i].content == NULL ||
			plist->plist[i].content[0] == 0 ||
			(cache->category_id == DIVOOM_AI_TALK_MSG_TYPE_GUIDE_CARD &&
			 (plist->plist[i].msg_id <= 0 || plist->plist[i].file_id[0] == '\0')))
		{
			continue;
		}
		divoom_ai_talk_cache_response_text(cache, plist->plist[i].content,
			plist->plist[i].msg_id, plist->plist[i].file_id);
		if (cache->category_id == DIVOOM_AI_TALK_MSG_TYPE_GUIDE_CARD &&
			cache->msg_num > new_guide_num)
		{
			guide_image_id = divoom_ai_talk_build_guide_image_id(
				cache->clock_id, cache->msg_id[cache->msg_num - 1]);
			if (guide_image_id != 0)
			{
				new_guide_ids[new_guide_num++] = guide_image_id;
			}
		}
	}
	cache->used_num = 0;
	cache->revision = plist->revision;
	cache->generation++;
	if (cache->generation == 0)
	{
		cache->generation++;
	}
	if (cache->msg_num > 0)
	{
		cache->request_pending = 0;
		cache->request_tick = 0;
	}
	else
	{
		/* 空响应进入统一超时退避，避免场景刷新时持续向服务器重发。 */
		cache->request_pending = 1;
		cache->request_tick = divoom_system_get_ticks();
	}
	save_cache = cache->msg_num > 0;
	if (save_cache)
	{
		cache_clock_id = cache->clock_id;
		cache_category_id = cache->category_id;
		strncpy(cache_language, cache->language, sizeof(cache_language) - 1);
		cache_language[sizeof(cache_language) - 1] = '\0';
		if (cache_category_id == DIVOOM_AI_TALK_MSG_TYPE_GREETING)
		{
			/* 发布新批前先阻断acquire/commit，确保完整MsgList先原子落盘。 */
			cache->persistence_pending = 1;
		}
	}
	divoom_ai_talk_unlock();

	if (save_cache)
	{
		if (cache_category_id == DIVOOM_AI_TALK_MSG_TYPE_GUIDE_CARD)
		{
			/* 收到有效批次立即下载；旧批中已移除的图片同时进入异步删除队列。 */
			divoom_ai_talk_queue_guide_images(cache_clock_id, cache_language);
			divoom_ai_talk_delete_stale_guide_images(old_guide_ids,
				old_guide_num, new_guide_ids, new_guide_num);
		}
		save_result = divoom_ai_talk_save_cache_by_key(cache_clock_id, cache_language,
			cache_category_id);
		if (cache_category_id == DIVOOM_AI_TALK_MSG_TYPE_GREETING)
		{
			divoom_ai_talk_lock();
			cache = divoom_ai_talk_find_cache(cache_clock_id, cache_language,
				cache_category_id);
			if (cache)
			{
				if (save_result == 0)
				{
					/* 33批次落盘后释放正文，RAM只保留顺序游标和批次数。 */
					divoom_ai_talk_free_cache_messages(cache);
					cache->persistence_pending = 0;
				}
				else
				{
					/* 文件无完整批次时禁止继续消费，下一次获取会重新请求。 */
					divoom_ai_talk_free_cache_item(cache);
				}
			}
			divoom_ai_talk_unlock();
		}
	}
}

void divoom_ai_talk_init(void)
{
	if (divoom_ai_talk_ensure_handle() != 0)
	{
		return;
	}

	divoom_ai_talk_lock();
	divoom_ai_talk_clear_all_cache();
	memset(pdivoom_ai_talk_handle->transaction, 0,
		sizeof(pdivoom_ai_talk_handle->transaction));
	divoom_ai_talk_unlock();

	if (divoom_ai_talk_load_info() != 0)
	{
		/* 首次启动或格式升级后立即固化当前版本，避免下次启动再次清空新缓存。 */
		(void)divoom_ai_talk_save_info();
	}
}

char *divoom_ai_talk_get_random_msg(int clock_id, int category_id)
{
	const char *language;
	Pdivoom_ai_talk_category_cache cache;
	char msg_copy[DIVOOM_AI_TALK_PIXEL_MSG_MAX_LEN];
	char *result;
	int msg_available;
	int request_new_batch;

	if (clock_id <= 0 || category_id <= 0)
	{
		return NULL;
	}
	category_id = divoom_ai_talk_normalize_category_id(category_id);
	if (category_id == DIVOOM_AI_TALK_MSG_TYPE_GREETING)
	{
		/* 33分类必须走acquire/commit文件游标事务，旧接口不能整批回写该文件。 */
		return NULL;
	}
	if (divoom_ai_talk_ensure_handle() != 0)
	{
		return NULL;
	}
	if (divoom_ai_talk_request_is_enabled() == 0)
	{
		return NULL;
	}

	language = divoom_ai_talk_get_language();
	msg_copy[0] = '\0';
	msg_available = 0;
	request_new_batch = 0;
	divoom_ai_talk_lock();
	cache = divoom_ai_talk_find_cache(clock_id, language, category_id);
	divoom_ai_talk_unlock();

	if (cache == NULL)
	{
		(void)divoom_ai_talk_load_cache(clock_id, language, category_id);
	}
	divoom_ai_talk_lock();
	cache = divoom_ai_talk_find_cache(clock_id, language, category_id);
	if (cache && cache->used_num < cache->msg_num)
	{
		if (cache->msg[cache->used_num] &&
			strlen(cache->msg[cache->used_num]) < sizeof(msg_copy))
		{
			strncpy(msg_copy, cache->msg[cache->used_num], sizeof(msg_copy) - 1);
			msg_copy[sizeof(msg_copy) - 1] = '\0';
			cache->used_num++;
			request_new_batch = cache->used_num >= cache->msg_num ? 1 : 0;
			msg_available = 1;
		}
	}
	divoom_ai_talk_unlock();
	if (msg_available)
	{
		/* UsedNum 每次取用后落盘；第十条取出时立即预取下一批。 */
		(void)divoom_ai_talk_save_cache_by_key(clock_id, language, category_id);
		if (request_new_batch)
		{
			(void)divoom_ai_talk_request_category(clock_id, language, category_id, 0);
		}
		divoom_ai_talk_lock();
		strncpy(pdivoom_ai_talk_handle->pixel_community_msg, msg_copy,
			sizeof(pdivoom_ai_talk_handle->pixel_community_msg) - 1);
		pdivoom_ai_talk_handle->pixel_community_msg[
			sizeof(pdivoom_ai_talk_handle->pixel_community_msg) - 1] = '\0';
		result = pdivoom_ai_talk_handle->pixel_community_msg;
		divoom_ai_talk_unlock();
		return result;
	}
	(void)divoom_ai_talk_request_category(clock_id, language, category_id, 0);
	return NULL;
}

static uint8 divoom_ai_talk_msg_uses_big_text(int msg_type)
{
	/* 大框归属由固定分类契约决定，网络和缓存无需重复保存类型字段。 */
	return (msg_type >= DIVOOM_AI_TALK_MSG_TYPE_LITERATURE &&
		msg_type <= DIVOOM_AI_TALK_MSG_TYPE_AI_BASICS) ||
		msg_type == DIVOOM_AI_TALK_MSG_TYPE_NEWS;
}

/* 拒绝非法 UTF-8 和超长字段，不能在半个字符上截断后交给 LVGL。 */
static int32 divoom_ai_talk_card_text_valid(const char *text, uint32 capacity,
	uint8 single_line)
{
	const uint8 *p = (const uint8 *)text;
	uint32 code, minimum;
	int32 remaining;
	if (text == NULL || strlen(text) >= capacity)
	{
		return 0;
	}
	while (*p)
	{
		if (*p < 0x80)
		{
			if (*p < 0x20 && (single_line || (*p != '\n' && *p != '\r')))
			{
				return 0;
			}
			p++;
			continue;
		}
		if (*p >= 0xC2 && *p <= 0xDF)
		{
			code = *p++ & 0x1F; remaining = 1; minimum = 0x80;
		}
		else if (*p >= 0xE0 && *p <= 0xEF)
		{
			code = *p++ & 0x0F; remaining = 2; minimum = 0x800;
		}
		else if (*p >= 0xF0 && *p <= 0xF4)
		{
			code = *p++ & 0x07; remaining = 3; minimum = 0x10000;
		}
		else
		{
			return 0;
		}
		while (remaining-- > 0)
		{
			if ((*p & 0xC0) != 0x80)
			{
				return 0;
			}
			code = (code << 6) | (*p++ & 0x3F);
		}
		if (code < minimum || code > 0x10FFFF || (code >= 0xD800 && code <= 0xDFFF))
		{
			return 0;
		}
	}
	return 1;
}

/*
 * 小框允许一条语料携带前后两段内容。拆分只作用于事务副本，磁盘及
 * 服务端原文保持不变；分隔符任一侧为空时退化为单段，绝不把标记显示出来。
 */
static int divoom_ai_talk_split_small_text(divoom_ai_talk_transaction_slot *slot)
{
	char *separator;
	char *secondary;
	char *primary_end;
	size_t secondary_len;

	if (slot == NULL || slot->small_text[0] == '\0')
	{
		return 0;
	}
	separator = strstr(slot->small_text, DIVOOM_AI_TALK_SMALL_TEXT_SEPARATOR);
	if (separator == NULL)
	{
		return 1;
	}
	secondary = separator + strlen(DIVOOM_AI_TALK_SMALL_TEXT_SEPARATOR);
	while (*secondary == ' ' || *secondary == '\t' ||
		*secondary == '\r' || *secondary == '\n')
	{
		secondary++;
	}
	secondary_len = strlen(secondary);
	while (secondary_len > 0 &&
		(secondary[secondary_len - 1] == ' ' ||
		 secondary[secondary_len - 1] == '\t' ||
		 secondary[secondary_len - 1] == '\r' ||
		 secondary[secondary_len - 1] == '\n'))
	{
		secondary_len--;
	}
	if (secondary_len >= sizeof(slot->small_text_secondary))
	{
		return 0;
	}
	if (secondary_len > 0)
	{
		memcpy(slot->small_text_secondary, secondary, secondary_len);
		slot->small_text_secondary[secondary_len] = '\0';
	}
	primary_end = separator;
	while (primary_end > slot->small_text &&
		(primary_end[-1] == ' ' || primary_end[-1] == '\t' ||
		 primary_end[-1] == '\r' || primary_end[-1] == '\n'))
	{
		primary_end--;
	}
	*primary_end = '\0';
	if (slot->small_text[0] == '\0' && slot->small_text_secondary[0] != '\0')
	{
		snprintf(slot->small_text, sizeof(slot->small_text), "%s",
			slot->small_text_secondary);
		slot->small_text_secondary[0] = '\0';
	}
	return slot->small_text[0] != '\0';
}

static int divoom_ai_talk_fill_card_view(divoom_ai_talk_transaction_slot *slot,
	const char *combined, int msg_type, uint8 use_big_text)
{
	const char *separator;
	const char *small_text;
	int32 big_len;
	int written;

	if (slot == NULL || combined == NULL || combined[0] == '\0')
	{
		return 0;
	}
	separator = strstr(combined, DIVOOM_AI_TALK_PORTRY_SHORT_SEPARATOR);
	if (separator)
	{
		big_len = (int32)(separator - combined);
		while (big_len > 0 &&
			(combined[big_len - 1] == '\r' || combined[big_len - 1] == '\n'))
		{
			big_len--;
		}
		small_text = separator + strlen(DIVOOM_AI_TALK_PORTRY_SHORT_SEPARATOR);
		while (*small_text == '\r' || *small_text == '\n')
		{
			small_text++;
		}
		if (big_len <= 0 || big_len >= (int32)sizeof(slot->big_text) ||
			small_text[0] == '\0' || strlen(small_text) >= sizeof(slot->small_text))
		{
			return 0;
		}
		memcpy(slot->big_text, combined, (size_t)big_len);
		slot->big_text[big_len] = '\0';
		snprintf(slot->small_text, sizeof(slot->small_text), "%s", small_text);
	}
	else if (use_big_text)
	{
		/*
		 * Buddy 旧类型 3/4 只有正文。投影到 2303 时只提供 532，
		 * 498 必须保留上一条有效短评，不能复制正文伪造成短评。
		 * 新的成对分类若缺少分隔符则视为坏数据，避免提交半套场景。
		 */
		if (msg_type == DIVOOM_AI_TALK_MSG_TYPE_PORTRY_SHORT ||
			(msg_type >= DIVOOM_AI_TALK_MSG_TYPE_LITERATURE &&
			 msg_type <= DIVOOM_AI_TALK_MSG_TYPE_AI_BASICS) ||
			msg_type == DIVOOM_AI_TALK_MSG_TYPE_GREETING)
		{
			return 0;
		}
		written = snprintf(slot->big_text, sizeof(slot->big_text), "%s", combined);
		if (written <= 0 || written >= (int)sizeof(slot->big_text))
		{
			return 0;
		}
	}
	else
	{
		written = snprintf(slot->small_text, sizeof(slot->small_text), "%s", combined);
		if (written <= 0 || written >= (int)sizeof(slot->small_text))
		{
			return 0;
		}
	}
	if (slot->small_text[0] != '\0' && divoom_ai_talk_split_small_text(slot) == 0)
	{
		return 0;
	}
	slot->view.msg_type = msg_type;
	slot->view.big_text = slot->big_text[0] ? slot->big_text : NULL;
	slot->view.small_text = slot->small_text[0] ? slot->small_text : NULL;
	slot->view.small_text_secondary = slot->small_text_secondary[0] ?
		slot->small_text_secondary : NULL;
	return 1;
}

static const char *divoom_ai_talk_default_greeting(int clock_id, const char *language)
{
	uint8 simplified;
	uint8 traditional;

	simplified = language && strcmp(language, "zh-hans") == 0;
	traditional = language && strcmp(language, "zh-hant") == 0;
	if (simplified)
	{
		switch (clock_id)
		{
		case DIVOOM_AI_TALK_CLOCK_ID_MOM:
		{
			return "先把脚步放稳，今天慢慢来。\n<DIVOOM_AI_COMMENT>\n我陪着你。";
		}
		case DIVOOM_AI_TALK_CLOCK_ID_DIVO:
		{
			return "新的信号到了，探索可以开始。\n<DIVOOM_AI_COMMENT>\n一起出发。";
		}
		case DIVOOM_AI_TALK_CLOCK_ID_RICH_GUY:
		{
			return "新一轮开始，先看清再行动。\n<DIVOOM_AI_COMMENT>\n从容一点。";
		}
		case DIVOOM_AI_TALK_CLOCK_ID_PROFESSOR:
		{
			return "今天从一个好问题开始。\n<DIVOOM_AI_COMMENT>\n一起找答案。";
		}
		default:
		{
			return "今天好像藏着新鲜事。\n<DIVOOM_AI_COMMENT>\n一起去看看。";
		}
		}
	}
	if (traditional)
	{
		switch (clock_id)
		{
		case DIVOOM_AI_TALK_CLOCK_ID_MOM:
		{
			return "先把腳步放穩，今天慢慢來。\n<DIVOOM_AI_COMMENT>\n我陪著你。";
		}
		case DIVOOM_AI_TALK_CLOCK_ID_DIVO:
		{
			return "新的訊號到了，探索可以開始。\n<DIVOOM_AI_COMMENT>\n一起出發。";
		}
		case DIVOOM_AI_TALK_CLOCK_ID_RICH_GUY:
		{
			return "新一輪開始，先看清再行動。\n<DIVOOM_AI_COMMENT>\n從容一點。";
		}
		case DIVOOM_AI_TALK_CLOCK_ID_PROFESSOR:
		{
			return "今天從一個好問題開始。\n<DIVOOM_AI_COMMENT>\n一起找答案。";
		}
		default:
		{
			return "今天好像藏著新鮮事。\n<DIVOOM_AI_COMMENT>\n一起去看看。";
		}
		}
	}
	switch (clock_id)
	{
	case DIVOOM_AI_TALK_CLOCK_ID_MOM:
	{
		return "Let's take today one steady step at a time.\n<DIVOOM_AI_COMMENT>\nI'm right here with you.";
	}
	case DIVOOM_AI_TALK_CLOCK_ID_DIVO:
	{
		return "A new signal is in. Our next exploration can begin.\n<DIVOOM_AI_COMMENT>\nReady when you are.";
	}
	case DIVOOM_AI_TALK_CLOCK_ID_RICH_GUY:
	{
		return "A fresh round begins; let's read the room first.\n<DIVOOM_AI_COMMENT>\nCalm choices travel farther.";
	}
	case DIVOOM_AI_TALK_CLOCK_ID_PROFESSOR:
	{
		return "Today can begin with one worthwhile question.\n<DIVOOM_AI_COMMENT>\nLet's examine it together.";
	}
	default:
	{
		return "Something new may be hiding in today.\n<DIVOOM_AI_COMMENT>\nLet's go take a look.";
	}
	}
}

const char *divoom_ai_talk_get_default_greeting(int clock_id)
{
	return divoom_ai_talk_default_greeting(clock_id, divoom_ai_talk_get_language());
}

/* 调用方已持有 talk 锁；默认问候不进入正式缓存，也不改变 used_num。 */
static int32 divoom_ai_talk_acquire_default_greeting_locked(
	const divoom_ai_talk_acquire_req_t *req, const char *language,
	divoom_ai_talk_card_view_t *card, uint32 *token)
{
	divoom_ai_talk_transaction_slot *slot;
	const char *text;
	int i;

	if (req->msg_type != DIVOOM_AI_TALK_MSG_TYPE_GREETING ||
		req->restore_msg_id != 0)
	{
		return 0;
	}
	for (i = 0; i < DIVOOM_AI_TALK_TRANSACTION_SLOT_NUM; i++)
	{
		if (pdivoom_ai_talk_handle->transaction[i].active &&
			pdivoom_ai_talk_handle->transaction[i].session_id == req->session_id &&
			pdivoom_ai_talk_handle->transaction[i].category_id == req->msg_type)
		{
			return 0;
		}
	}
	slot = NULL;
	for (i = 0; i < DIVOOM_AI_TALK_TRANSACTION_SLOT_NUM; i++)
	{
		if (pdivoom_ai_talk_handle->transaction[i].active == 0)
		{
			slot = &pdivoom_ai_talk_handle->transaction[i];
			break;
		}
	}
	if (slot == NULL)
	{
		return 0;
	}
	text = divoom_ai_talk_default_greeting(req->clock_id, language);
	memset(slot, 0, sizeof(*slot));
	if (divoom_ai_talk_fill_card_view(slot, text, req->msg_type, 1) == 0)
	{
		return -1;
	}
	pdivoom_ai_talk_handle->transaction_seed++;
	if (pdivoom_ai_talk_handle->transaction_seed == 0)
	{
		pdivoom_ai_talk_handle->transaction_seed++;
	}
	slot->active = 1;
	slot->fallback_only = 1;
	slot->token = pdivoom_ai_talk_handle->transaction_seed;
	slot->session_id = req->session_id;
	slot->request_sequence = req->request_sequence;
	slot->clock_id = req->clock_id;
	slot->category_id = req->msg_type;
	strncpy(slot->language, language, sizeof(slot->language) - 1);
	slot->view.generation = slot->token;
	slot->view.msg_type = req->msg_type;
	slot->view.fallback_only = 1;
	*card = slot->view;
	*token = slot->token;
	return 1;
}

int32 divoom_ai_talk_acquire(const divoom_ai_talk_acquire_req_t *req,
	divoom_ai_talk_card_view_t *card, uint32 *token)
{
	divoom_ai_talk_acquire_req_t normalized_req;
	const char *language;
	const char *author_name;
	const char *card_text;
	Pdivoom_ai_talk_category_cache cache;
	divoom_ai_talk_transaction_slot *slot;
	char pixel_msg[DIVOOM_AI_TALK_PIXEL_MSG_MAX_LEN];
	char guide_file_id[DIVOOM_AI_TALK_FILE_ID_MAX_LEN];
	uint32 guide_generation;
	uint32 guide_image_id;
	uint32 guide_revision;
	uint8 use_big_text;
	uint8 greeting_reload_needed;
#if defined(DIVOOM_SIMULATOR)
	uint8 simulator_refill_needed;
#endif
	int i;
	int guide_msg_id;
	int msg_index;
	int invalid_greeting;
	int invalid_greeting_used;
	int persistence_result;

	if (card)
	{
		memset(card, 0, sizeof(*card));
	}
	if (token)
	{
		*token = 0;
	}
	if (req == NULL || card == NULL || token == NULL)
	{
		return -1;
	}
	normalized_req = *req;
	normalized_req.msg_type = divoom_ai_talk_normalize_category_id(req->msg_type);
	req = &normalized_req;
	if (req->session_id == 0 || req->clock_id <= 0 || req->msg_type <= 0 ||
		divoom_ai_talk_ensure_handle() != 0 || divoom_ai_talk_request_is_enabled() == 0)
	{
		return -1;
	}
	language = req->language && req->language[0] ?
		req->language : divoom_ai_talk_get_language();
	author_name = req->msg_type == DIVOOM_AI_TALK_MSG_TYPE_PIXEL_COMMUNITY ?
		divoom_ai_talk_get_pixel_community_author_name() : "";
	divoom_ai_talk_lock();
	cache = divoom_ai_talk_find_cache(req->clock_id, language, req->msg_type);
	divoom_ai_talk_unlock();
	if (cache == NULL)
	{
		(void)divoom_ai_talk_load_cache(req->clock_id, language, req->msg_type);
	}
	greeting_reload_needed = 0;
	if (req->msg_type == DIVOOM_AI_TALK_MSG_TYPE_GREETING &&
		req->restore_msg_id == 0)
	{
		divoom_ai_talk_lock();
		cache = divoom_ai_talk_find_cache(req->clock_id, language, req->msg_type);
		msg_index = cache ? cache->used_num : -1;
		if (cache != NULL && cache->persistence_pending == 0 &&
			msg_index >= 0 && msg_index < cache->msg_num &&
			cache->msg[msg_index] == NULL)
		{
			greeting_reload_needed = 1;
		}
		divoom_ai_talk_unlock();
		if (greeting_reload_needed)
		{
			(void)divoom_ai_talk_reload_greeting_cache(req->clock_id, language);
		}
	}
#if defined(DIVOOM_SIMULATOR)
	simulator_refill_needed = 0;
	divoom_ai_talk_lock();
	cache = divoom_ai_talk_find_cache(req->clock_id, language, req->msg_type);
	msg_index = cache ? cache->used_num : -1;
	if (req->msg_type != DIVOOM_AI_TALK_MSG_TYPE_GREETING &&
		req->restore_msg_id == 0 && cache != NULL &&
		(msg_index < 0 || msg_index >= cache->msg_num || cache->msg[msg_index] == NULL))
	{
		simulator_refill_needed = 1;
	}
	divoom_ai_talk_unlock();
	if (simulator_refill_needed)
	{
		(void)divoom_ai_talk_refill_simulator_cache(req->clock_id, language,
			req->msg_type);
	}
#endif
	divoom_ai_talk_lock();
	cache = divoom_ai_talk_find_cache(req->clock_id, language, req->msg_type);
	if (cache == NULL)
	{
		divoom_ai_talk_unlock();
		if (req->restore_msg_id == 0)
		{
			(void)divoom_ai_talk_request_category(req->clock_id, language,
				req->msg_type, req->request_sequence);
		}
		divoom_ai_talk_lock();
		i = divoom_ai_talk_acquire_default_greeting_locked(req, language, card, token);
		divoom_ai_talk_unlock();
		if (i != 0)
		{
			return i;
		}
		return 0;
	}
	if (cache->persistence_pending)
	{
		/* 新批落盘或UsedNum提交尚未完成，本轮等待而不使用半持久化数据。 */
		divoom_ai_talk_unlock();
		return 0;
	}
	msg_index = cache->used_num;
	if (req->restore_msg_id != 0)
	{
		/* 恢复仅查已有批次的 used 区，不把未显示卡伪装成已消费卡。 */
		for (msg_index = 0; msg_index < cache->msg_num; msg_index++)
		{
			if (msg_index < cache->used_num && cache->msg_id[msg_index] == req->restore_msg_id)
			{
				break;
			}
		}
	}
	if (msg_index < 0 || msg_index >= cache->msg_num || cache->msg[msg_index] == NULL)
	{
		divoom_ai_talk_unlock();
		if (req->restore_msg_id == 0)
		{
			(void)divoom_ai_talk_request_category(req->clock_id, language,
				req->msg_type, req->request_sequence);
		}
		divoom_ai_talk_lock();
		i = divoom_ai_talk_acquire_default_greeting_locked(req, language, card, token);
		divoom_ai_talk_unlock();
		if (i != 0)
		{
			return i;
		}
		return 0;
	}
	if (req->msg_type == DIVOOM_AI_TALK_MSG_TYPE_GUIDE_CARD)
	{
		guide_msg_id = cache->msg_id[msg_index];
		guide_image_id = divoom_ai_talk_build_guide_image_id(req->clock_id,
			guide_msg_id);
		guide_generation = cache->generation;
		guide_revision = cache->revision;
		strncpy(guide_file_id, cache->file_id[msg_index],
			sizeof(guide_file_id) - 1);
		guide_file_id[sizeof(guide_file_id) - 1] = '\0';
		divoom_ai_talk_unlock();
		if (guide_image_id == 0 || guide_file_id[0] == '\0')
		{
			return -1;
		}
		if (divoom_manage_image_check_exist(
			DIVOOM_MANAGE_IMAGE_AI_GUIDE_CARD_IMG, guide_image_id) == 0)
		{
			/* acquire 不等待网络；下载完成后 ready_generation 会唤醒显示重试。 */
			divoom_ai_talk_queue_guide_images(req->clock_id, language);
			return 0;
		}
		divoom_ai_talk_lock();
		cache = divoom_ai_talk_find_cache(req->clock_id, language, req->msg_type);
		if (cache == NULL || cache->persistence_pending ||
			cache->generation != guide_generation || cache->revision != guide_revision ||
			msg_index < 0 || msg_index >= cache->msg_num ||
			cache->msg_id[msg_index] != guide_msg_id ||
			cache->msg[msg_index] == NULL)
		{
			divoom_ai_talk_unlock();
			return 0;
		}
	}
	for (i = 0; i < DIVOOM_AI_TALK_TRANSACTION_SLOT_NUM; i++)
	{
		if (pdivoom_ai_talk_handle->transaction[i].active &&
			pdivoom_ai_talk_handle->transaction[i].clock_id == req->clock_id &&
			pdivoom_ai_talk_handle->transaction[i].category_id == req->msg_type &&
			pdivoom_ai_talk_handle->transaction[i].msg_id == cache->msg_id[msg_index] &&
			strcmp(pdivoom_ai_talk_handle->transaction[i].language, language) == 0)
		{
			divoom_ai_talk_unlock();
			return 0;
		}
	}
	slot = NULL;
	for (i = 0; i < DIVOOM_AI_TALK_TRANSACTION_SLOT_NUM; i++)
	{
		if (pdivoom_ai_talk_handle->transaction[i].active == 0)
		{
			slot = &pdivoom_ai_talk_handle->transaction[i];
			break;
		}
	}
	if (slot == NULL)
	{
		divoom_ai_talk_unlock();
		return 0;
	}
	memset(slot, 0, sizeof(*slot));
	use_big_text = divoom_ai_talk_msg_uses_big_text(req->msg_type);
	if (req->require_big && req->clock_id == DIVOOM_AI_TALK_LEGACY_CLOCK_ID &&
		(req->msg_type == DIVOOM_AI_TALK_MSG_TYPE_ANIMAL_KNOWLEDGE ||
		 req->msg_type == DIVOOM_AI_TALK_MSG_TYPE_NATURE_KNOWLEDGE))
	{
		/* V2 动物/植物文本在知识子表盘投影为正文，不改分类和缓存版本。 */
		use_big_text = 1;
	}
	card_text = cache->msg[msg_index];
	if (req->msg_type == DIVOOM_AI_TALK_MSG_TYPE_PIXEL_COMMUNITY &&
		strstr(card_text, DIVOOM_AI_TALK_PIXEL_AUTHOR_TOKEN))
	{
		if (divoom_ai_talk_format_pixel_community_msg(card_text, author_name,
			pixel_msg, (int)sizeof(pixel_msg)) == 0)
		{
			memset(slot, 0, sizeof(*slot));
			divoom_ai_talk_unlock();
			return -1;
		}
		card_text = pixel_msg;
	}
	if (divoom_ai_talk_fill_card_view(slot, card_text,
		req->msg_type, use_big_text) == 0 ||
		(req->require_big && req->msg_type != DIVOOM_AI_TALK_MSG_TYPE_GUIDE_CARD &&
		 (slot->view.big_text == NULL || slot->view.big_text[0] == '\0')))
	{
		invalid_greeting = req->msg_type == DIVOOM_AI_TALK_MSG_TYPE_GREETING &&
			req->restore_msg_id == 0;
		invalid_greeting_used = 0;
		memset(slot, 0, sizeof(*slot));
		if (invalid_greeting)
		{
			/* 坏问候整批隔离，避免每次重试都卡在同一条非法内容。 */
			invalid_greeting_used = cache->msg_num;
			cache->used_num = invalid_greeting_used;
			cache->persistence_pending = 1;
			divoom_ai_talk_free_cache_messages(cache);
		}
		divoom_ai_talk_unlock();
		if (invalid_greeting)
		{
			persistence_result = divoom_ai_talk_update_greeting_used_file(req->clock_id,
				language, invalid_greeting_used);
			divoom_ai_talk_lock();
			cache = divoom_ai_talk_find_cache(req->clock_id, language,
				req->msg_type);
			if (cache)
			{
				cache->persistence_pending = 0;
				if (persistence_result != 0)
				{
					cache->used_num = 0;
					cache->msg_num = 0;
				}
			}
			divoom_ai_talk_unlock();
			(void)divoom_ai_talk_request_category(req->clock_id, language,
				req->msg_type, req->request_sequence);
			divoom_ai_talk_lock();
			i = divoom_ai_talk_acquire_default_greeting_locked(req, language,
				card, token);
			divoom_ai_talk_unlock();
			return i != 0 ? i : -1;
		}
		return -1;
	}
	pdivoom_ai_talk_handle->transaction_seed++;
	if (pdivoom_ai_talk_handle->transaction_seed == 0)
	{
		pdivoom_ai_talk_handle->transaction_seed++;
	}
	slot->active = 1;
	slot->restore_only = req->restore_msg_id != 0;
	slot->token = pdivoom_ai_talk_handle->transaction_seed;
	slot->session_id = req->session_id;
	slot->request_sequence = req->request_sequence;
	slot->clock_id = req->clock_id;
	slot->category_id = req->msg_type;
	slot->msg_id = cache->msg_id[msg_index];
	strncpy(slot->language, language, sizeof(slot->language) - 1);
	slot->view.generation = slot->token;
	slot->view.revision = cache->revision;
	slot->view.msg_id = slot->msg_id;
	if (req->msg_type == DIVOOM_AI_TALK_MSG_TYPE_GUIDE_CARD)
	{
		slot->view.guide_image_id = divoom_ai_talk_build_guide_image_id(
			req->clock_id, slot->msg_id);
	}
	*card = slot->view;
	*token = slot->token;
	if (req->msg_type == DIVOOM_AI_TALK_MSG_TYPE_GREETING)
	{
		/* 事务已经持有文本副本；当前问候也不需要继续占用RAM。 */
		divoom_free(cache->msg[msg_index]);
		cache->msg[msg_index] = NULL;
	}
	divoom_ai_talk_unlock();
	return 1;
}

uint32 divoom_ai_talk_get_ready_generation(int clock_id, int msg_type,
	uint32 request_sequence)
{
	Pdivoom_ai_talk_category_cache cache;
	const char *language;
	uint32 generation;
	uint32 guide_image_id;
	int normalized_type;

	if (pdivoom_ai_talk_handle == NULL || clock_id <= 0 || msg_type <= 0)
	{
		return 0;
	}
	normalized_type = divoom_ai_talk_normalize_category_id(msg_type);
	language = divoom_ai_talk_get_language();
	generation = 0;
	guide_image_id = 0;
	divoom_ai_talk_lock();
	cache = divoom_ai_talk_find_cache(clock_id, language, normalized_type);
	if (cache && cache->request_pending == 0 && cache->persistence_pending == 0 &&
		cache->used_num >= 0 && cache->used_num < cache->msg_num &&
		(request_sequence == 0 || cache->request_sequence == request_sequence) &&
		(normalized_type == DIVOOM_AI_TALK_MSG_TYPE_GREETING ||
		 cache->msg[cache->used_num] != NULL))
	{
		generation = cache->generation;
		if (normalized_type == DIVOOM_AI_TALK_MSG_TYPE_GUIDE_CARD)
		{
			guide_image_id = divoom_ai_talk_build_guide_image_id(clock_id,
				cache->msg_id[cache->used_num]);
		}
	}
	divoom_ai_talk_unlock();
	if (guide_image_id != 0 && divoom_manage_image_check_exist(
		DIVOOM_MANAGE_IMAGE_AI_GUIDE_CARD_IMG, guide_image_id) == 0)
	{
		return 0;
	}
	return generation;
}

int32 divoom_ai_talk_commit(uint32 session_id, uint32 token)
{
	Pdivoom_ai_talk_category_cache cache;
	divoom_ai_talk_transaction_slot *slot;
	char language[DIVOOM_AI_TALK_LANGUAGE_MAX_LEN];
	int clock_id;
	int category_id;
	int committed;
	int committed_used_num;
	int greeting_commit;
	int persistence_result;
	int request_new_batch;
	int i;
	int slot_index;

	if (pdivoom_ai_talk_handle == NULL || session_id == 0 || token == 0)
	{
		return 0;
	}
	cache = NULL;
	slot = NULL;
	committed = 0;
	committed_used_num = 0;
	greeting_commit = 0;
	persistence_result = -1;
	request_new_batch = 0;
	slot_index = -1;
	divoom_ai_talk_lock();
	for (i = 0; i < DIVOOM_AI_TALK_TRANSACTION_SLOT_NUM; i++)
	{
		if (pdivoom_ai_talk_handle->transaction[i].active &&
			pdivoom_ai_talk_handle->transaction[i].token == token &&
			pdivoom_ai_talk_handle->transaction[i].session_id == session_id)
		{
			slot = &pdivoom_ai_talk_handle->transaction[i];
			slot_index = i;
			break;
		}
	}
	if (slot)
	{
		if (slot->fallback_only)
		{
			memset(slot, 0, sizeof(*slot));
			divoom_ai_talk_unlock();
			return 1;
		}
		cache = divoom_ai_talk_find_cache(slot->clock_id, slot->language,
			slot->category_id);
		if (slot->restore_only)
		{
			if (cache && cache->revision == slot->view.revision)
			{
				for (i = 0; i < cache->used_num && i < cache->msg_num; i++)
				{
					if (cache->msg_id[i] == slot->msg_id)
					{
						committed = 1;
						break;
					}
				}
			}
			memset(slot, 0, sizeof(*slot));
			divoom_ai_talk_unlock();
			return committed;
		}
		clock_id = slot->clock_id;
		category_id = slot->category_id;
		strncpy(language, slot->language, sizeof(language) - 1);
		language[sizeof(language) - 1] = '\0';
		if (cache && cache->persistence_pending == 0 &&
			cache->revision == slot->view.revision && cache->used_num < cache->msg_num &&
			cache->msg_id[cache->used_num] == slot->msg_id)
		{
			committed_used_num = cache->used_num + 1;
			if (category_id == DIVOOM_AI_TALK_MSG_TYPE_GREETING)
			{
				/* 文件游标成功持久化前，RAM游标和事务都保持原状。 */
				cache->persistence_pending = 1;
				greeting_commit = 1;
			}
			else
			{
				cache->used_num = committed_used_num;
				request_new_batch = cache->used_num >= cache->msg_num ? 1 : 0;
				committed = 1;
			}
		}
		if (greeting_commit == 0)
		{
			memset(slot, 0, sizeof(*slot));
		}
	}
	divoom_ai_talk_unlock();
	if (greeting_commit)
	{
		persistence_result = divoom_ai_talk_update_greeting_used_file(clock_id,
			language, committed_used_num);
		divoom_ai_talk_lock();
		cache = divoom_ai_talk_find_cache(clock_id, language, category_id);
		slot = slot_index >= 0 ?
			&pdivoom_ai_talk_handle->transaction[slot_index] : NULL;
		if (persistence_result == 0 && cache && slot && slot->active &&
			slot->token == token && slot->session_id == session_id &&
			cache->revision == slot->view.revision &&
			cache->used_num + 1 == committed_used_num &&
			cache->msg_id[cache->used_num] == slot->msg_id)
		{
			cache->used_num = committed_used_num;
			request_new_batch = cache->used_num >= cache->msg_num ? 1 : 0;
			committed = 1;
		}
		if (cache)
		{
			cache->persistence_pending = 0;
		}
		if (slot && slot->active && slot->token == token &&
			slot->session_id == session_id)
		{
			/* 写盘失败也结束本事务；RAM游标未推进，下一轮会重取同一条。 */
			memset(slot, 0, sizeof(*slot));
		}
		divoom_ai_talk_unlock();
	}
	if (committed == 0)
	{
		return 0;
	}
	if (category_id != DIVOOM_AI_TALK_MSG_TYPE_GREETING)
	{
		(void)divoom_ai_talk_save_cache_by_key(clock_id, language, category_id);
	}
	if (request_new_batch)
	{
		(void)divoom_ai_talk_request_category(clock_id, language, category_id, 0);
	}
	return 1;
}

void divoom_ai_talk_cancel(uint32 session_id, uint32 token)
{
	int i;

	if (pdivoom_ai_talk_handle == NULL || session_id == 0 || token == 0)
	{
		return;
	}
	divoom_ai_talk_lock();
	for (i = 0; i < DIVOOM_AI_TALK_TRANSACTION_SLOT_NUM; i++)
	{
		if (pdivoom_ai_talk_handle->transaction[i].active &&
			pdivoom_ai_talk_handle->transaction[i].session_id == session_id &&
			pdivoom_ai_talk_handle->transaction[i].token == token)
		{
			memset(&pdivoom_ai_talk_handle->transaction[i], 0,
				sizeof(pdivoom_ai_talk_handle->transaction[i]));
			break;
		}
	}
	divoom_ai_talk_unlock();
}

void divoom_ai_talk_cancel_session(uint32 session_id)
{
	int i;

	if (pdivoom_ai_talk_handle == NULL || session_id == 0)
	{
		return;
	}
	divoom_ai_talk_lock();
	for (i = 0; i < DIVOOM_AI_TALK_TRANSACTION_SLOT_NUM; i++)
	{
		if (pdivoom_ai_talk_handle->transaction[i].active &&
			pdivoom_ai_talk_handle->transaction[i].session_id == session_id)
		{
			memset(&pdivoom_ai_talk_handle->transaction[i], 0,
				sizeof(pdivoom_ai_talk_handle->transaction[i]));
		}
	}
	divoom_ai_talk_unlock();
}

#if defined(DIVOOM_SIMULATOR)
int divoom_ai_talk_whitebox_small_text_separator(void)
{
	divoom_ai_talk_transaction_slot slot;
	char pixel_msg[DIVOOM_AI_TALK_PIXEL_MSG_MAX_LEN];

	memset(&slot, 0, sizeof(slot));
	if (divoom_ai_talk_fill_card_view(&slot,
		"big\n<DIVOOM_AI_COMMENT>\nfirst<SEP>second",
		DIVOOM_AI_TALK_MSG_TYPE_PORTRY_SHORT, 1) == 0 ||
		slot.view.big_text == NULL || strcmp(slot.view.big_text, "big") != 0 ||
		slot.view.small_text == NULL || strcmp(slot.view.small_text, "first") != 0 ||
		slot.view.small_text_secondary == NULL ||
		strcmp(slot.view.small_text_secondary, "second") != 0)
	{
		return 0;
	}
	memset(&slot, 0, sizeof(slot));
	if (divoom_ai_talk_fill_card_view(&slot, "single",
		DIVOOM_AI_TALK_MSG_TYPE_NONSENSE, 0) == 0 ||
		slot.view.small_text == NULL || strcmp(slot.view.small_text, "single") != 0 ||
		slot.view.small_text_secondary != NULL)
	{
		return 0;
	}
	if (divoom_ai_talk_build_guide_image_id(DIVOOM_AI_TALK_LEGACY_CLOCK_ID, 123) != 615 ||
		divoom_ai_talk_build_guide_image_id(DIVOOM_AI_TALK_CLOCK_ID_MOM, 123) != 616 ||
		divoom_ai_talk_build_guide_image_id(DIVOOM_AI_TALK_CLOCK_ID_DIVO, 123) != 617 ||
		divoom_ai_talk_build_guide_image_id(DIVOOM_AI_TALK_CLOCK_ID_RICH_GUY, 123) != 618 ||
		divoom_ai_talk_build_guide_image_id(DIVOOM_AI_TALK_CLOCK_ID_PROFESSOR, 123) != 619)
	{
		return 0;
	}
	if (divoom_ai_talk_pixel_author_name_is_valid("PixelArtist") == 0 ||
		divoom_ai_talk_pixel_author_name_is_valid("/userdata/divoom/12/123.bin") != 0 ||
		divoom_ai_talk_pixel_author_name_is_valid("C:\\userdata\\123.bin") != 0)
	{
		return 0;
	}
	if (divoom_ai_talk_format_pixel_community_msg(
		"pixel work by {author}", "/userdata/divoom/12/123.bin",
		pixel_msg, (int)sizeof(pixel_msg)) == 0 ||
		strcmp(pixel_msg, "pixel work by /userdata/divoom/12/123.bin") != 0)
	{
		return 0;
	}
	if (divoom_ai_talk_format_pixel_community_msg(
		"pixel work by {author}", "", pixel_msg,
		(int)sizeof(pixel_msg)) == 0 ||
		strcmp(pixel_msg, "pixel work by ") != 0 ||
		strstr(pixel_msg, DIVOOM_AI_TALK_PIXEL_AUTHOR_TOKEN) != NULL)
	{
		return 0;
	}
	return 1;
}

int divoom_ai_talk_get_probe(divoom_ai_talk_probe_t *probe)
{
	Pdivoom_ai_talk_category_cache cache;
	int i;

	if (probe == NULL)
	{
		return 0;
	}
	memset(probe, 0, sizeof(*probe));
	if (pdivoom_ai_talk_handle == NULL)
	{
		return 1;
	}
	divoom_ai_talk_lock();
	for (i = 0; i < DIVOOM_AI_TALK_TRANSACTION_SLOT_NUM; i++)
	{
		if (pdivoom_ai_talk_handle->transaction[i].active)
		{
			probe->pending_tokens++;
		}
	}
	cache = pdivoom_ai_talk_handle->cache_list;
	while (cache)
	{
		probe->cache_used = (uint16)(probe->cache_used + cache->used_num);
		if (cache->request_pending)
		{
			probe->cache_request_pending = 1;
		}
		cache = cache->next;
	}
	probe->stale_drop_count = pdivoom_ai_talk_handle->stale_drop_count;
	divoom_ai_talk_unlock();
	return 1;
}

int divoom_ai_talk_get_category_probe(int clock_id, const char *language,
	int msg_type, divoom_ai_talk_probe_t *probe)
{
	Pdivoom_ai_talk_category_cache cache;
	const char *target_language;
	int i;

	if (probe == NULL || clock_id <= 0 || msg_type <= 0)
	{
		return 0;
	}
	memset(probe, 0, sizeof(*probe));
	if (pdivoom_ai_talk_handle == NULL)
	{
		return 1;
	}
	target_language = language && language[0] ? language : divoom_ai_talk_get_language();
	divoom_ai_talk_lock();
	for (i = 0; i < DIVOOM_AI_TALK_TRANSACTION_SLOT_NUM; i++)
	{
		if (pdivoom_ai_talk_handle->transaction[i].active &&
			pdivoom_ai_talk_handle->transaction[i].clock_id == clock_id &&
			pdivoom_ai_talk_handle->transaction[i].category_id == msg_type &&
			strcmp(pdivoom_ai_talk_handle->transaction[i].language,
				target_language) == 0)
		{
			probe->pending_tokens++;
		}
	}
	cache = divoom_ai_talk_find_cache(clock_id, target_language, msg_type);
	if (cache)
	{
		probe->cache_used = (uint16)cache->used_num;
		probe->cache_request_pending = cache->request_pending ? 1 : 0;
	}
	probe->stale_drop_count = pdivoom_ai_talk_handle->stale_drop_count;
	divoom_ai_talk_unlock();
	return 1;
}

const char *divoom_ai_talk_whitebox_get_language(void)
{
	return divoom_ai_talk_get_language();
}

int divoom_ai_talk_whitebox_expire_category(int clock_id, const char *language,
	int msg_type)
{
	Pdivoom_ai_talk_category_cache cache;
	const char *target_language;

	if (pdivoom_ai_talk_handle == NULL || clock_id <= 0 || msg_type <= 0)
	{
		return 0;
	}
	target_language = language && language[0] ? language : divoom_ai_talk_get_language();
	divoom_ai_talk_lock();
	cache = divoom_ai_talk_find_cache(clock_id, target_language, msg_type);
	if (cache)
	{
		cache->used_num = cache->msg_num;
		cache->request_pending = 0;
		cache->request_tick = 0;
	}
	divoom_ai_talk_unlock();
	return cache ? 1 : 0;
}

int divoom_ai_talk_whitebox_revoke_card(int clock_id, const char *language,
	int msg_type, int msg_id)
{
	Pdivoom_ai_talk_category_cache cache;
	const char *target_language;
	int i;
	int j;
	int removed;
	uint32 removed_image_id;

	if (pdivoom_ai_talk_handle == NULL || clock_id <= 0 || msg_type <= 0 || msg_id <= 0)
	{
		return 0;
	}
	target_language = language && language[0] ? language : divoom_ai_talk_get_language();
	removed = 0;
	removed_image_id = 0;
	divoom_ai_talk_lock();
	cache = divoom_ai_talk_find_cache(clock_id, target_language, msg_type);
	if (cache)
	{
		for (i = 0; i < cache->msg_num; i++)
		{
			if (cache->msg_id[i] != msg_id)
			{
				continue;
			}
			if (cache->msg[i])
			{
				divoom_system_free(cache->msg[i]);
			}
			if (msg_type == DIVOOM_AI_TALK_MSG_TYPE_GUIDE_CARD)
			{
				removed_image_id = divoom_ai_talk_build_guide_image_id(clock_id,
					cache->msg_id[i]);
			}
			for (j = i; j + 1 < cache->msg_num; j++)
			{
				cache->msg_id[j] = cache->msg_id[j + 1];
				strncpy(cache->file_id[j], cache->file_id[j + 1],
					sizeof(cache->file_id[j]) - 1);
				cache->msg[j] = cache->msg[j + 1];
			}
			cache->msg_num--;
			cache->msg_id[cache->msg_num] = 0;
			cache->file_id[cache->msg_num][0] = '\0';
			cache->msg[cache->msg_num] = NULL;
			if (cache->used_num > i)
			{
				cache->used_num--;
			}
			removed = 1;
			break;
		}
	}
	if (removed)
	{
		for (i = 0; i < DIVOOM_AI_TALK_TRANSACTION_SLOT_NUM; i++)
		{
			if (pdivoom_ai_talk_handle->transaction[i].active &&
				pdivoom_ai_talk_handle->transaction[i].clock_id == clock_id &&
				pdivoom_ai_talk_handle->transaction[i].category_id == msg_type &&
				pdivoom_ai_talk_handle->transaction[i].msg_id == msg_id)
			{
				memset(&pdivoom_ai_talk_handle->transaction[i], 0,
					sizeof(pdivoom_ai_talk_handle->transaction[i]));
			}
		}
	}
	divoom_ai_talk_unlock();
	if (removed_image_id != 0)
	{
		divoom_manage_image_del(DIVOOM_MANAGE_IMAGE_AI_GUIDE_CARD_IMG,
			&removed_image_id, 1, 1);
	}
	return removed;
}
#endif

uint8 divoom_ai_talk_split_portry_short(const char *combined, char *portry_buf,
	int32 buf_len, const char **comment)
{
	const char *separator;
	const char *comment_text;
	int32 portry_len;

	if (combined == NULL || portry_buf == NULL || buf_len <= 1 || comment == NULL)
	{
		return 0;
	}
	separator = strstr(combined, DIVOOM_AI_TALK_PORTRY_SHORT_SEPARATOR);
	if (separator == NULL || separator == combined)
	{
		return 0;
	}
	portry_len = (int32)(separator - combined);
	/* 仅裁掉分隔符前的行尾；内部换行全部保留，正文可以是任意行数。 */
	while (portry_len > 0 &&
		(combined[portry_len - 1] == '\r' || combined[portry_len - 1] == '\n'))
	{
		portry_len--;
	}
	if (portry_len <= 0 || portry_len >= buf_len)
	{
		return 0;
	}
	comment_text = separator + strlen(DIVOOM_AI_TALK_PORTRY_SHORT_SEPARATOR);
	while (*comment_text == '\r' || *comment_text == '\n')
	{
		comment_text++;
	}
	if (*comment_text == '\0' || strstr(comment_text,
		DIVOOM_AI_TALK_PORTRY_SHORT_SEPARATOR) != NULL)
	{
		return 0;
	}

	memcpy(portry_buf, combined, (size_t)portry_len);
	portry_buf[portry_len] = '\0';
	*comment = comment_text;
	return 1;
}

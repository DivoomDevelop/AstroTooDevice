#ifndef _divoom_bailian_character_text_h_
#define _divoom_bailian_character_text_h_

#include "divoom_type.h"

#define DIVOOM_BAILIAN_CHARACTER_TEXT_MAX_OUTPUT_LEN 1024
#define DIVOOM_BAILIAN_CHARACTER_TEXT_MAX_ERROR_LEN  192

/* AI 角色表盘原始表盘 ID，同时也是本模块对外使用的角色 ID。 */
typedef enum _divoom_bailian_character_role_id
{
    DIVOOM_BAILIAN_CHARACTER_ROLE_BUDDY = 255,
    DIVOOM_BAILIAN_CHARACTER_ROLE_MOM = 1721,
    DIVOOM_BAILIAN_CHARACTER_ROLE_DIVO = 1817,
    DIVOOM_BAILIAN_CHARACTER_ROLE_ASTRO_CAT = DIVOOM_BAILIAN_CHARACTER_ROLE_DIVO,
    DIVOOM_BAILIAN_CHARACTER_ROLE_RICH_GUY = 1896,
    DIVOOM_BAILIAN_CHARACTER_ROLE_PROFESSOR = 1991
} divoom_bailian_character_role_id;

typedef enum _divoom_bailian_character_text_status
{
    DIVOOM_BAILIAN_CHARACTER_TEXT_OK = 0,
    DIVOOM_BAILIAN_CHARACTER_TEXT_INVALID_ARGUMENT = -1,
    DIVOOM_BAILIAN_CHARACTER_TEXT_NOT_CONFIGURED = -2,
    DIVOOM_BAILIAN_CHARACTER_TEXT_NO_MEMORY = -3,
    DIVOOM_BAILIAN_CHARACTER_TEXT_QUEUE_FULL = -4,
    DIVOOM_BAILIAN_CHARACTER_TEXT_NETWORK_ERROR = -5,
    DIVOOM_BAILIAN_CHARACTER_TEXT_HTTP_ERROR = -6,
    DIVOOM_BAILIAN_CHARACTER_TEXT_RESPONSE_ERROR = -7,
    DIVOOM_BAILIAN_CHARACTER_TEXT_STOPPING = -8,
    DIVOOM_BAILIAN_CHARACTER_TEXT_DISABLED = -9
} divoom_bailian_character_text_status;

typedef struct _divoom_bailian_character_text_config
{
    /* API Key 仅在内存中使用，禁止写入资源、日志或源码。 */
    const char *api_key;
    /* 使用百炼业务空间专属域名时填写；endpoint 非空时可不填。 */
    const char *workspace_id;
    /* 可选完整 chat/completions 地址，默认按 cn-beijing 业务空间域名生成。 */
    const char *endpoint;
    /* 可选模型名，默认 qwen3.8-max。 */
    const char *model;
} divoom_bailian_character_text_config, *Pdivoom_bailian_character_text_config;

typedef struct _divoom_bailian_character_text_result
{
    uint32 request_id;
    int role_id;
    int status;
    int http_status;
    char language[16];
    char text[DIVOOM_BAILIAN_CHARACTER_TEXT_MAX_OUTPUT_LEN];
    char error[DIVOOM_BAILIAN_CHARACTER_TEXT_MAX_ERROR_LEN];
} divoom_bailian_character_text_result, *Pdivoom_bailian_character_text_result;

/*
 * 回调运行在百炼网络工作线程，回调返回后 result 立即失效。
 * 回调中禁止直接操作 LVGL；UI 使用方应复制结果并投递到 LVGL 线程。
 */
typedef void (*divoom_bailian_character_text_callback)(
    const divoom_bailian_character_text_result *result, void *user_data);

/* 显式配置并启动串行工作线程；重复初始化需先调用 shutdown。 */
int divoom_bailian_character_text_init(
    const divoom_bailian_character_text_config *config);

/* 从以上编译宏初始化；保留函数名以兼容既有调用。 */
int divoom_bailian_character_text_init_from_env(void);

/*
 * 只入队，不在调用线程执行 HTTP。语言自动读取当前系统语言；max_chars
 * 是返回正文包含标点在内的 Unicode 字符上限，max_bytes 是 UTF-8 正文
 * 字节上限（不含结尾零字符）。
 * callback 可为空；request_id 可为空。
 */
int divoom_bailian_character_text_request(int role_id, const char *input_text,
    uint16 max_chars, uint16 max_bytes,
    divoom_bailian_character_text_callback callback, void *user_data,
    uint32 *request_id);

/* 等待当前网络请求结束并停止线程；禁止在结果回调内部调用。 */
void divoom_bailian_character_text_shutdown(void);

/* 返回角色英文名/性格摘要或当前系统语言 BCP-47 标签，返回值无需释放。 */
const char *divoom_bailian_character_text_get_role_name(int role_id);
const char *divoom_bailian_character_text_get_role_summary(int role_id);
const char *divoom_bailian_character_text_get_system_language(void);

#endif

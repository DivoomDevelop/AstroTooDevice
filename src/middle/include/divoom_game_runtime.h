#ifndef _divoom_game_runtime_h_
#define _divoom_game_runtime_h_

#include "divoom_type.h"

struct _lv_obj_t;
typedef struct _lv_obj_t lv_obj_t;

#define DIVOOM_GAMES_ROOT_USER          "/userdata/divoom_games"
#define DIVOOM_GAMES_NATIVE_SUB         "native"
#define DIVOOM_GAMES_SAVES_SUB          "saves"
#define DIVOOM_GAMES_SEED_ROOT          "/usr/share/divoom_app/games_seed"

#define DIVOOM_GAME_MAX_ENTRIES         64
#define DIVOOM_GAME_TITLE_MAX           64
#define DIVOOM_GAME_ID_MAX              32
#define DIVOOM_GAME_PATH_MAX            256
#define DIVOOM_GAME_ENTRY_FILE_MAX      64

/* Lua VM 堆上限：Astro Pet V8/V9（v8_data + aw_sdk + render/states/main + EXPLORE 7 星球
 * 数据/贴图引用）已逼近原 256KB；上调到 512KB 留足余量，避免加载 states.lua 时 LUA_ERRMEM。*/
#define DIVOOM_GAME_LUA_HEAP_MAX        (512 * 1024)
#define DIVOOM_GAME_FRAME_MS_DEFAULT    33
#define DIVOOM_GAME_LUA_FRAME_BUDGET_MS 8

typedef enum
{
    DIVOOM_GAME_TYPE_NATIVE = 0,
} divoom_game_type_t;

typedef enum
{
    DIVOOM_GAME_SOURCE_OFFICIAL = 0,
    DIVOOM_GAME_SOURCE_USER,
} divoom_game_source_t;

typedef struct
{
    char id[DIVOOM_GAME_ID_MAX];
    char title[DIVOOM_GAME_TITLE_MAX];
    char title_key[32];
    char dir_rel[128];
    char dir_abs[DIVOOM_GAME_PATH_MAX];
    char entry[DIVOOM_GAME_ENTRY_FILE_MAX];
    divoom_game_type_t type;
    divoom_game_source_t source;
    uint8 fps;
} divoom_game_entry_t;

typedef struct
{
    divoom_game_entry_t entries[DIVOOM_GAME_MAX_ENTRIES];
    int count;
} divoom_game_catalog_t;

typedef enum
{
    DIVOOM_GAME_NATIVE_OK = 0,
    DIVOOM_GAME_NATIVE_ERR,
    DIVOOM_GAME_NATIVE_QUIT,
} divoom_game_native_result_t;

typedef void (*divoom_game_native_quit_cb)(void *user_data);

void divoom_game_runtime_ensure_dirs(void);
int divoom_game_runtime_loader_refresh(divoom_game_catalog_t *catalog);
int divoom_game_runtime_loader_remove_entry(const divoom_game_entry_t *entry);
void divoom_game_runtime_low_mem_guard_set(uint8 active);
uint8 divoom_game_runtime_low_mem_guard_is_active(void);

const char *divoom_game_runtime_type_label(divoom_game_type_t type);

int divoom_game_native_lua_start(const divoom_game_entry_t *entry, lv_obj_t *parent,
    divoom_game_native_quit_cb quit_cb, void *quit_user_data);
void divoom_game_native_lua_stop(void);
void divoom_game_native_lua_pause(int paused);
void divoom_game_native_lua_on_key(uint8_t key_code, uint8_t pressed);
void divoom_game_native_lua_key_hold(uint8_t key_code);
/* 把「返回/ESC」按键派发给 Lua on_key("b")，并返回 Lua 是否已自行处理：
 * 返回 1 表示 Lua 调用了 game.consume_back()（逐级返回上一层）或 game.quit()（顶层退出），
 *        此时由 Lua/计时器负责后续，C 层不要再强制 stop_running；
 * 返回 0 表示 Lua 未处理，调用方应自行结束游戏（保持 snake 等旧行为）。 */
int divoom_game_native_lua_dispatch_back(uint8_t key_code);
int divoom_game_native_lua_is_running(void);
const char *divoom_game_native_lua_get_id(void);
const char *divoom_game_native_lua_get_state(void);

int divoom_game_pack_install_from_zip(const char *zip_path);
int divoom_game_pack_install_request_url(const char *url, const char *dest_zip_path);
int divoom_game_pack_install_poll(void);
int divoom_game_pack_install_pending(void);
int divoom_game_pack_install_from_local_manifest(void);

#endif

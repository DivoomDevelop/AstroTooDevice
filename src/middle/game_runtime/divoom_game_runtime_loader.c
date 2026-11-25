#include "divoom.h"
#include "divoom_game_runtime.h"
#include "divoom_menu_control.h"
#include "divoom_port_fs.h"
#include <stdlib.h>
#include <string.h>

static volatile uint8 g_divoom_game_runtime_low_mem_guard;

static void divoom_game_runtime_free_read_buf(char *buf)
{
    if (buf != NULL)
    {
        divoom_system_free(buf);
    }
}

static int divoom_game_runtime_type_is_native(const char *type_str)
{
    if (type_str == NULL || type_str[0] == 0 || strcmp(type_str, "native") == 0)
    {
        return 1;
    }
    return 0;
}

static divoom_game_source_t divoom_game_runtime_parse_source(const char *src_str)
{
    if (src_str && strcmp(src_str, "user") == 0)
    {
        return DIVOOM_GAME_SOURCE_USER;
    }
    return DIVOOM_GAME_SOURCE_OFFICIAL;
}

static int divoom_game_runtime_entry_exists(const divoom_game_catalog_t *catalog, const char *id)
{
    int i;

    for (i = 0; i < catalog->count; i++)
    {
        if (strcmp(catalog->entries[i].id, id) == 0)
        {
            return 1;
        }
    }
    return 0;
}

static int divoom_game_runtime_add_entry(divoom_game_catalog_t *catalog, const divoom_game_entry_t *entry)
{
    if (catalog->count >= DIVOOM_GAME_MAX_ENTRIES)
    {
        return -1;
    }
    if (divoom_game_runtime_entry_exists(catalog, entry->id))
    {
        return 0;
    }
    catalog->entries[catalog->count] = *entry;
    catalog->count++;
    return 0;
}

static void divoom_game_runtime_parse_game_json(const char *game_json_path,
    divoom_game_catalog_t *catalog)
{
    char *buf;
    divoom_cJSON *root;
    divoom_cJSON *item;
    divoom_game_entry_t entry;
    const char *type_str;
    const char *src_str;
    const char *title;
    const char *title_key;
    const char *entry_file;
    const char *id_str;

    buf = divoom_port_file_read_all(game_json_path, NULL);
    if (buf == NULL)
    {
        return;
    }
    root = divoom_cJSON_Parse(buf);
    divoom_game_runtime_free_read_buf(buf);
    if (root == NULL)
    {
        return;
    }

    memset(&entry, 0, sizeof(entry));
    id_str = divoom_cJSON_GetObjectItem(root, "id") ?
        divoom_cJSON_GetObjectItem(root, "id")->valuestring : NULL;
    if (id_str == NULL || id_str[0] == 0)
    {
        divoom_cJSON_Delete(root);
        return;
    }
    lv_snprintf(entry.id, sizeof(entry.id), "%s", id_str);

    type_str = divoom_cJSON_GetObjectItem(root, "type") ?
        divoom_cJSON_GetObjectItem(root, "type")->valuestring : "native";
    if (!divoom_game_runtime_type_is_native(type_str))
    {
        divoom_cJSON_Delete(root);
        return;
    }
    entry.type = DIVOOM_GAME_TYPE_NATIVE;

    src_str = divoom_cJSON_GetObjectItem(root, "source") ?
        divoom_cJSON_GetObjectItem(root, "source")->valuestring : "official";
    entry.source = divoom_game_runtime_parse_source(src_str);

    title_key = divoom_cJSON_GetObjectItem(root, "title_key") ?
        divoom_cJSON_GetObjectItem(root, "title_key")->valuestring : NULL;
    if (title_key)
    {
        lv_snprintf(entry.title_key, sizeof(entry.title_key), "%s", title_key);
        lv_snprintf(entry.title, sizeof(entry.title), "%s",
            divoom_menu_control_get_disp_str(title_key));
    }
    title = divoom_cJSON_GetObjectItem(root, "title") ?
        divoom_cJSON_GetObjectItem(root, "title")->valuestring : NULL;
    if (entry.title[0] == 0 && title)
    {
        lv_snprintf(entry.title, sizeof(entry.title), "%s", title);
    }
    if (entry.title[0] == 0)
    {
        lv_snprintf(entry.title, sizeof(entry.title), "%s", entry.id);
    }

    entry_file = divoom_cJSON_GetObjectItem(root, "entry") ?
        divoom_cJSON_GetObjectItem(root, "entry")->valuestring : "main.lua";
    lv_snprintf(entry.entry, sizeof(entry.entry), "%s", entry_file);

    item = divoom_cJSON_GetObjectItem(root, "fps");
    if (item)
    {
        entry.fps = (uint8)divoom_cJSON_GetObjectInt(root, "fps");
    }
    if (entry.fps == 0)
    {
        entry.fps = 30;
    }

    lv_snprintf(entry.dir_abs, sizeof(entry.dir_abs), "%s", game_json_path);
    {
        char *slash;

        slash = strrchr(entry.dir_abs, '/');
        if (slash)
        {
            *slash = 0;
        }
    }
    {
        const char *native_tag;

        native_tag = strstr(entry.dir_abs, DIVOOM_GAMES_NATIVE_SUB);
        if (native_tag)
        {
            lv_snprintf(entry.dir_rel, sizeof(entry.dir_rel), "%s", native_tag);
        }
    }

    divoom_game_runtime_add_entry(catalog, &entry);
    divoom_cJSON_Delete(root);
}

static void divoom_game_runtime_load_catalog_file(const char *catalog_path,
    const char *catalog_root, divoom_game_catalog_t *catalog)
{
    char *buf;
    divoom_cJSON *root;
    divoom_cJSON *games;
    int i;
    int n;

    buf = divoom_port_file_read_all(catalog_path, NULL);
    if (buf == NULL)
    {
        return;
    }
    root = divoom_cJSON_Parse(buf);
    divoom_game_runtime_free_read_buf(buf);
    if (root == NULL)
    {
        return;
    }

    games = divoom_cJSON_GetObjectItem(root, "games");
    if (games == NULL || (games->type & 0xFF) != divoom_cJSON_Array)
    {
        divoom_cJSON_Delete(root);
        return;
    }

    n = divoom_cJSON_GetArraySize(games);
    for (i = 0; i < n; i++)
    {
        divoom_game_entry_t entry;
        divoom_cJSON *g;
        const char *id_str;
        const char *type_str;
        const char *dir_str;
        const char *src_str;
        char game_json[DIVOOM_GAME_PATH_MAX];

        g = divoom_cJSON_GetArrayItem(games, i);
        if (g == NULL)
        {
            continue;
        }

        memset(&entry, 0, sizeof(entry));
        id_str = divoom_cJSON_GetObjectItem(g, "id") ?
            divoom_cJSON_GetObjectItem(g, "id")->valuestring : NULL;
        if (id_str == NULL)
        {
            continue;
        }
        lv_snprintf(entry.id, sizeof(entry.id), "%s", id_str);

        type_str = divoom_cJSON_GetObjectItem(g, "type") ?
            divoom_cJSON_GetObjectItem(g, "type")->valuestring : "native";
        if (!divoom_game_runtime_type_is_native(type_str))
        {
            continue;
        }
        entry.type = DIVOOM_GAME_TYPE_NATIVE;

        src_str = divoom_cJSON_GetObjectItem(g, "source") ?
            divoom_cJSON_GetObjectItem(g, "source")->valuestring : "official";
        entry.source = divoom_game_runtime_parse_source(src_str);

        dir_str = divoom_cJSON_GetObjectItem(g, "dir") ?
            divoom_cJSON_GetObjectItem(g, "dir")->valuestring : NULL;
        if (dir_str)
        {
            lv_snprintf(entry.dir_rel, sizeof(entry.dir_rel), "%s", dir_str);
            lv_snprintf(entry.dir_abs, sizeof(entry.dir_abs), "%s/%s",
                catalog_root, dir_str);
        }
        else
        {
            lv_snprintf(entry.dir_rel, sizeof(entry.dir_rel), "%s/%s",
                DIVOOM_GAMES_NATIVE_SUB, entry.id);
            lv_snprintf(entry.dir_abs, sizeof(entry.dir_abs), "%s/%s/%s",
                catalog_root, DIVOOM_GAMES_NATIVE_SUB, entry.id);
        }

        lv_snprintf(game_json, sizeof(game_json), "%s/game.json", entry.dir_abs);
        if (divoom_port_file_exists(game_json))
        {
            divoom_game_runtime_parse_game_json(game_json, catalog);
            continue;
        }

        lv_snprintf(entry.title, sizeof(entry.title), "%s", entry.id);
        lv_snprintf(entry.entry, sizeof(entry.entry), "main.lua");
        entry.fps = 30;
        divoom_game_runtime_add_entry(catalog, &entry);
    }

    divoom_cJSON_Delete(root);
}

void divoom_game_runtime_ensure_dirs(void)
{
    char path[DIVOOM_GAME_PATH_MAX];

    divoom_port_mkdir_p(DIVOOM_GAMES_ROOT_USER);
    lv_snprintf(path, sizeof(path), "%s/%s", DIVOOM_GAMES_ROOT_USER, DIVOOM_GAMES_NATIVE_SUB);
    divoom_port_mkdir_p(path);
    lv_snprintf(path, sizeof(path), "%s/icons", DIVOOM_GAMES_ROOT_USER);
    divoom_port_mkdir_p(path);
    lv_snprintf(path, sizeof(path), "%s/%s", DIVOOM_GAMES_ROOT_USER, DIVOOM_GAMES_SAVES_SUB);
    divoom_port_mkdir_p(path);
}

int divoom_game_runtime_loader_refresh(divoom_game_catalog_t *catalog)
{
    if (catalog == NULL)
    {
        return -1;
    }

    memset(catalog, 0, sizeof(*catalog));
    divoom_game_runtime_ensure_dirs();
    divoom_game_runtime_load_catalog_file(DIVOOM_GAMES_SEED_ROOT "/catalog.json",
        DIVOOM_GAMES_SEED_ROOT, catalog);

    divoom_print("[game_runtime] catalog count=%d", catalog->count);
    return catalog->count;
}

void divoom_game_runtime_low_mem_guard_set(uint8 active)
{
    active = active ? 1 : 0;
    if (g_divoom_game_runtime_low_mem_guard == active)
    {
        return;
    }

    /*
     * Games 列表页开始就进入低内存保护态，让 HTTP/MQTT/图片下载先停下来，
     * 避免真正启动 Lua 游戏时后台任务继续申请大块临时内存。
     */
    g_divoom_game_runtime_low_mem_guard = active;
    divoom_print("[game_runtime] low_mem_guard %s\n", active ? "on" : "off");
}

uint8 divoom_game_runtime_low_mem_guard_is_active(void)
{
    return g_divoom_game_runtime_low_mem_guard ? 1 : 0;
}

const char *divoom_game_runtime_type_label(divoom_game_type_t type)
{
    (void)type;
    return "Native";
}

int divoom_game_runtime_loader_remove_entry(const divoom_game_entry_t *entry)
{
    (void)entry;
    return -1;
}

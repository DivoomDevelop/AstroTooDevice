#include "divoom.h"
#include "lvgl.h"
#include "divoom_game_runtime.h"
#include "divoom_menu_control.h"
#include "divoom_astrootoo_ui_base.h"
#include "divoom_main.h"

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include "cJSON.h"
#include "divoom_rk_mpp.h"
#include "divoom_aw_pic_decode.h"
#include "divoom_local_music_ctrl.h"
#include "divoom_port_fs.h"
#include "divoom_pixel_format.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define DIVOOM_GAME_FB_W        DIVOOM_SCREEN_WIDTH
#define DIVOOM_GAME_FB_H        DIVOOM_SCREEN_HIGH
#define DIVOOM_GAME_LOGIC_W     240
#define DIVOOM_GAME_LOGIC_H     240
#define DIVOOM_GAME_LOGIC_SCALE 2
#define DIVOOM_GAME_IMG_CACHE_MAX   96
#define DIVOOM_GAME_IMG_CACHE_SWEEP_FRAMES  15
#define DIVOOM_GAME_IMG_CACHE_KEEP_FRAMES   30
#define DIVOOM_GAME_IMG_CACHE_NEG_KEEP_FRAMES 300
#define DIVOOM_GAME_IMG_CACHE_RETRY_FRAMES  30
#define DIVOOM_GAME_IMG_CACHE_BYTES_MAX     (384U * 1024U)
#define DIVOOM_GAME_LVGL_IMAGE_CACHE_GAME_BYTES (256U * 1024U)
#define DIVOOM_GAME_IMG_DECODE_RESERVE_BYTES (1536U * 1024U)
#define DIVOOM_GAME_IMG_DECODE_SMALL_RESERVE_BYTES (0U)
#define DIVOOM_GAME_IMG_CACHE_LARGE_BYTES   (400U * 1024U)
#define DIVOOM_GAME_JPEG_DECODE_MIN_LARGEST_FREE_KB 1536L
#define DIVOOM_GAME_JPEG_DECODE_POOL_MIN_LARGEST_FREE_KB 1024L
#define DIVOOM_GAME_FRONT_COLOR_FORMAT      LV_COLOR_FORMAT_RGB565
#define DIVOOM_GAME_FB_BYTES                (DIVOOM_GAME_FB_W * DIVOOM_GAME_FB_H * DIVOOM_APP_RGB565_BYTES_PER_PX)
#define DIVOOM_GAME_BASE_RGB565_POOL_BYTES  DIVOOM_GAME_FB_BYTES
#ifndef DIVOOM_GAME_NATIVE_LUA_DOUBLE_BUFFER
/* 默认使用双缓冲：Lua 绘制在后缓冲完成后再整帧提交给 LVGL，减少整屏刷新闪烁。
 * 如需节省一块 480x480 RGB565 缓冲内存，可在编译参数中置 0 回到单缓冲。 */
#define DIVOOM_GAME_NATIVE_LUA_DOUBLE_BUFFER 1
#endif
#define DIVOOM_GAME_TEXT_CMD_MAX            96
#define DIVOOM_GAME_TEXT_CMD_TEXT_MAX       128
#define DIVOOM_GAME_LABEL_ID_MAX            32
#define DIVOOM_GAME_TEXT_LABEL_MAX          DIVOOM_GAME_TEXT_CMD_MAX
#define DIVOOM_GAME_LABEL_MAX               (DIVOOM_GAME_TEXT_LABEL_MAX + 32)
/* 游戏文字必须使用系统字体，由 LVGL label 在帧末作为 overlay 绘制。
 * 真机字体 bitmap 可能是平台私有/压缩格式，不能直接按 A8 字形手动混入帧缓冲。 */
#define DIVOOM_GAME_MANUAL_FONT_DRAW        0
#ifndef DIVOOM_GAME_NATIVE_LUA_AUDIO_ENABLE
#if DIVOOM_LVGL9_IS_SIM
#define DIVOOM_GAME_NATIVE_LUA_AUDIO_ENABLE 0
#else
#define DIVOOM_GAME_NATIVE_LUA_AUDIO_ENABLE 0
#endif
#endif
#define DIVOOM_GAME_SAVE_PATH_MAX   256
#define DIVOOM_GAME_TEXT_DEFAULT_SIZE 8  /* draw_text 默认逻辑字号（fb 像素 = *scale） */
/* 按住超时：sim 按住时每 DIVOOM_KEY_SEND_PRESSING_TIME(100ms) 发一次重复事件刷新，
 * 取 250ms 作为安全阈值——按住期间持续判定为按下，松开/轻点后最多 250ms 自动复位，
 * 避免轻点(SHORT 无 UP)或漏收释放事件导致 key_down 永久卡住 */
#define DIVOOM_GAME_KEY_HOLD_TIMEOUT_MS 250
#define DIVOOM_GAME_SHARED_COUNTERS_FILE  DIVOOM_GAMES_ROOT_USER "/shared_counters.json"
#define DIVOOM_GAME_SHARED_DAY_FIELD      "day_id"
#define DIVOOM_GAME_LANG_BASE_PATH        DIVOOM_GAMES_SEED_ROOT "/lang"

typedef struct _divoom_game_lang_item_t
{
    char *key;
    char *value;
    struct _divoom_game_lang_item_t *next;
} divoom_game_lang_item_t;

typedef struct
{
    int lang_id;
    const char *dir_path;
} divoom_game_lang_dir_t;

static const divoom_game_lang_dir_t g_divoom_game_lang_dirs[] =
{
    {DIVOOM_LVGL_MENU_LANG_ENGLISH,    "en"},
    {DIVOOM_LVGL_MENU_LANG_SC,         "sc"},
    {DIVOOM_LVGL_MENU_LANG_TC,         "tc"},
    {DIVOOM_LVGL_MENU_LANG_JP,         "ja"},
    {DIVOOM_LVGL_MENU_LANG_Germany,    "de"},
    {DIVOOM_LVGL_MENU_LANG_Thai,       "th"},
    {DIVOOM_LVGL_MENU_LANG_Russian,    "ru"},
    {DIVOOM_LVGL_MENU_LANG_Korean,     "ko"},
    {DIVOOM_LVGL_MENU_LANG_Vietnam,    "vi"},
    {DIVOOM_LVGL_MENU_LANG_French,     "fr"},
    {DIVOOM_LVGL_MENU_LANG_Turkey,     "tr"},
    {DIVOOM_LVGL_MENU_LANG_Spanish,    "es"},
    {DIVOOM_LVGL_MENU_LANG_Portuguese, "pt"},
    {DIVOOM_LVGL_MENU_LANG_Indonesian, "in"},
    {DIVOOM_LVGL_MENU_LANG_Italy,      "it"},
    {DIVOOM_LVGL_MENU_LANG_Ukraine,    "uk"},
    {DIVOOM_LVGL_MENU_LANG_Swedish,    "sw"},
    {DIVOOM_LVGL_MENU_LANG_Danish,     "da"},
    {DIVOOM_LVGL_MENU_LANG_Poland,     "pl"},
};

typedef enum
{
    DIVOOM_GAME_KEY_UP = 0,
    DIVOOM_GAME_KEY_DOWN,
    DIVOOM_GAME_KEY_LEFT,
    DIVOOM_GAME_KEY_RIGHT,
    DIVOOM_GAME_KEY_A,
    DIVOOM_GAME_KEY_B,
    DIVOOM_GAME_KEY_START,
    DIVOOM_GAME_KEY_COUNT
} divoom_game_key_slot_t;

typedef struct
{
    char path[128];
    uint8 *data;
    int w;
    int h;
    uint32 byte_size;
    uint32 last_used_frame;
    uint8 format;
    uint8 valid;
    uint8 external_data;
    uint8 retryable;
    uint32 retry_after_frame;
} divoom_game_img_cache_entry_t;

typedef struct
{
    char text[DIVOOM_GAME_TEXT_CMD_TEXT_MAX];
    int x;
    int y;
    int max_w;
    lv_color_t color;
    const lv_font_t *font;
} divoom_game_text_cmd_t;

typedef struct
{
    char id[DIVOOM_GAME_LABEL_ID_MAX];
    lv_obj_t *obj;
} divoom_game_label_slot_t;

typedef enum
{
    DIVOOM_GAME_IMG_CACHE_FMT_NONE = 0,
    DIVOOM_GAME_IMG_CACHE_FMT_RGB565,
    DIVOOM_GAME_IMG_CACHE_FMT_RGB565A8,
} divoom_game_img_cache_format_t;

typedef struct
{
    char path[128];
    void *handle;
    uint8 *frame_buf;
    int w;
    int h;
    uint32 next_frame_tick;
    uint8 valid;
} divoom_game_webp_anim_slot_t;

typedef struct
{
    lua_State *L;
    lv_obj_t *root;
    lv_obj_t *canvas;
    lv_obj_t *draw_canvas;
    uint8 *fb;      /* Lua 绘制使用的 RGB565 后缓冲；单缓冲时也作为 canvas 显示缓冲 */
    uint8 *canvas_fb; /* 双缓冲时 LVGL canvas 绑定的 RGB565 前缓冲，帧末由 fb 整帧提交 */
    uint8 *base_rgb565_buf; /* 页底图 JPEG 输出复用池，避免运行中反复申请大块 RGB565 */
    uint32 base_rgb565_buf_size;
    lv_timer_t *timer;
    divoom_game_entry_t entry;
    divoom_game_native_quit_cb quit_cb;
    void *quit_user_data;
    uint8 key_down[DIVOOM_GAME_KEY_COUNT];
    uint8 key_pressed[DIVOOM_GAME_KEY_COUNT];
    uint32 key_last_ms[DIVOOM_GAME_KEY_COUNT]; /* 最近一次按下/按住事件的时刻，用于超时自动松开 */
    uint8 running;
    uint8 paused;
    uint8 quit_request;
    uint8 quit_notified;
    uint8 back_consumed; /* 本次 ESC 派发中，Lua 是否调用 game.consume_back() 自行返回上一层 */
    uint8 defer_invalidate; /* on_frame 绘制期间先攒脏标记，帧末统一刷新，避免文字/图片半帧抖动 */
    uint8 frame_dirty;
    uint8 use_double_buffer;
    uint8 lvgl_cache_limited;
    uint32 lua_heap_used;
    uint32 frame_ms;
    uint32 frame_seq;
    uint32 img_cache_owned_bytes;
    uint32 img_cache_external_bytes;
    uint32 img_cache_last_sweep;
    int lang_flag;
    divoom_game_lang_item_t *lang_items;
    divoom_game_img_cache_entry_t img_cache[DIVOOM_GAME_IMG_CACHE_MAX];
    divoom_game_webp_anim_slot_t webp_anim;
    divoom_game_text_cmd_t text_cmds[DIVOOM_GAME_TEXT_CMD_MAX];
    divoom_game_label_slot_t labels[DIVOOM_GAME_LABEL_MAX];
    uint8 text_cmd_count;
    uint8 text_label_active_count;
#if DIVOOM_GAME_MANUAL_FONT_DRAW
    uint8 *font_bitmap_buf;
    uint32 font_bitmap_buf_size;
#endif
} divoom_game_native_lua_ctrl_t;

/* The LVGL thread owns the active instance and every timer/canvas that uses it. */
static divoom_game_native_lua_ctrl_t *g_native_lua_ctrl;

static int divoom_game_native_lua_load_script(lua_State *L, const char *script_path);
static void divoom_game_fb_blend_px(int x, int y, lv_color_t color, uint8 a);
static void divoom_game_native_lua_flush_dirty(void);
static uint16 divoom_game_rgb565_load_le(const uint8 *src);
static void divoom_game_rgb565_to_rgb(uint16 pixel, uint8 *r, uint8 *g, uint8 *b);
static uint8 divoom_game_native_lua_is_base_path(const char *rel);
static divoom_game_label_slot_t *divoom_game_native_lua_label_find(const char *id);
static divoom_game_label_slot_t *divoom_game_native_lua_label_get_or_create(const char *id);

static void divoom_game_native_lua_log_mem_stage(const char *stage)
{
    (void)stage;
}

static double divoom_game_lua_safe_number(double value)
{
    /*
     * cJSON 打印 NaN/Inf 时在部分真机库上会生成空数字，形成 `"key":,`
     * 这种非法 JSON。Native Lua 存档统一把非有限值归零，保证可再次解析。
     */
    if (value != value || value > 1.0e300 || value < -1.0e300)
    {
        return 0;
    }
    return value;
}

static void divoom_game_lua_log_json_error(const char *tag, const char *json_str,
    const char *error_ptr, int json_len)
{
    int pos;
    int start;
    int copy_len;
    char snippet[96];
    int i;

    if (tag == NULL)
    {
        tag = "json";
    }
    if (json_str == NULL || error_ptr == NULL || error_ptr < json_str)
    {
        divoom_print("[native_lua] %s json error pos=-1 len=%d\n", tag, json_len);
        return;
    }

    pos = (int)(error_ptr - json_str);
    start = pos - 40;
    if (start < 0)
    {
        start = 0;
    }
    copy_len = json_len - start;
    if (copy_len > 80)
    {
        copy_len = 80;
    }
    if (copy_len < 0)
    {
        copy_len = 0;
    }
    for (i = 0; i < copy_len && i < (int)sizeof(snippet) - 1; i++)
    {
        char ch;

        ch = json_str[start + i];
        if (ch == '\r' || ch == '\n' || ch == '\t')
        {
            ch = ' ';
        }
        snippet[i] = ch;
    }
    snippet[i] = '\0';
    divoom_print("[native_lua] %s json error pos=%d len=%d near=%s\n",
        tag, pos, json_len, snippet);
}

static char *divoom_game_native_lua_read_file(const char *path, int *file_len)
{
    if (file_len != NULL)
    {
        *file_len = 0;
    }
    if (path == NULL)
    {
        return NULL;
    }
    return divoom_port_file_read_all(path, file_len);
}

static void divoom_game_native_lua_free_file_buf(char *buf)
{
    if (buf != NULL)
    {
        divoom_system_free(buf);
    }
}

static uint8 divoom_game_native_lua_get_file_size(const char *path, int *file_len)
{
    void *file;
    long size;

    if (file_len != NULL)
    {
        *file_len = 0;
    }
    if (path == NULL || file_len == NULL)
    {
        return 0;
    }
    file = divoom_port_file_open(path, "rb");
    if (file == NULL)
    {
        return 0;
    }
    if (divoom_port_file_seek(file, 0, DIVOOM_PORT_SEEK_END) != 0)
    {
        divoom_port_file_close(file);
        return 0;
    }
    size = divoom_port_file_tell(file);
    divoom_port_file_close(file);
    if (size <= 0 || size > INT32_MAX)
    {
        return 0;
    }
    *file_len = (int)size;
    return 1;
}

static uint8 divoom_game_native_lua_read_file_to_buf(const char *path,
    uint8 *buf, uint32 buf_size, int *file_len)
{
    void *file;
    long size;
    size_t read_len;

    if (file_len != NULL)
    {
        *file_len = 0;
    }
    if (path == NULL || buf == NULL || file_len == NULL)
    {
        return 0;
    }
    file = divoom_port_file_open(path, "rb");
    if (file == NULL)
    {
        return 0;
    }
    if (divoom_port_file_seek(file, 0, DIVOOM_PORT_SEEK_END) != 0)
    {
        divoom_port_file_close(file);
        return 0;
    }
    size = divoom_port_file_tell(file);
    if (size <= 0 || size > INT32_MAX || (uint32)size > buf_size)
    {
        divoom_port_file_close(file);
        return 0;
    }
    if (divoom_port_file_seek(file, 0, DIVOOM_PORT_SEEK_SET) != 0)
    {
        divoom_port_file_close(file);
        return 0;
    }
    read_len = divoom_port_file_read(buf, 1, (size_t)size, file);
    divoom_port_file_close(file);
    if (read_len != (size_t)size)
    {
        return 0;
    }
    *file_len = (int)size;
    return 1;
}

static int divoom_game_native_lua_write_file(const char *path, const uint8 *buf, uint32 len)
{
    void *fp;
    size_t written;

    if (path == NULL || (buf == NULL && len > 0U))
    {
        return -1;
    }
    fp = divoom_port_file_open(path, "wb");
    if (fp == NULL)
    {
        divoom_print("[native_lua] open write failed: %s\n", path);
        return -1;
    }
    written = 0;
    if (len > 0U)
    {
        written = divoom_port_file_write(buf, 1, (size_t)len, fp);
    }
    if (divoom_port_file_close(fp) != 0)
    {
        divoom_print("[native_lua] close write failed: %s\n", path);
        return -1;
    }
    if (written != (size_t)len)
    {
        divoom_print("[native_lua] write failed: %s %u/%u\n", path, (uint32)written, len);
        return -1;
    }
    divoom_port_sync();
    return 0;
}

static void divoom_game_native_lua_clear_text_cmds(void)
{
    g_native_lua_ctrl->text_cmd_count = 0;
}

static void divoom_game_native_lua_draw_text_cmds(void)
{
    uint8 i;
    uint8 label_count;
    char label_id[DIVOOM_GAME_LABEL_ID_MAX];

    label_count = 0;
    for (i = 0; i < g_native_lua_ctrl->text_cmd_count; i++)
    {
        divoom_game_text_cmd_t *cmd;
        divoom_game_label_slot_t *slot;
        lv_coord_t max_w;

        cmd = &g_native_lua_ctrl->text_cmds[i];
        if (cmd->text[0] == 0 || cmd->font == NULL)
        {
            continue;
        }
        if (label_count >= DIVOOM_GAME_TEXT_LABEL_MAX)
        {
            break;
        }
        lv_snprintf(label_id, sizeof(label_id), "__txt_%03u", label_count);
        slot = divoom_game_native_lua_label_get_or_create(label_id);
        if (slot == NULL || slot->obj == NULL)
        {
            continue;
        }
        max_w = cmd->max_w;
        if (max_w <= 0)
        {
            max_w = (lv_coord_t)(DIVOOM_GAME_FB_W - cmd->x);
        }
        if (max_w <= 0)
        {
            max_w = DIVOOM_GAME_FB_W;
        }
        /*
         * 普通 draw_text 也使用 LVGL label 承载。这样系统字体始终在
         * LVGL 对象层绘制，不再写入游戏 canvas，规避真机上 canvas 局部
         * 刷新时字形被擦出横线/闪烁的问题。
         */
        lv_obj_set_pos(slot->obj, cmd->x, cmd->y);
        lv_obj_set_width(slot->obj, max_w);
        lv_obj_set_style_text_color(slot->obj, cmd->color, 0);
        lv_obj_set_style_text_font(slot->obj, cmd->font, 0);
        lv_label_set_text(slot->obj, cmd->text);
        lv_obj_clear_flag(slot->obj, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(slot->obj);
        label_count++;
    }
    for (i = label_count; i < g_native_lua_ctrl->text_label_active_count; i++)
    {
        divoom_game_label_slot_t *slot;

        lv_snprintf(label_id, sizeof(label_id), "__txt_%03u", i);
        slot = divoom_game_native_lua_label_find(label_id);
        if (slot != NULL && slot->obj != NULL)
        {
            lv_obj_add_flag(slot->obj, LV_OBJ_FLAG_HIDDEN);
        }
    }
    g_native_lua_ctrl->text_label_active_count = label_count;
}

static uint8 divoom_game_native_lua_queue_text(lv_coord_t x, lv_coord_t y,
    lv_coord_t max_w, lv_draw_label_dsc_t *draw_dsc, const char *txt)
{
    divoom_game_text_cmd_t *cmd;
    uint32 len;

    if (draw_dsc == NULL || draw_dsc->font == NULL || txt == NULL || max_w <= 0)
    {
        return 0;
    }
    if (g_native_lua_ctrl->text_cmd_count >= DIVOOM_GAME_TEXT_CMD_MAX)
    {
        divoom_print("[native_lua] text queue full\n");
        return 0;
    }

    len = (uint32)strlen(txt);
    if (len >= DIVOOM_GAME_TEXT_CMD_TEXT_MAX)
    {
        len = DIVOOM_GAME_TEXT_CMD_TEXT_MAX - 1U;
    }

    cmd = &g_native_lua_ctrl->text_cmds[g_native_lua_ctrl->text_cmd_count++];
    memset(cmd, 0, sizeof(*cmd));
    memcpy(cmd->text, txt, len);
    cmd->text[len] = 0;
    cmd->x = x;
    cmd->y = y;
    cmd->max_w = max_w;
    cmd->color = draw_dsc->color;
    cmd->font = draw_dsc->font;
    return 1;
}

static void divoom_game_native_lua_mark_dirty(void)
{
    g_native_lua_ctrl->frame_dirty = 1;
    if (!g_native_lua_ctrl->defer_invalidate && g_native_lua_ctrl->canvas != NULL)
    {
        divoom_game_native_lua_flush_dirty();
    }
}

static void divoom_game_native_lua_flush_dirty(void)
{
    if ((g_native_lua_ctrl->frame_dirty || g_native_lua_ctrl->text_cmd_count > 0) &&
        g_native_lua_ctrl->canvas != NULL)
    {
        /*
         * 双缓冲模式下 Lua 图形原语只写后缓冲 fb；帧末整帧提交到
         * LVGL canvas 绑定的前缓冲，避免正在扫描显示时看到半帧绘制过程。
         * 单缓冲 fallback 时 canvas_fb == fb，保持原来的只 invalidate 行为。
         * Lua 状态页通常只在 dirty 帧绘制一次。无新画面也无新文字时必须保留
         * 上一帧 label；否则 timer 下一拍会把 TITLE/PAUSE 文案全部隐藏，表现为闪一下。
         */
        if (g_native_lua_ctrl->frame_dirty && g_native_lua_ctrl->use_double_buffer &&
            g_native_lua_ctrl->canvas_fb != NULL && g_native_lua_ctrl->canvas_fb != g_native_lua_ctrl->fb)
        {
            memcpy(g_native_lua_ctrl->canvas_fb, g_native_lua_ctrl->fb,
                (size_t)DIVOOM_GAME_FB_BYTES);
        }
        divoom_game_native_lua_draw_text_cmds();
        lv_obj_invalidate(g_native_lua_ctrl->canvas);
    }
    divoom_game_native_lua_clear_text_cmds();
    g_native_lua_ctrl->frame_dirty = 0;
}

static void divoom_game_native_lua_canvas_draw_text(lv_obj_t *canvas, lv_coord_t x,
    lv_coord_t y, lv_coord_t max_w, lv_draw_label_dsc_t *draw_dsc, const char *txt)
{
    if (canvas == NULL || draw_dsc == NULL || txt == NULL || max_w <= 0)
    {
        return;
    }
    if (!divoom_game_native_lua_queue_text(x, y, max_w, draw_dsc, txt))
    {
        return;
    }
    divoom_game_native_lua_mark_dirty();
}

static void divoom_game_native_lua_label_clear_all(void)
{
    int i;

    for (i = 0; i < DIVOOM_GAME_LABEL_MAX; i++)
    {
        if (g_native_lua_ctrl->labels[i].obj != NULL)
        {
            lv_obj_del(g_native_lua_ctrl->labels[i].obj);
            g_native_lua_ctrl->labels[i].obj = NULL;
        }
        g_native_lua_ctrl->labels[i].id[0] = 0;
    }
    g_native_lua_ctrl->text_label_active_count = 0;
}

static divoom_game_label_slot_t *divoom_game_native_lua_label_find(const char *id)
{
    int i;

    if (id == NULL || id[0] == 0)
    {
        return NULL;
    }
    for (i = 0; i < DIVOOM_GAME_LABEL_MAX; i++)
    {
        if (g_native_lua_ctrl->labels[i].obj != NULL &&
            strcmp(g_native_lua_ctrl->labels[i].id, id) == 0)
        {
            return &g_native_lua_ctrl->labels[i];
        }
    }
    return NULL;
}

static divoom_game_label_slot_t *divoom_game_native_lua_label_get_or_create(const char *id)
{
    divoom_game_label_slot_t *slot;
    int i;

    slot = divoom_game_native_lua_label_find(id);
    if (slot != NULL)
    {
        return slot;
    }
    if (g_native_lua_ctrl->root == NULL || id == NULL || id[0] == 0)
    {
        return NULL;
    }
    for (i = 0; i < DIVOOM_GAME_LABEL_MAX; i++)
    {
        if (g_native_lua_ctrl->labels[i].obj == NULL)
        {
            slot = &g_native_lua_ctrl->labels[i];
            memset(slot, 0, sizeof(*slot));
            strncpy(slot->id, id, sizeof(slot->id) - 1);
            /*
             * overlay label 是少量稳定文字专用层：背景仍由 Lua canvas 绘制，
             * label 只负责系统字体显示，避免动画局部清屏擦到文字。
             */
            slot->obj = lv_label_create(g_native_lua_ctrl->root);
            if (slot->obj == NULL)
            {
                slot->id[0] = 0;
                return NULL;
            }
            lv_label_set_long_mode(slot->obj, LV_LABEL_LONG_CLIP);
            lv_obj_set_style_bg_opa(slot->obj, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(slot->obj, 0, 0);
            lv_obj_set_style_pad_all(slot->obj, 0, 0);
            lv_obj_clear_flag(slot->obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_clear_flag(slot->obj, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_move_foreground(slot->obj);
            return slot;
        }
    }
    divoom_print("[native_lua] label pool full\n");
    return NULL;
}

#if DIVOOM_GAME_MANUAL_FONT_DRAW
static uint32_t divoom_game_native_lua_utf8_next(const char *txt, uint32 *ofs)
{
    const uint8 *s;
    uint32 i;
    uint32_t code;

    if (txt == NULL)
    {
        return 0;
    }
    i = (ofs != NULL) ? *ofs : 0;
    s = (const uint8 *)&txt[i];
    if (s[0] == 0)
    {
        return 0;
    }

    if (s[0] < 0x80)
    {
        code = s[0];
        i += 1;
    }
    else if ((s[0] & 0xE0) == 0xC0 && s[1] != 0)
    {
        code = ((uint32_t)(s[0] & 0x1F) << 6) |
            (uint32_t)(s[1] & 0x3F);
        i += 2;
    }
    else if ((s[0] & 0xF0) == 0xE0 && s[1] != 0 && s[2] != 0)
    {
        code = ((uint32_t)(s[0] & 0x0F) << 12) |
            ((uint32_t)(s[1] & 0x3F) << 6) |
            (uint32_t)(s[2] & 0x3F);
        i += 3;
    }
    else if ((s[0] & 0xF8) == 0xF0 && s[1] != 0 && s[2] != 0 && s[3] != 0)
    {
        code = ((uint32_t)(s[0] & 0x07) << 18) |
            ((uint32_t)(s[1] & 0x3F) << 12) |
            ((uint32_t)(s[2] & 0x3F) << 6) |
            (uint32_t)(s[3] & 0x3F);
        i += 4;
    }
    else
    {
        code = s[0];
        i += 1;
    }

    if (ofs != NULL)
    {
        *ofs = i;
    }
    return code;
}

static uint8 *divoom_game_native_lua_font_buf_ensure(uint32 need_size)
{
    uint8 *new_buf;

    if (need_size == 0)
    {
        return NULL;
    }
    if (g_native_lua_ctrl->font_bitmap_buf != NULL &&
        g_native_lua_ctrl->font_bitmap_buf_size >= need_size)
    {
        return g_native_lua_ctrl->font_bitmap_buf;
    }

    new_buf = (uint8 *)divoom_system_malloc(need_size);
    if (new_buf == NULL)
    {
        return NULL;
    }
    if (g_native_lua_ctrl->font_bitmap_buf != NULL)
    {
        divoom_system_free(g_native_lua_ctrl->font_bitmap_buf);
    }
    g_native_lua_ctrl->font_bitmap_buf = new_buf;
    g_native_lua_ctrl->font_bitmap_buf_size = need_size;
    return g_native_lua_ctrl->font_bitmap_buf;
}

static const uint8 *divoom_game_native_lua_get_glyph_bitmap(const lv_font_t *font,
    lv_font_glyph_dsc_t *glyph_dsc, uint32_t letter, uint8 **bitmap_buf, uint32 *stride)
{
    uint32 bitmap_stride;
    uint32 bitmap_size;

    if (font == NULL || glyph_dsc == NULL || bitmap_buf == NULL || stride == NULL)
    {
        return NULL;
    }

#if DIVOOM_LVGL9_IS_SIM
    bitmap_stride = (uint32)glyph_dsc->box_w;
    bitmap_size = bitmap_stride * (uint32)glyph_dsc->box_h;
    *bitmap_buf = divoom_game_native_lua_font_buf_ensure(bitmap_size);
    if (*bitmap_buf == NULL)
    {
        return NULL;
    }
    *stride = bitmap_stride;
    return lv_font_get_glyph_bitmap(glyph_dsc->resolved_font, letter, *bitmap_buf);
#else
    lv_draw_buf_t draw_buf;

    if (glyph_dsc->format == LV_FONT_GLYPH_FORMAT_IMAGE ||
        glyph_dsc->format == LV_FONT_GLYPH_FORMAT_VECTOR ||
        glyph_dsc->format == LV_FONT_GLYPH_FORMAT_SVG ||
        glyph_dsc->format == LV_FONT_GLYPH_FORMAT_CUSTOM)
    {
        return NULL;
    }

    bitmap_stride = lv_draw_buf_width_to_stride(glyph_dsc->box_w, LV_COLOR_FORMAT_A8);
    bitmap_size = bitmap_stride * (uint32)glyph_dsc->box_h;
    *bitmap_buf = divoom_game_native_lua_font_buf_ensure(bitmap_size);
    if (*bitmap_buf == NULL)
    {
        return NULL;
    }
    memset(&draw_buf, 0, sizeof(draw_buf));
    draw_buf.header.magic = LV_IMAGE_HEADER_MAGIC;
    draw_buf.header.cf = LV_COLOR_FORMAT_A8;
    draw_buf.header.w = glyph_dsc->box_w;
    draw_buf.header.h = glyph_dsc->box_h;
    draw_buf.header.stride = bitmap_stride;
    draw_buf.data_size = bitmap_size;
    draw_buf.data = *bitmap_buf;
    draw_buf.unaligned_data = *bitmap_buf;
    draw_buf.handlers = lv_draw_buf_get_font_handlers();
    *stride = bitmap_stride;
    return (const uint8 *)lv_font_get_glyph_bitmap(glyph_dsc, &draw_buf);
#endif
}

static uint8 divoom_game_native_lua_draw_glyph_to_fb(const lv_font_t *font,
    uint32_t letter, uint32_t letter_next, int *pen_x, int base_y, lv_color_t color)
{
    lv_font_glyph_dsc_t glyph_dsc;
    const uint8_t *bitmap;
    uint8 *bitmap_buf;
    uint32 bitmap_size;
    uint32 bitmap_stride;
    int row;
    int col;
    int letter_x;
    int letter_y;

    if (font == NULL || pen_x == NULL)
    {
        return 0;
    }
    if (!lv_font_get_glyph_dsc(font, &glyph_dsc, letter, letter_next))
    {
        return 0;
    }
    if (glyph_dsc.adv_w > 0)
    {
        /* adv_w 即使空格也要推进光标。 */
        *pen_x += glyph_dsc.adv_w;
    }
    if (glyph_dsc.box_w == 0 || glyph_dsc.box_h == 0)
    {
        return 1;
    }
#if DIVOOM_LVGL9_IS_SIM
    if (glyph_dsc.bpp == LV_IMGFONT_BPP)
    {
        return 0;
    }
#endif

    bitmap = divoom_game_native_lua_get_glyph_bitmap(font, &glyph_dsc, letter,
        &bitmap_buf, &bitmap_stride);
    if (bitmap == NULL)
    {
        return 0;
    }
    bitmap_size = bitmap_stride * (uint32)glyph_dsc.box_h;
    (void)bitmap_size;

    letter_x = (*pen_x - glyph_dsc.adv_w) + glyph_dsc.ofs_x;
    letter_y = base_y + (font->line_height - font->base_line) -
        glyph_dsc.box_h - glyph_dsc.ofs_y;
    for (row = 0; row < glyph_dsc.box_h; row++)
    {
        for (col = 0; col < glyph_dsc.box_w; col++)
        {
            uint8 a;

            a = bitmap[(uint32)row * bitmap_stride + (uint32)col];
            if (a == 0)
            {
                continue;
            }
            divoom_game_fb_blend_px(letter_x + col, letter_y + row, color, a);
        }
    }
#if !DIVOOM_LVGL9_IS_SIM
    lv_font_glyph_release_draw_data(&glyph_dsc);
#endif
    return 1;
}

static uint8 divoom_game_native_lua_draw_text_to_fb(const char *text, int x, int y,
    int max_w, lv_color_t color, const lv_font_t *font)
{
    uint32 ofs;
    int pen_x;
    int line_y;

    if (text == NULL || font == NULL || max_w <= 0)
    {
        return 0;
    }

    ofs = 0;
    pen_x = x;
    line_y = y;
    while (text[ofs] != '\0')
    {
        uint32 old_ofs;
        uint32 next_ofs;
        uint32_t letter;
        uint32_t letter_next;

        old_ofs = ofs;
        letter = divoom_game_native_lua_utf8_next(text, &ofs);
        if (letter == '\0')
        {
            break;
        }
        if (letter == '\n')
        {
            pen_x = x;
            line_y += font->line_height;
            continue;
        }
        next_ofs = ofs;
        letter_next = (text[next_ofs] != '\0') ?
            divoom_game_native_lua_utf8_next(&text[next_ofs], NULL) : 0;
        if (pen_x - x >= max_w)
        {
            break;
        }
        if (!divoom_game_native_lua_draw_glyph_to_fb(font, letter, letter_next,
                &pen_x, line_y, color))
        {
            ofs = old_ofs;
            return 0;
        }
    }
    divoom_game_native_lua_mark_dirty();
    return 1;
}
#endif

static void *divoom_game_lua_alloc(void *ud, void *ptr, size_t osize, size_t nsize)
{
    divoom_game_native_lua_ctrl_t *ctrl;
    void *np;
    size_t old_size;

    ctrl = (divoom_game_native_lua_ctrl_t *)ud;
    old_size = osize;

    if (nsize == 0)
    {
        if (ptr != NULL)
        {
            if (old_size > 0 && ctrl->lua_heap_used >= (uint32)old_size)
            {
                ctrl->lua_heap_used -= (uint32)old_size;
            }
            divoom_system_free(ptr);
        }
        return NULL;
    }

    if (ptr == NULL)
    {
        if (ctrl->lua_heap_used + nsize > DIVOOM_GAME_LUA_HEAP_MAX)
        {
            return NULL;
        }
    }
    else if (nsize > old_size && old_size > 0)
    {
        if (ctrl->lua_heap_used + (nsize - old_size) > DIVOOM_GAME_LUA_HEAP_MAX)
        {
            return NULL;
        }
    }

    np = divoom_system_remalloc(ptr, (uint32)nsize);
    if (np == NULL)
    {
        return NULL;
    }

    if (ptr == NULL)
    {
        ctrl->lua_heap_used += (uint32)nsize;
    }
    else if (old_size > 0)
    {
        if (nsize > old_size)
        {
            ctrl->lua_heap_used += (uint32)(nsize - old_size);
        }
        else if (nsize < old_size)
        {
            ctrl->lua_heap_used -= (uint32)(old_size - nsize);
        }
    }
    return np;
}

static char *divoom_game_lang_copy_range(const char *src, int len)
{
    char *dst;

    if (src == NULL || len < 0)
    {
        return NULL;
    }
    dst = (char *)divoom_system_malloc((uint32)(len + 1));
    if (dst == NULL)
    {
        return NULL;
    }
    memcpy(dst, src, (size_t)len);
    dst[len] = 0;
    return dst;
}

static void divoom_game_lang_apply_newline(char *value)
{
    char *p;

    if (value == NULL)
    {
        return;
    }
    p = strstr(value, "\\n");
    while (p)
    {
        p[0] = '\r';
        p[1] = '\n';
        p += 2;
        p = strstr(p, "\\n");
    }
}

static void divoom_game_lang_decode_entities(char *value)
{
    char *src;
    char *dst;

    if (value == NULL)
    {
        return;
    }
    src = value;
    dst = value;
    while (*src)
    {
        if (strncmp(src, "&amp;", 5) == 0)
        {
            *dst++ = '&';
            src += 5;
        }
        else if (strncmp(src, "&lt;", 4) == 0)
        {
            *dst++ = '<';
            src += 4;
        }
        else if (strncmp(src, "&gt;", 4) == 0)
        {
            *dst++ = '>';
            src += 4;
        }
        else if (strncmp(src, "&quot;", 6) == 0)
        {
            *dst++ = '"';
            src += 6;
        }
        else if (strncmp(src, "&apos;", 6) == 0)
        {
            *dst++ = '\'';
            src += 6;
        }
        else
        {
            *dst++ = *src++;
        }
    }
    *dst = 0;
}

static void divoom_game_lang_free(void)
{
    divoom_game_lang_item_t *item;

    item = g_native_lua_ctrl->lang_items;
    while (item)
    {
        divoom_game_lang_item_t *next;

        next = item->next;
        if (item->key)
        {
            divoom_system_free(item->key);
        }
        if (item->value)
        {
            divoom_system_free(item->value);
        }
        divoom_system_free(item);
        item = next;
    }
    g_native_lua_ctrl->lang_items = NULL;
    g_native_lua_ctrl->lang_flag = -1;
}

static divoom_game_lang_item_t *divoom_game_lang_find(const char *key, int key_len)
{
    divoom_game_lang_item_t *item;

    item = g_native_lua_ctrl->lang_items;
    while (item)
    {
        if (item->key && (int)strlen(item->key) == key_len &&
            memcmp(item->key, key, (size_t)key_len) == 0)
        {
            return item;
        }
        item = item->next;
    }
    return NULL;
}

static void divoom_game_lang_set(const char *key, int key_len, const char *value, int value_len)
{
    divoom_game_lang_item_t *item;
    char *value_copy;

    if (key == NULL || value == NULL || key_len <= 0 || value_len < 0)
    {
        return;
    }
    value_copy = divoom_game_lang_copy_range(value, value_len);
    if (value_copy == NULL)
    {
        return;
    }
    divoom_game_lang_decode_entities(value_copy);
    divoom_game_lang_apply_newline(value_copy);

    item = divoom_game_lang_find(key, key_len);
    if (item)
    {
        if (item->value)
        {
            divoom_system_free(item->value);
        }
        item->value = value_copy;
        return;
    }

    item = (divoom_game_lang_item_t *)divoom_system_malloc(sizeof(divoom_game_lang_item_t));
    if (item == NULL)
    {
        divoom_system_free(value_copy);
        return;
    }
    memset(item, 0, sizeof(*item));
    item->key = divoom_game_lang_copy_range(key, key_len);
    if (item->key == NULL)
    {
        divoom_system_free(value_copy);
        divoom_system_free(item);
        return;
    }
    item->value = value_copy;
    item->next = g_native_lua_ctrl->lang_items;
    g_native_lua_ctrl->lang_items = item;
}

static int divoom_game_lang_parse(char *raw_info)
{
    const char *name_tag = "<string name=\"";
    const char *name_end_tag = "\">";
    const char *value_end_tag = "</string>";
    char *start_ptr;
    int count;

    if (raw_info == NULL)
    {
        return 0;
    }
    count = 0;
    start_ptr = raw_info;
    while (start_ptr)
    {
        char *key_start;
        char *key_end;
        char *value_start;
        char *value_end;

        key_start = strstr(start_ptr, name_tag);
        if (key_start == NULL)
        {
            break;
        }
        key_start += strlen(name_tag);
        key_end = strstr(key_start, name_end_tag);
        if (key_end == NULL)
        {
            break;
        }
        value_start = key_end + strlen(name_end_tag);
        value_end = strstr(value_start, value_end_tag);
        if (value_end == NULL)
        {
            break;
        }
        divoom_game_lang_set(key_start, (int)(key_end - key_start),
            value_start, (int)(value_end - value_start));
        count++;
        start_ptr = value_end + strlen(value_end_tag);
    }
    return count;
}

static int divoom_game_lang_load_file(const char *file_name)
{
    char *lang_file_info;
    int count;

    lang_file_info = divoom_game_native_lua_read_file(file_name, NULL);
    if (lang_file_info == NULL)
    {
        divoom_print("[native_lua] game lang missing: %s\n", file_name);
        return 0;
    }
    count = divoom_game_lang_parse(lang_file_info);
    divoom_game_native_lua_free_file_buf(lang_file_info);
    divoom_print("[native_lua] game lang loaded: %s (%d)\n", file_name, count);
    return count;
}

static const char *divoom_game_lang_dir_from_id(int lang)
{
    int i;

    for (i = 0; i < (int)(sizeof(g_divoom_game_lang_dirs) / sizeof(g_divoom_game_lang_dirs[0])); i++)
    {
        if (g_divoom_game_lang_dirs[i].lang_id == lang)
        {
            return g_divoom_game_lang_dirs[i].dir_path;
        }
    }
    return "en";
}

static int divoom_game_lang_load_current(void)
{
    char file_name[DIVOOM_GAME_PATH_MAX];
    const char *dir;
    int lang;
    int count;

    divoom_game_lang_free();
    lang = divoom_main_set_lang_flag(-1);
    dir = divoom_game_lang_dir_from_id(lang);
    g_native_lua_ctrl->lang_flag = lang;

    lv_snprintf(file_name, sizeof(file_name), "%s/strings.xml", DIVOOM_GAME_LANG_BASE_PATH);
    count = divoom_game_lang_load_file(file_name);

    lv_snprintf(file_name, sizeof(file_name), "%s/%s/strings.xml", DIVOOM_GAME_LANG_BASE_PATH, dir);
    count += divoom_game_lang_load_file(file_name);
    return count;
}

static const char *divoom_game_lang_get_value(const char *key)
{
    divoom_game_lang_item_t *item;

    if (key == NULL)
    {
        return NULL;
    }
    item = divoom_game_lang_find(key, (int)strlen(key));
    if (item && item->value && item->value[0])
    {
        return item->value;
    }
    return NULL;
}

static lv_color_t divoom_game_parse_color(lua_State *L, int idx)
{
    size_t len;
    const char *s;
    uint32 rgb;
    uint8 r;
    uint8 g;
    uint8 b;

    if (lua_type(L, idx) == LUA_TNUMBER)
    {
        rgb = (uint32)lua_tointeger(L, idx);
    }
    else
    {
        s = luaL_checkstring(L, idx);
        len = strlen(s);
        if (len >= 7 && s[0] == '#')
        {
            rgb = (uint32)strtoul(s + 1, NULL, 16);
        }
        else
        {
            rgb = 0;
        }
    }
    r = (uint8)((rgb >> 16) & 0xFF);
    g = (uint8)((rgb >> 8) & 0xFF);
    b = (uint8)(rgb & 0xFF);
    return lv_color_make(r, g, b);
}

static void divoom_game_fb_fill_rect(int x, int y, int w, int h, lv_color_t color)
{
    int row;
    int col;
    int x2;
    int y2;
    uint16 fill;

    if (g_native_lua_ctrl->fb == NULL)
    {
        return;
    }
    if (w <= 0 || h <= 0)
    {
        return;
    }
    x2 = x + w;
    y2 = y + h;
    if (x < 0)
    {
        x = 0;
    }
    if (y < 0)
    {
        y = 0;
    }
    if (x2 > DIVOOM_GAME_FB_W)
    {
        x2 = DIVOOM_GAME_FB_W;
    }
    if (y2 > DIVOOM_GAME_FB_H)
    {
        y2 = DIVOOM_GAME_FB_H;
    }
    fill = divoom_rgb888_to_rgb565(color.red, color.green, color.blue);
    for (row = y; row < y2; row++)
    {
        uint8 *line;

        line = g_native_lua_ctrl->fb + (uint32)row * DIVOOM_GAME_FB_W *
            DIVOOM_APP_RGB565_BYTES_PER_PX;
        for (col = x; col < x2; col++)
        {
            divoom_store_rgb565_le(line + (uint32)col * DIVOOM_APP_RGB565_BYTES_PER_PX,
                fill);
        }
    }
}

static void divoom_game_fb_draw_rect(int x, int y, int w, int h, lv_color_t color)
{
    divoom_game_fb_fill_rect(x, y, w, 1, color);
    divoom_game_fb_fill_rect(x, y + h - 1, w, 1, color);
    divoom_game_fb_fill_rect(x, y, 1, h, color);
    divoom_game_fb_fill_rect(x + w - 1, y, 1, h, color);
}

/* 单像素 alpha 混合（0~255），越界自动裁剪 */
static void divoom_game_fb_blend_px(int x, int y, lv_color_t color, uint8 a)
{
    uint8 *dst;
    uint16 dst565;
    uint16 out565;
    uint8 dr;
    uint8 dg;
    uint8 db;
    uint32 inv_a;
    uint8 out_r;
    uint8 out_g;
    uint8 out_b;

    if (g_native_lua_ctrl->fb == NULL)
    {
        return;
    }
    if (x < 0 || y < 0 || x >= DIVOOM_GAME_FB_W || y >= DIVOOM_GAME_FB_H)
    {
        return;
    }
    if (a == 0)
    {
        return;
    }
    dst = g_native_lua_ctrl->fb + ((uint32)y * DIVOOM_GAME_FB_W + (uint32)x) *
        DIVOOM_APP_RGB565_BYTES_PER_PX;
    if (a >= 255)
    {
        out565 = divoom_rgb888_to_rgb565(color.red, color.green, color.blue);
    }
    else
    {
        inv_a = (uint32)(255 - a);
        dst565 = divoom_game_rgb565_load_le(dst);
        divoom_game_rgb565_to_rgb(dst565, &dr, &dg, &db);
        out_r = (uint8)(((uint32)color.red * a + (uint32)dr * inv_a + 127U) / 255U);
        out_g = (uint8)(((uint32)color.green * a + (uint32)dg * inv_a + 127U) / 255U);
        out_b = (uint8)(((uint32)color.blue * a + (uint32)db * inv_a + 127U) / 255U);
        out565 = divoom_rgb888_to_rgb565(out_r, out_g, out_b);
    }
    divoom_store_rgb565_le(dst, out565);
}

/* 半透明实心矩形 */
static void divoom_game_fb_fill_rect_a(int x, int y, int w, int h, lv_color_t color, uint8 a)
{
    int row;
    int col;

    if (a >= 255)
    {
        divoom_game_fb_fill_rect(x, y, w, h, color);
        return;
    }
    if (w <= 0 || h <= 0 || a == 0)
    {
        return;
    }
    for (row = 0; row < h; row++)
    {
        for (col = 0; col < w; col++)
        {
            divoom_game_fb_blend_px(x + col, y + row, color, a);
        }
    }
}

/* 圆角内某像素是否在圆角裁掉的角落外（用于实心/描边） */
static int divoom_game_round_clip(int col, int row, int w, int h, int r)
{
    int dx;
    int dy;

    if (r <= 0)
    {
        return 0;
    }
    /* 左上 */
    if (col < r && row < r)
    {
        dx = r - col;
        dy = r - row;
        return (dx * dx + dy * dy) > (r * r);
    }
    /* 右上 */
    if (col >= w - r && row < r)
    {
        dx = col - (w - r - 1);
        dy = r - row;
        return (dx * dx + dy * dy) > (r * r);
    }
    /* 左下 */
    if (col < r && row >= h - r)
    {
        dx = r - col;
        dy = row - (h - r - 1);
        return (dx * dx + dy * dy) > (r * r);
    }
    /* 右下 */
    if (col >= w - r && row >= h - r)
    {
        dx = col - (w - r - 1);
        dy = row - (h - r - 1);
        return (dx * dx + dy * dy) > (r * r);
    }
    return 0;
}

/* 圆角实心矩形（带 alpha） */
static void divoom_game_fb_fill_round_rect(int x, int y, int w, int h, int r, lv_color_t color, uint8 a)
{
    int row;
    int col;

    if (w <= 0 || h <= 0 || a == 0)
    {
        return;
    }
    if (r * 2 > w)
    {
        r = w / 2;
    }
    if (r * 2 > h)
    {
        r = h / 2;
    }
    for (row = 0; row < h; row++)
    {
        for (col = 0; col < w; col++)
        {
            if (divoom_game_round_clip(col, row, w, h, r))
            {
                continue;
            }
            divoom_game_fb_blend_px(x + col, y + row, color, a);
        }
    }
}

/* 圆角描边矩形（线宽 t） */
static void divoom_game_fb_draw_round_rect(int x, int y, int w, int h, int r, int t, lv_color_t color)
{
    int row;
    int col;

    if (w <= 0 || h <= 0)
    {
        return;
    }
    if (t < 1)
    {
        t = 1;
    }
    if (r * 2 > w)
    {
        r = w / 2;
    }
    if (r * 2 > h)
    {
        r = h / 2;
    }
    for (row = 0; row < h; row++)
    {
        for (col = 0; col < w; col++)
        {
            int edge;

            if (divoom_game_round_clip(col, row, w, h, r))
            {
                continue;
            }
            edge = 0;
            if (col < t || col >= w - t || row < t || row >= h - t)
            {
                edge = 1;
            }
            else if (r > 0)
            {
                /* 圆角带内边：内缩 t 后若被裁掉则属于描边 */
                if (divoom_game_round_clip(col - t, row - t, w - 2 * t, h - 2 * t, (r > t) ? (r - t) : 0))
                {
                    edge = 1;
                }
            }
            if (edge)
            {
                divoom_game_fb_blend_px(x + col, y + row, color, 255);
            }
        }
    }
}

/* 线性渐变矩形：dir=0 竖向(c1 顶->c2 底)，dir=1 横向(c1 左->c2 右) */
static void divoom_game_fb_fill_grad_rect(int x, int y, int w, int h, lv_color_t c1, lv_color_t c2, int dir)
{
    int row;
    int col;
    int span;

    if (w <= 0 || h <= 0)
    {
        return;
    }
    span = (dir == 1) ? w : h;
    if (span <= 1)
    {
        divoom_game_fb_fill_rect(x, y, w, h, c1);
        return;
    }
    for (row = 0; row < h; row++)
    {
        for (col = 0; col < w; col++)
        {
            uint8 mix;
            int t;

            t = (dir == 1) ? col : row;
            /* mix=255 取 c1，0 取 c2；这里 t=0 取 c1 */
            mix = (uint8)(255 - (t * 255 / (span - 1)));
            divoom_game_fb_blend_px(x + col, y + row, lv_color_mix(c1, c2, mix), 255);
        }
    }
}

static int divoom_game_lua_clear(lua_State *L)
{
    lv_color_t c;
    int a;

    c = divoom_game_parse_color(L, 1);
    a = (int)luaL_optnumber(L, 2, 255); /* 可选第 2 参：alpha 0~255，允许透明上层清屏 */
    if (a < 0)
    {
        a = 0;
    }
    if (a > 255)
    {
        a = 255;
    }
    if (a >= 255)
    {
        divoom_game_fb_fill_rect(0, 0, DIVOOM_GAME_FB_W, DIVOOM_GAME_FB_H, c);
    }
    else
    {
        divoom_game_fb_fill_rect_a(0, 0, DIVOOM_GAME_FB_W, DIVOOM_GAME_FB_H, c,
            (uint8)a);
    }
    divoom_game_native_lua_mark_dirty();
    return 0;
}

static int divoom_game_lua_fill_rect(lua_State *L)
{
    int x;
    int y;
    int w;
    int h;
    lv_color_t c;
    int a;

    x = (int)luaL_checknumber(L, 1);
    y = (int)luaL_checknumber(L, 2);
    w = (int)luaL_checknumber(L, 3);
    h = (int)luaL_checknumber(L, 4);
    c = divoom_game_parse_color(L, 5);
    a = (int)luaL_optnumber(L, 6, 255); /* 可选第 6 参：alpha 0~255 */
    if (a < 0)
    {
        a = 0;
    }
    if (a > 255)
    {
        a = 255;
    }
    divoom_game_fb_fill_rect_a(x, y, w, h, c, (uint8)a);
    divoom_game_native_lua_mark_dirty();
    return 0;
}

static int divoom_game_lua_draw_rect(lua_State *L)
{
    int x;
    int y;
    int w;
    int h;
    lv_color_t c;

    x = (int)luaL_checknumber(L, 1);
    y = (int)luaL_checknumber(L, 2);
    w = (int)luaL_checknumber(L, 3);
    h = (int)luaL_checknumber(L, 4);
    c = divoom_game_parse_color(L, 5);
    divoom_game_fb_draw_rect(x, y, w, h, c);
    divoom_game_native_lua_mark_dirty();
    return 0;
}

/* game.fill_round_rect(x,y,w,h,r,color[,alpha]) */
static int divoom_game_lua_fill_round_rect(lua_State *L)
{
    int x;
    int y;
    int w;
    int h;
    int r;
    lv_color_t c;
    int a;

    x = (int)luaL_checknumber(L, 1);
    y = (int)luaL_checknumber(L, 2);
    w = (int)luaL_checknumber(L, 3);
    h = (int)luaL_checknumber(L, 4);
    r = (int)luaL_checknumber(L, 5);
    c = divoom_game_parse_color(L, 6);
    a = (int)luaL_optnumber(L, 7, 255);
    if (a < 0)
    {
        a = 0;
    }
    if (a > 255)
    {
        a = 255;
    }
    divoom_game_fb_fill_round_rect(x, y, w, h, r, c, (uint8)a);
    divoom_game_native_lua_mark_dirty();
    return 0;
}

/* game.draw_round_rect(x,y,w,h,r,color[,thickness]) */
static int divoom_game_lua_draw_round_rect(lua_State *L)
{
    int x;
    int y;
    int w;
    int h;
    int r;
    lv_color_t c;
    int t;

    x = (int)luaL_checknumber(L, 1);
    y = (int)luaL_checknumber(L, 2);
    w = (int)luaL_checknumber(L, 3);
    h = (int)luaL_checknumber(L, 4);
    r = (int)luaL_checknumber(L, 5);
    c = divoom_game_parse_color(L, 6);
    t = (int)luaL_optnumber(L, 7, 1);
    divoom_game_fb_draw_round_rect(x, y, w, h, r, t, c);
    divoom_game_native_lua_mark_dirty();
    return 0;
}

/* game.fill_grad_rect(x,y,w,h,c1,c2[,dir]) dir=0 竖向/1 横向 */
static int divoom_game_lua_fill_grad_rect(lua_State *L)
{
    int x;
    int y;
    int w;
    int h;
    lv_color_t c1;
    lv_color_t c2;
    int dir;

    x = (int)luaL_checknumber(L, 1);
    y = (int)luaL_checknumber(L, 2);
    w = (int)luaL_checknumber(L, 3);
    h = (int)luaL_checknumber(L, 4);
    c1 = divoom_game_parse_color(L, 5);
    c2 = divoom_game_parse_color(L, 6);
    dir = (int)luaL_optnumber(L, 7, 0);
    divoom_game_fb_fill_grad_rect(x, y, w, h, c1, c2, dir);
    divoom_game_native_lua_mark_dirty();
    return 0;
}

static int divoom_game_lua_key_slot_from_name(const char *name)
{
    if (strcmp(name, "up") == 0)
    {
        return DIVOOM_GAME_KEY_UP;
    }
    if (strcmp(name, "down") == 0)
    {
        return DIVOOM_GAME_KEY_DOWN;
    }
    if (strcmp(name, "left") == 0)
    {
        return DIVOOM_GAME_KEY_LEFT;
    }
    if (strcmp(name, "right") == 0)
    {
        return DIVOOM_GAME_KEY_RIGHT;
    }
    if (strcmp(name, "a") == 0)
    {
        return DIVOOM_GAME_KEY_A;
    }
    if (strcmp(name, "b") == 0)
    {
        return DIVOOM_GAME_KEY_B;
    }
    if (strcmp(name, "start") == 0)
    {
        return DIVOOM_GAME_KEY_START;
    }
    return -1;
}

static int divoom_game_lua_input_down(lua_State *L)
{
    const char *key;
    int slot;

    key = luaL_checkstring(L, 1);
    slot = divoom_game_lua_key_slot_from_name(key);
    if (slot < 0)
    {
        lua_pushboolean(L, 0);
        return 1;
    }
    /* 超时自动松开：距最近一次按下/按住事件超过阈值则视为已松开，
     * 兜底轻点(SHORT 无 UP)与漏收释放的情况，避免卡死 */
    if (g_native_lua_ctrl->key_down[slot]
        && (uint32)(lv_tick_get() - g_native_lua_ctrl->key_last_ms[slot]) > DIVOOM_GAME_KEY_HOLD_TIMEOUT_MS)
    {
        g_native_lua_ctrl->key_down[slot] = 0;
    }
    lua_pushboolean(L, g_native_lua_ctrl->key_down[slot] ? 1 : 0);
    return 1;
}

static int divoom_game_lua_input_pressed(lua_State *L)
{
    const char *key;
    int slot;

    key = luaL_checkstring(L, 1);
    slot = divoom_game_lua_key_slot_from_name(key);
    if (slot < 0)
    {
        lua_pushboolean(L, 0);
        return 1;
    }
    lua_pushboolean(L, g_native_lua_ctrl->key_pressed[slot] ? 1 : 0);
    return 1;
}

static int divoom_game_lua_quit(lua_State *L)
{
    (void)L;
#if DIVOOM_GAME_NATIVE_LUA_AUDIO_ENABLE
    divoom_local_music_ctrl_stop_tone();
#endif
    g_native_lua_ctrl->quit_request = 1;
    return 0;
}

/* game.consume_back()：Lua 在 on_key("b") 里逐级返回上一层时调用，告知 C 层「ESC 已被消费」，
 * 不要把它当成退出整局游戏处理。顶层退出请改用 game.quit()。 */
static int divoom_game_lua_consume_back(lua_State *L)
{
    (void)L;
    g_native_lua_ctrl->back_consumed = 1;
    return 0;
}

/* game.log(msg)：量产环境默认静默；自动验收只放行 ARCADE_ 状态事件，避免逐帧日志增加 CPU 和 I/O。 */
static int divoom_game_lua_log(lua_State *L)
{
    const char *message;
    const char *autoplay;
    const char *key_verify;
    const char *pet_autoplay;

    message = luaL_checkstring(L, 1);
    autoplay = getenv("DIVOOM_ARCADE_AUTOPLAY");
    key_verify = getenv("DIVOOM_ARCADE_KEY_VERIFY");
    pet_autoplay = getenv("DIVOOM_ASTRO_PET_AUTOPLAY");
    if (((autoplay != NULL && strcmp(autoplay, "1") == 0)
        || (key_verify != NULL && strcmp(key_verify, "1") == 0))
        && strncmp(message, "ARCADE_", 7) == 0)
    {
        divoom_print("[game_lua] %s\n", message);
    }
    else if (pet_autoplay != NULL && strcmp(pet_autoplay, "1") == 0
        && strncmp(message, "pet ", 4) == 0)
    {
        /* AstroPet 自动验收仅输出状态事件，不打开其它游戏的量产日志。 */
        divoom_print("[astro_lua] %s\n", message);
    }
    else if (strncmp(message, "astro_diag ", 11) == 0)
    {
        /* AstroRun 性能诊断只输出一次性/切帧级事件，避免恢复逐帧日志。 */
        divoom_print("[astro_lua] %s\n", message);
    }
    return 0;
}

/* game.log_mem(stage)：状态切换诊断使用，统一输出总空闲、最大连续块和游戏缓存占用。 */
static int divoom_game_lua_log_mem(lua_State *L)
{
    (void)L;
    return 0;
}

/* game.getenv(name)：读取进程环境变量；不存在返回 nil。用于按 env 开启自动驾驶等调试开关 */
static int divoom_game_lua_getenv(lua_State *L)
{
    const char *name;
    const char *val;

    name = luaL_checkstring(L, 1);
    val = getenv(name);
    if (val != NULL)
    {
        lua_pushstring(L, val);
    }
    else
    {
        lua_pushnil(L);
    }
    return 1;
}

static void divoom_game_native_lua_free_img_cache_slot(divoom_game_img_cache_entry_t *slot)
{
    if (slot == NULL)
    {
        return;
    }
    if (slot->valid && slot->data)
    {
        if (!slot->external_data)
        {
            divoom_system_free(slot->data);
        }
        if (slot->external_data)
        {
            if (g_native_lua_ctrl->img_cache_external_bytes >= slot->byte_size)
            {
                g_native_lua_ctrl->img_cache_external_bytes -= slot->byte_size;
            }
            else
            {
                g_native_lua_ctrl->img_cache_external_bytes = 0;
            }
        }
        else
        {
            if (g_native_lua_ctrl->img_cache_owned_bytes >= slot->byte_size)
            {
                g_native_lua_ctrl->img_cache_owned_bytes -= slot->byte_size;
            }
            else
            {
                g_native_lua_ctrl->img_cache_owned_bytes = 0;
            }
        }
    }
    memset(slot, 0, sizeof(*slot));
}

static void divoom_game_native_lua_release_webp_anim(void)
{
    if (g_native_lua_ctrl->webp_anim.frame_buf != NULL)
    {
        divoom_system_free(g_native_lua_ctrl->webp_anim.frame_buf);
        g_native_lua_ctrl->webp_anim.frame_buf = NULL;
    }
    if (g_native_lua_ctrl->webp_anim.handle != NULL)
    {
        divoom_libwebp_decode_exit(g_native_lua_ctrl->webp_anim.handle);
        g_native_lua_ctrl->webp_anim.handle = NULL;
    }
    memset(&g_native_lua_ctrl->webp_anim, 0, sizeof(g_native_lua_ctrl->webp_anim));
}

static void divoom_game_native_lua_set_img_cache_negative(
    divoom_game_img_cache_entry_t *slot, const char *rel)
{
    if (slot == NULL || rel == NULL)
    {
        return;
    }
    divoom_game_native_lua_free_img_cache_slot(slot);
    lv_snprintf(slot->path, sizeof(slot->path), "%s", rel);
    slot->data = NULL;
    slot->w = 0;
    slot->h = 0;
    slot->byte_size = 0;
    slot->format = DIVOOM_GAME_IMG_CACHE_FMT_NONE;
    slot->last_used_frame = g_native_lua_ctrl->frame_seq;
    slot->valid = 1;
}

static void divoom_game_native_lua_set_img_cache_retry(
    divoom_game_img_cache_entry_t *slot, const char *rel)
{
    if (slot == NULL || rel == NULL)
    {
        return;
    }
    divoom_game_native_lua_free_img_cache_slot(slot);
    lv_snprintf(slot->path, sizeof(slot->path), "%s", rel);
    slot->last_used_frame = g_native_lua_ctrl->frame_seq;
    slot->retry_after_frame = g_native_lua_ctrl->frame_seq +
        DIVOOM_GAME_IMG_CACHE_RETRY_FRAMES;
    slot->retryable = 1;
    slot->valid = 1;
}

static uint8 divoom_game_native_lua_can_decode_jpeg(const char *file_path,
    int file_len, uint8 input_reuse)
{
    long largest_free_kb;
    long total_free_kb;
    long need_free_kb;
    uint8 use_base_pool;

    use_base_pool = (g_native_lua_ctrl->base_rgb565_buf != NULL &&
        g_native_lua_ctrl->base_rgb565_buf_size >= DIVOOM_GAME_BASE_RGB565_POOL_BYTES) ? 1 : 0;
    need_free_kb = use_base_pool ? DIVOOM_GAME_JPEG_DECODE_POOL_MIN_LARGEST_FREE_KB :
        DIVOOM_GAME_JPEG_DECODE_MIN_LARGEST_FREE_KB;
    total_free_kb = divoom_system_get_ram_free_size();
    largest_free_kb = divoom_system_get_ram_largest_free_block_size();
    if (largest_free_kb > 0 &&
        largest_free_kb < need_free_kb)
    {
        /*
         * JPEG 硬件解码器内部仍会申请 CEDAR/ION 帧缓冲。即使 RGB565 输出已有复用池，
         * 连续内存过低时也不能硬闯，否则会影响系统 PNG/G2D 解码。
         */
        divoom_print("[native_lua] skip jpg decode low heap total_free_kb=%ld largest_free_kb=%ld need_kb=%ld pool=%d input_reuse=%d file_len=%d path=%s\n",
            total_free_kb, largest_free_kb, need_free_kb, use_base_pool,
            input_reuse, file_len, file_path ? file_path : "");
        return 0;
    }
    return 1;
}

static uint8 divoom_game_native_lua_img_cache_slot_is_base(
    const divoom_game_img_cache_entry_t *slot)
{
    if (slot == NULL || !slot->valid || slot->data == NULL)
    {
        return 0;
    }
    if (slot->format == DIVOOM_GAME_IMG_CACHE_FMT_RGB565 &&
        slot->byte_size >= DIVOOM_GAME_IMG_CACHE_LARGE_BYTES)
    {
        return 1;
    }
    return divoom_game_native_lua_is_base_path(slot->path);
}

static void divoom_game_native_lua_prune_large_img_cache(divoom_game_img_cache_entry_t *protect_slot)
{
    int i;

    for (i = 0; i < DIVOOM_GAME_IMG_CACHE_MAX; i++)
    {
        divoom_game_img_cache_entry_t *slot;

        slot = &g_native_lua_ctrl->img_cache[i];
        if (slot == protect_slot || !slot->valid || slot->data == NULL)
        {
            continue;
        }
        if (divoom_game_native_lua_img_cache_slot_is_base(slot))
        {
            divoom_game_native_lua_free_img_cache_slot(slot);
        }
    }
}

static void divoom_game_native_lua_prune_img_cache(uint32 need_bytes,
    divoom_game_img_cache_entry_t *protect_slot)
{
    while (g_native_lua_ctrl->img_cache_owned_bytes + need_bytes > DIVOOM_GAME_IMG_CACHE_BYTES_MAX)
    {
        divoom_game_img_cache_entry_t *oldest;
        uint32 oldest_idle;
        int i;

        oldest = NULL;
        oldest_idle = 0;
        for (i = 0; i < DIVOOM_GAME_IMG_CACHE_MAX; i++)
        {
            divoom_game_img_cache_entry_t *slot;
            uint32 idle;

            slot = &g_native_lua_ctrl->img_cache[i];
            if (!slot->valid || slot == protect_slot || slot->data == NULL)
            {
                continue;
            }
            if (divoom_game_native_lua_img_cache_slot_is_base(slot))
            {
                continue;
            }
            idle = g_native_lua_ctrl->frame_seq - slot->last_used_frame;
            if (oldest == NULL || idle > oldest_idle)
            {
                oldest = slot;
                oldest_idle = idle;
            }
        }
        if (oldest == NULL)
        {
            for (i = 0; i < DIVOOM_GAME_IMG_CACHE_MAX; i++)
            {
                divoom_game_img_cache_entry_t *slot;
                uint32 idle;

                slot = &g_native_lua_ctrl->img_cache[i];
                if (!slot->valid || slot == protect_slot || slot->data == NULL)
                {
                    continue;
                }
                idle = g_native_lua_ctrl->frame_seq - slot->last_used_frame;
                if (oldest == NULL || idle > oldest_idle)
                {
                    oldest = slot;
                    oldest_idle = idle;
                }
            }
        }
        if (oldest == NULL)
        {
            return;
        }
        divoom_game_native_lua_free_img_cache_slot(oldest);
    }
}

static int divoom_game_native_lua_get_img_cache_count(void)
{
    int i;
    int count;

    count = 0;
    for (i = 0; i < DIVOOM_GAME_IMG_CACHE_MAX; i++)
    {
        if (g_native_lua_ctrl->img_cache[i].valid)
        {
            count++;
        }
    }
    return count;
}

static uint16 divoom_game_rgb_to_rgb565(uint8 r, uint8 g, uint8 b)
{
    return (uint16)(((uint16)(r & 0xF8) << 8) |
        ((uint16)(g & 0xFC) << 3) |
        ((uint16)b >> 3));
}

static void divoom_game_rgb565_store_le(uint8 *dst, uint16 pixel)
{
    if (dst == NULL)
    {
        return;
    }
    dst[0] = (uint8)(pixel & 0xFF);
    dst[1] = (uint8)((pixel >> 8) & 0xFF);
}

static uint16 divoom_game_rgb565_load_le(const uint8 *src)
{
    if (src == NULL)
    {
        return 0;
    }
    return (uint16)((uint16)src[0] | ((uint16)src[1] << 8));
}

static void divoom_game_native_lua_get_png_rgba(const uint8 *src,
    uint8 *r, uint8 *g, uint8 *b, uint8 *a)
{
    if (src == NULL || r == NULL || g == NULL || b == NULL || a == NULL)
    {
        return;
    }
    /* divoom_rk_mpp_get_png_info normalizes simulator and F101 output to RGBA. */
    *r = src[0];
    *g = src[1];
    *b = src[2];
    *a = src[3];
}

static void divoom_game_rgb565_to_rgb(uint16 pixel, uint8 *r, uint8 *g, uint8 *b)
{
    uint8 r5;
    uint8 g6;
    uint8 b5;

    r5 = (uint8)((pixel >> 11) & 0x1F);
    g6 = (uint8)((pixel >> 5) & 0x3F);
    b5 = (uint8)(pixel & 0x1F);
    *r = (uint8)((r5 << 3) | (r5 >> 2));
    *g = (uint8)((g6 << 2) | (g6 >> 4));
    *b = (uint8)((b5 << 3) | (b5 >> 2));
}

static uint8 divoom_game_native_lua_is_base_image(const char *rel, int w, int h)
{
    const char *name;

    if (rel == NULL)
    {
        return 0;
    }
    if (strstr(rel, "background/") != NULL)
    {
        return 1;
    }
    if (w == DIVOOM_GAME_FB_W && h == DIVOOM_GAME_FB_H &&
        strstr(rel, "pages/") != NULL)
    {
        return 1;
    }
    name = strrchr(rel, '/');
    name = (name != NULL) ? (name + 1) : rel;
    if (w == DIVOOM_GAME_FB_W && h == DIVOOM_GAME_FB_H &&
        strncmp(name, "bg_", 3) == 0)
    {
        return 1;
    }
    if (((w == DIVOOM_GAME_LOGIC_W && h == DIVOOM_GAME_LOGIC_H) ||
            (w == DIVOOM_GAME_FB_W && h == DIVOOM_GAME_FB_H)) &&
        strncmp(name, "game05_", 7) == 0)
    {
        /* AstroRun 开始/结束动画帧是整屏不透明底图；按 base 图用 RGB565 缓存，
         * 避免 240/480 整屏帧被当作带 alpha 小图占用 1.5 倍缓存并频繁触发解码。 */
        return 1;
    }
    return 0;
}

static uint8 divoom_game_native_lua_is_base_path(const char *rel)
{
    const char *name;

    if (rel == NULL)
    {
        return 0;
    }
    if (strstr(rel, "background/") != NULL)
    {
        return 1;
    }
    if (strstr(rel, "pages/") != NULL)
    {
        return 1;
    }
    name = strrchr(rel, '/');
    name = (name != NULL) ? (name + 1) : rel;
    if (strncmp(name, "bg_", 3) == 0)
    {
        return 1;
    }
    return 0;
}

static uint8 divoom_game_native_lua_replace_path_ext(const char *path, const char *ext,
    char *out, int out_sz)
{
    const char *dot;
    const char *slash;
    size_t prefix_len;
    size_t ext_len;

    if (path == NULL || ext == NULL || out == NULL || out_sz <= 0)
    {
        return 0;
    }
    dot = strrchr(path, '.');
    slash = strrchr(path, '/');
    if (dot == NULL || (slash != NULL && slash > dot))
    {
        return 0;
    }
    prefix_len = (size_t)(dot - path);
    ext_len = strlen(ext);
    if (prefix_len + ext_len + 1U > (size_t)out_sz)
    {
        return 0;
    }
    memcpy(out, path, prefix_len);
    memcpy(out + prefix_len, ext, ext_len);
    out[prefix_len + ext_len] = '\0';
    return 1;
}

static uint8 divoom_game_native_lua_path_has_ext(const char *path, const char *ext)
{
    const char *dot;

    if (path == NULL || ext == NULL)
    {
        return 0;
    }
    dot = strrchr(path, '.');
    if (dot == NULL)
    {
        return 0;
    }
    return (strcmp(dot, ext) == 0) ? 1 : 0;
}

static uint8 divoom_game_native_lua_read_png_ihdr_file(const char *path,
    int *w, int *h)
{
    uint8 hdr[24];
    void *file;
    size_t read_len;

    if (path == NULL || w == NULL || h == NULL)
    {
        return 0;
    }
    file = divoom_port_file_open(path, "rb");
    if (file == NULL)
    {
        return 0;
    }
    read_len = divoom_port_file_read(hdr, 1, sizeof(hdr), file);
    divoom_port_file_close(file);
    if (read_len != sizeof(hdr))
    {
        return 0;
    }
    return divoom_aw_parse_png_ihdr_info((const char *)hdr, (int)sizeof(hdr), w, h);
}

static uint8 *divoom_game_native_lua_decode_jpeg_rgb565_file(const char *file_path,
    int *w, int *h, uint8 *external_data, uint8 *retryable_fail)
{
    int file_len;
    uint8 *file_buf;
    uint8 *rgb565_buf;
    uint8 input_reuse;

    if (w != NULL)
    {
        *w = 0;
    }
    if (h != NULL)
    {
        *h = 0;
    }
    if (external_data != NULL)
    {
        *external_data = 0;
    }
    if (retryable_fail != NULL)
    {
        *retryable_fail = 0;
    }
    if (file_path == NULL || w == NULL || h == NULL)
    {
        return NULL;
    }
    if (!divoom_game_native_lua_get_file_size(file_path, &file_len))
    {
        return NULL;
    }
    input_reuse = (g_native_lua_ctrl->base_rgb565_buf != NULL &&
        g_native_lua_ctrl->base_rgb565_buf_size > 0 &&
        file_len <= (int)g_native_lua_ctrl->base_rgb565_buf_size) ? 1 : 0;
    if (!divoom_game_native_lua_can_decode_jpeg(file_path, file_len, input_reuse))
    {
        if (retryable_fail != NULL)
        {
            *retryable_fail = 1;
        }
        return NULL;
    }
    /*
     * 连续块门槛通过后才允许覆盖 base pool。低内存 skip 时保留旧底图 slot；
     * 一旦真正开始读新 JPG，输入/输出都会覆盖同一池，旧 slot 必须先失效。
     */
    if (g_native_lua_ctrl->base_rgb565_buf != NULL)
    {
        divoom_game_native_lua_prune_large_img_cache(NULL);
    }
    divoom_game_native_lua_log_mem_stage("jpg decode before");
    file_buf = NULL;
    if (input_reuse)
    {
        if (!divoom_game_native_lua_read_file_to_buf(file_path,
                g_native_lua_ctrl->base_rgb565_buf,
                g_native_lua_ctrl->base_rgb565_buf_size, &file_len))
        {
            return NULL;
        }
        file_buf = g_native_lua_ctrl->base_rgb565_buf;
    }
    else
    {
        file_buf = (uint8 *)divoom_game_native_lua_read_file(file_path, &file_len);
    }
    if (file_buf == NULL || file_len <= 0)
    {
        return NULL;
    }
    if (g_native_lua_ctrl->base_rgb565_buf != NULL &&
        g_native_lua_ctrl->base_rgb565_buf_size > 0)
    {
        /*
         * 页底图输出固定写入启动时预留的 RGB565 池。游戏开始能拿到连续大块后，
         * 后续切场景不再向堆申请 480x480 输出图，降低碎片导致 JPEG 失败的概率。
         */
        rgb565_buf = divoom_rk_mpp_decode_jpeg_rgb565_to_buf_info((char *)file_buf,
            file_len, w, h, g_native_lua_ctrl->base_rgb565_buf,
            g_native_lua_ctrl->base_rgb565_buf_size);
        if (rgb565_buf != NULL)
        {
            if (external_data != NULL)
            {
                *external_data = 1;
            }
            divoom_print("[native_lua] jpg decode reused base pool largest_kb=%ld need_kb=%ld pool=1 input_reuse=%d file_len=%d path=%s w=%d h=%d\n",
                divoom_system_get_ram_largest_free_block_size(),
                (long)DIVOOM_GAME_JPEG_DECODE_POOL_MIN_LARGEST_FREE_KB,
                input_reuse, file_len, file_path, *w, *h);
            divoom_game_native_lua_log_mem_stage("jpg decode after");
        }
        else
        {
            divoom_print("[native_lua] jpg decode base pool failed largest_kb=%ld need_kb=%ld pool=1 input_reuse=%d file_len=%d path=%s\n",
                divoom_system_get_ram_largest_free_block_size(),
                (long)DIVOOM_GAME_JPEG_DECODE_POOL_MIN_LARGEST_FREE_KB,
                input_reuse, file_len, file_path ? file_path : "");
            divoom_game_native_lua_log_mem_stage("jpg decode failed");
        }
    }
    else
    {
        /* 兼容兜底：没有预留池时维持旧行为。 */
        rgb565_buf = divoom_rk_mpp_decode_jpeg_rgb565_info((char *)file_buf, file_len, w, h);
        divoom_game_native_lua_log_mem_stage(rgb565_buf != NULL ?
            "jpg decode after" : "jpg decode failed");
    }
    if (!input_reuse)
    {
        divoom_game_native_lua_free_file_buf((char *)file_buf);
    }
    return rgb565_buf;
}

static uint8 *divoom_game_native_lua_try_decode_base_jpeg(const char *rel,
    const char *full_path, int *w, int *h, char *used_path, int used_path_sz,
    uint8 *external_data, uint8 *retryable_fail)
{
    char jpg_path[DIVOOM_GAME_PATH_MAX];
    uint8 *rgb565_buf;
    uint8 any_retryable_fail;
    uint8 jpeg_retryable_fail;

    any_retryable_fail = 0;
    jpeg_retryable_fail = 0;
    if (!divoom_game_native_lua_is_base_path(rel))
    {
        return NULL;
    }
    if (retryable_fail != NULL)
    {
        *retryable_fail = 0;
    }
    if (divoom_game_native_lua_path_has_ext(full_path, ".jpg") ||
        divoom_game_native_lua_path_has_ext(full_path, ".jpeg"))
    {
        /*
         * 调用方明确传入 JPG 时只走 JPEG 解码。失败后不能再把同一个
         * JPG 交给 PNG 解码，否则真机会出现 ihdr parse fail 并反复申请内存。
         */
        rgb565_buf = divoom_game_native_lua_decode_jpeg_rgb565_file(full_path, w, h,
            external_data, retryable_fail);
        if (rgb565_buf != NULL)
        {
            lv_snprintf(used_path, (uint32)used_path_sz, "%s", full_path);
        }
        return rgb565_buf;
    }
    if (divoom_game_native_lua_replace_path_ext(full_path, ".jpg", jpg_path,
            (int)sizeof(jpg_path)))
    {
        jpeg_retryable_fail = 0;
        rgb565_buf = divoom_game_native_lua_decode_jpeg_rgb565_file(jpg_path, w, h,
            external_data, &jpeg_retryable_fail);
        if (jpeg_retryable_fail)
        {
            any_retryable_fail = 1;
        }
        if (rgb565_buf != NULL)
        {
            lv_snprintf(used_path, (uint32)used_path_sz, "%s", jpg_path);
            return rgb565_buf;
        }
    }
    if (divoom_game_native_lua_replace_path_ext(full_path, ".jpeg", jpg_path,
            (int)sizeof(jpg_path)))
    {
        jpeg_retryable_fail = 0;
        rgb565_buf = divoom_game_native_lua_decode_jpeg_rgb565_file(jpg_path, w, h,
            external_data, &jpeg_retryable_fail);
        if (jpeg_retryable_fail)
        {
            any_retryable_fail = 1;
        }
        if (rgb565_buf != NULL)
        {
            lv_snprintf(used_path, (uint32)used_path_sz, "%s", jpg_path);
            return rgb565_buf;
        }
    }
    if (retryable_fail != NULL)
    {
        *retryable_fail = any_retryable_fail;
    }
    return NULL;
}

static uint8 *divoom_game_native_lua_decode_png_cache_file(const char *rel,
    const char *full_path, int *w, int *h, uint8 *format, uint32 *byte_size)
{
    uint8 *cache_data;
    uint8 *file_buf;
    uint8 *input_buf;
    int file_len;
    int png_w;
    int png_h;
    lv_color_format_t cf;
    uint32 cache_size;
    uint8 input_reuse;

    if (rel == NULL || full_path == NULL || w == NULL || h == NULL ||
        format == NULL || byte_size == NULL)
    {
        return NULL;
    }
    if (!divoom_game_native_lua_get_file_size(full_path, &file_len) ||
        !divoom_game_native_lua_read_png_ihdr_file(full_path, &png_w, &png_h))
    {
        return NULL;
    }
    *format = divoom_game_native_lua_is_base_image(rel, png_w, png_h) ?
        DIVOOM_GAME_IMG_CACHE_FMT_RGB565 : DIVOOM_GAME_IMG_CACHE_FMT_RGB565A8;
    cf = (*format == DIVOOM_GAME_IMG_CACHE_FMT_RGB565) ?
        LV_COLOR_FORMAT_RGB565 : LV_COLOR_FORMAT_RGB565A8;
    cache_size = divoom_pixel_format_size((uint32)png_w, (uint32)png_h, cf);
    if (cache_size == 0)
    {
        return NULL;
    }

    divoom_game_native_lua_log_mem_stage("png decode before");
    divoom_game_native_lua_prune_img_cache(cache_size, NULL);
    cache_data = (uint8 *)divoom_system_malloc(cache_size);
    if (cache_data == NULL)
    {
        divoom_game_native_lua_prune_img_cache(cache_size, NULL);
        cache_data = (uint8 *)divoom_system_malloc(cache_size);
    }
    if (cache_data == NULL)
    {
        return NULL;
    }

    file_buf = NULL;
    input_reuse = ((uint32)file_len <= cache_size) ? 1 : 0;
    if (input_reuse)
    {
        if (!divoom_game_native_lua_read_file_to_buf(full_path, cache_data,
                cache_size, &file_len))
        {
            divoom_system_free(cache_data);
            return NULL;
        }
        input_buf = cache_data;
    }
    else
    {
        file_buf = (uint8 *)divoom_game_native_lua_read_file(full_path, &file_len);
        if (file_buf == NULL || file_len <= 0)
        {
            if (file_buf != NULL)
            {
                divoom_game_native_lua_free_file_buf((char *)file_buf);
            }
            divoom_system_free(cache_data);
            return NULL;
        }
        input_buf = file_buf;
    }

    if (divoom_aw_decode_png_buf_to_lvfmt((const char *)input_buf, file_len, w,
            h, cache_data, cache_size, cf) == NULL)
    {
        if (!input_reuse)
        {
            divoom_game_native_lua_free_file_buf((char *)file_buf);
        }
        divoom_system_free(cache_data);
        return NULL;
    }
    if (!input_reuse)
    {
        divoom_game_native_lua_free_file_buf((char *)file_buf);
    }
    *byte_size = cache_size;
#if DIVOOM_LVGL9_IS_SIM
    if (getenv("DIVOOM_GAME_IMG_DECODE_LOG") != NULL)
    {
        divoom_print("[native_lua] png cache decoded input_reuse=%d file_len=%d cache_size=%u fmt=%d path=%s w=%d h=%d\n",
            input_reuse, file_len, cache_size, *format, full_path, *w, *h);
    }
#endif
    divoom_game_native_lua_log_mem_stage("png decode after");
    return cache_data;
}

static uint8 *divoom_game_native_lua_build_cached_image(const char *rel, const uint8 *rgba,
    int w, int h, uint8 *format, uint32 *byte_size)
{
    uint8 fmt;
    uint8 *data;
    uint8 *rgb565;
    uint8 *alpha;
    uint32 count;
    uint32 size;
    uint32 i;

    if (rgba == NULL || w <= 0 || h <= 0 || format == NULL || byte_size == NULL)
    {
        return NULL;
    }
    count = (uint32)w * (uint32)h;
    if (count == 0 || count > (UINT32_MAX / 3U))
    {
        return NULL;
    }

    fmt = divoom_game_native_lua_is_base_image(rel, w, h) ?
        DIVOOM_GAME_IMG_CACHE_FMT_RGB565 : DIVOOM_GAME_IMG_CACHE_FMT_RGB565A8;
    size = count * ((fmt == DIVOOM_GAME_IMG_CACHE_FMT_RGB565) ? 2U : 3U);
    data = (uint8 *)divoom_system_malloc(size);
    if (data == NULL)
    {
        divoom_game_native_lua_prune_img_cache(size, NULL);
        data = (uint8 *)divoom_system_malloc(size);
    }
    if (data == NULL)
    {
        return NULL;
    }

    rgb565 = data;
    alpha = (fmt == DIVOOM_GAME_IMG_CACHE_FMT_RGB565A8) ? (data + count * 2U) : NULL;
    for (i = 0; i < count; i++)
    {
        const uint8 *src;
        uint8 r;
        uint8 g;
        uint8 b;
        uint8 a;

        src = rgba + i * 4U;
        divoom_game_native_lua_get_png_rgba(src, &r, &g, &b, &a);
        divoom_game_rgb565_store_le(rgb565 + i * 2U, divoom_game_rgb_to_rgb565(r, g, b));
        if (alpha != NULL)
        {
            alpha[i] = a;
        }
    }

    *format = fmt;
    *byte_size = size;
    return data;
}

static void divoom_game_native_lua_free_img_cache(void)
{
    int i;

    for (i = 0; i < DIVOOM_GAME_IMG_CACHE_MAX; i++)
    {
        divoom_game_native_lua_free_img_cache_slot(&g_native_lua_ctrl->img_cache[i]);
    }
    g_native_lua_ctrl->img_cache_owned_bytes = 0;
    g_native_lua_ctrl->img_cache_external_bytes = 0;
    g_native_lua_ctrl->img_cache_last_sweep = g_native_lua_ctrl->frame_seq;
    divoom_game_native_lua_release_webp_anim();
}

static void divoom_game_native_lua_sweep_img_cache(void)
{
    uint32 now_frame;
    uint32 released_bytes;
    int released_count;
    int i;

    now_frame = g_native_lua_ctrl->frame_seq;
    if (now_frame - g_native_lua_ctrl->img_cache_last_sweep < DIVOOM_GAME_IMG_CACHE_SWEEP_FRAMES)
    {
        return;
    }
    g_native_lua_ctrl->img_cache_last_sweep = now_frame;
    released_bytes = 0;
    released_count = 0;

    for (i = 0; i < DIVOOM_GAME_IMG_CACHE_MAX; i++)
    {
        divoom_game_img_cache_entry_t *slot;
        uint32 idle_frames;

        slot = &g_native_lua_ctrl->img_cache[i];
        if (!slot->valid)
        {
            continue;
        }
        /*
         * external_data 指向当前页面的 base RGB565 池，清 slot 并不会释放内存，
         * 反而会让静态阶段再次请求同一 JPG 并触发整套硬解。该 slot 由下一张
         * 大底图进入时的 prune_large 统一替换，确保池内容和路径始终一致。
         */
        if (slot->external_data)
        {
            continue;
        }
        idle_frames = now_frame - slot->last_used_frame;
        if (slot->data != NULL && idle_frames > DIVOOM_GAME_IMG_CACHE_KEEP_FRAMES)
        {
            /* 所有 native Lua 游戏共用：页面切换后，约 2 秒未再绘制的图片释放，避免整局常驻。 */
            released_bytes += slot->byte_size;
            released_count++;
            divoom_game_native_lua_free_img_cache_slot(slot);
        }
        else if (slot->data == NULL && idle_frames > DIVOOM_GAME_IMG_CACHE_NEG_KEEP_FRAMES)
        {
            memset(slot, 0, sizeof(*slot));
        }
    }
    if (released_count > 0)
    {
        divoom_print("[native_lua] cache sweep released_count=%d released_bytes=%u frame=%u\n",
            released_count, released_bytes, now_frame);
    }
}

static void divoom_game_native_lua_resolve_asset_path(const char *rel, char *out, int out_sz)
{
    const char *use_rel;

    if (rel == NULL || out == NULL || out_sz <= 0)
    {
        return;
    }
    use_rel = rel;
    if (strncmp(use_rel, "common/", 7) == 0)
    {
        lv_snprintf(out, (uint32)out_sz, "%s/%s", DIVOOM_GAMES_SEED_ROOT, use_rel);
    }
    else if (strncmp(use_rel, "assets/", 7) != 0)
    {
        lv_snprintf(out, (uint32)out_sz, "%s/assets/%s",
            g_native_lua_ctrl->entry.dir_abs, use_rel);
    }
    else
    {
        lv_snprintf(out, (uint32)out_sz, "%s/%s", g_native_lua_ctrl->entry.dir_abs, use_rel);
    }
}

static divoom_game_img_cache_entry_t *divoom_game_native_lua_img_cache_get(const char *rel)
{
    char full_path[DIVOOM_GAME_PATH_MAX];
    char decode_path[DIVOOM_GAME_PATH_MAX];
    divoom_game_img_cache_entry_t *slot;
    divoom_game_img_cache_entry_t *oldest;
    uint32 oldest_idle;
    int i;
    int w;
    int h;
    uint8 *rgb565;
    uint8 *cache_data;
    uint8 cache_format;
    uint8 cache_external;
    uint8 jpg_retryable_fail;
    uint8 is_base_path;
    uint8 is_jpeg_path;
    uint32 byte_size;
    uint32 decode_reserve;

    divoom_game_native_lua_resolve_asset_path(rel, full_path, (int)sizeof(full_path));
    lv_snprintf(decode_path, sizeof(decode_path), "%s", full_path);
    is_base_path = divoom_game_native_lua_is_base_path(rel);
    is_jpeg_path = (divoom_game_native_lua_path_has_ext(full_path, ".jpg") ||
        divoom_game_native_lua_path_has_ext(full_path, ".jpeg")) ? 1 : 0;
    for (i = 0; i < DIVOOM_GAME_IMG_CACHE_MAX; i++)
    {
        if (g_native_lua_ctrl->img_cache[i].valid &&
            strcmp(g_native_lua_ctrl->img_cache[i].path, rel) == 0)
        {
            if (g_native_lua_ctrl->img_cache[i].retryable)
            {
                if (g_native_lua_ctrl->frame_seq <
                    g_native_lua_ctrl->img_cache[i].retry_after_frame)
                {
                    g_native_lua_ctrl->img_cache[i].last_used_frame =
                        g_native_lua_ctrl->frame_seq;
                    return &g_native_lua_ctrl->img_cache[i];
                }
                divoom_print("[native_lua] jpg retry window reached frame=%u path=%s\n",
                    g_native_lua_ctrl->frame_seq, rel);
                divoom_game_native_lua_free_img_cache_slot(
                    &g_native_lua_ctrl->img_cache[i]);
                break;
            }
            g_native_lua_ctrl->img_cache[i].last_used_frame = g_native_lua_ctrl->frame_seq;
            return &g_native_lua_ctrl->img_cache[i];
        }
    }

    /*
     * base pool 路径不需要为输出再预留 1536KB。旧场景的小图由 Lua 状态切换
     * 主动释放；若连续块仍不足，JPEG retry slot 会阻止每帧重复清缓存和解码。
     */
    if (is_base_path && is_jpeg_path &&
        g_native_lua_ctrl->base_rgb565_buf != NULL)
    {
        decode_reserve = DIVOOM_GAME_IMG_DECODE_SMALL_RESERVE_BYTES;
    }
    else
    {
        decode_reserve = is_base_path ? DIVOOM_GAME_IMG_DECODE_RESERVE_BYTES :
            DIVOOM_GAME_IMG_DECODE_SMALL_RESERVE_BYTES;
    }
    divoom_game_native_lua_prune_img_cache(decode_reserve, NULL);
    slot = NULL;
    oldest = NULL;
    oldest_idle = 0;
    for (i = 0; i < DIVOOM_GAME_IMG_CACHE_MAX; i++)
    {
        if (!g_native_lua_ctrl->img_cache[i].valid)
        {
            slot = &g_native_lua_ctrl->img_cache[i];
            break;
        }
        if (g_native_lua_ctrl->img_cache[i].external_data)
        {
            continue;
        }
        if (oldest == NULL ||
            g_native_lua_ctrl->frame_seq - g_native_lua_ctrl->img_cache[i].last_used_frame > oldest_idle)
        {
            oldest = &g_native_lua_ctrl->img_cache[i];
            oldest_idle = g_native_lua_ctrl->frame_seq - g_native_lua_ctrl->img_cache[i].last_used_frame;
        }
    }
    if (slot == NULL)
    {
        slot = oldest;
        divoom_game_native_lua_free_img_cache_slot(slot);
    }
    cache_external = 0;
    jpg_retryable_fail = 0;
    rgb565 = divoom_game_native_lua_try_decode_base_jpeg(rel, full_path, &w, &h,
        decode_path, (int)sizeof(decode_path), &cache_external, &jpg_retryable_fail);
    if (rgb565 != NULL)
    {
        if (w > 0 && h > 0)
        {
            byte_size = (uint32)w * (uint32)h * 2U;
            cache_format = DIVOOM_GAME_IMG_CACHE_FMT_RGB565;
            cache_data = rgb565;
            if (byte_size != 0)
            {
                goto cache_ready;
            }
        }
        if (!cache_external)
        {
            divoom_system_free(rgb565);
        }
        divoom_game_native_lua_set_img_cache_negative(slot, rel);
        divoom_print("[native_lua] jpg cache alloc failed: %s\n", decode_path);
        return NULL;
    }
    if (is_jpeg_path)
    {
        if (jpg_retryable_fail)
        {
            divoom_game_native_lua_set_img_cache_retry(slot, rel);
            divoom_print("[native_lua] jpg decode retry deferred frames=%u path=%s\n",
                (uint32)DIVOOM_GAME_IMG_CACHE_RETRY_FRAMES, full_path);
            return NULL;
        }
        divoom_game_native_lua_set_img_cache_negative(slot, rel);
        divoom_print("[native_lua] jpg decode failed cached once: %s\n", full_path);
        return NULL;
    }
    byte_size = 0;
    cache_format = DIVOOM_GAME_IMG_CACHE_FMT_NONE;
    cache_data = divoom_game_native_lua_decode_png_cache_file(rel, full_path,
        &w, &h, &cache_format, &byte_size);
    if (cache_data == NULL || w <= 0 || h <= 0 || byte_size == 0)
    {
        if (jpg_retryable_fail)
        {
            divoom_game_native_lua_set_img_cache_retry(slot, rel);
            divoom_print("[native_lua] image decode retry deferred frames=%u path=%s\n",
                (uint32)DIVOOM_GAME_IMG_CACHE_RETRY_FRAMES, full_path);
            return NULL;
        }
        /* 负缓存：记录失败路径（data=NULL），避免资源缺失时每帧重复打开文件并刷日志。
         * 后续查找命中该 slot 直接返回，draw_image 对 data=NULL 做 no-op。 */
        divoom_game_native_lua_set_img_cache_negative(slot, rel);
        divoom_print("[native_lua] image decode/read failed (cached once): %s\n", full_path);
        return NULL;
    }
cache_ready:
    if (cache_format == DIVOOM_GAME_IMG_CACHE_FMT_RGB565 &&
        byte_size >= DIVOOM_GAME_IMG_CACHE_LARGE_BYTES)
    {
        divoom_game_native_lua_prune_large_img_cache(slot);
    }
    divoom_game_native_lua_prune_img_cache(byte_size, slot);
    lv_snprintf(slot->path, sizeof(slot->path), "%s", rel);
    slot->data = cache_data;
    slot->w = w;
    slot->h = h;
    slot->byte_size = byte_size;
    slot->format = cache_format;
    slot->external_data = cache_external;
    slot->last_used_frame = g_native_lua_ctrl->frame_seq;
    slot->valid = 1;
    if (cache_external)
    {
        g_native_lua_ctrl->img_cache_external_bytes += byte_size;
    }
    else
    {
        g_native_lua_ctrl->img_cache_owned_bytes += byte_size;
    }
    return slot;
}

static void divoom_game_fb_blit_cached_scaled(int dx, int dy, int dw, int dh,
    const divoom_game_img_cache_entry_t *img)
{
    int row;
    int col;
    int x0;
    int y0;
    int x1;
    int y1;
    int sx;
    int sy;
    int sw;
    int sh;
    const uint8 *rgb565;
    const uint8 *alpha;
    uint32 count;
    uint8 no_scale;

    if (g_native_lua_ctrl->fb == NULL || img == NULL || img->data == NULL ||
        img->w <= 0 || img->h <= 0)
    {
        return;
    }
    sw = img->w;
    sh = img->h;
    if (dw <= 0)
    {
        dw = sw;
    }
    if (dh <= 0)
    {
        dh = sh;
    }
    if (dw <= 0 || dh <= 0)
    {
        return;
    }
    x0 = dx;
    y0 = dy;
    x1 = dx + dw;
    y1 = dy + dh;
    if (x0 < 0)
    {
        x0 = 0;
    }
    if (y0 < 0)
    {
        y0 = 0;
    }
    if (x1 > DIVOOM_GAME_FB_W)
    {
        x1 = DIVOOM_GAME_FB_W;
    }
    if (y1 > DIVOOM_GAME_FB_H)
    {
        y1 = DIVOOM_GAME_FB_H;
    }
    if (x0 >= x1 || y0 >= y1)
    {
        return;
    }
    count = (uint32)sw * (uint32)sh;
    rgb565 = img->data;
    alpha = (img->format == DIVOOM_GAME_IMG_CACHE_FMT_RGB565A8) ?
        (img->data + count * 2U) : NULL;
    no_scale = (sw == dw && sh == dh) ? 1 : 0;

    if (no_scale && img->format == DIVOOM_GAME_IMG_CACHE_FMT_RGB565)
    {
        uint32 row_bytes;

        /* 480x480 JPEG 底图是已解码 RGB565，1:1 绘制时按行复制即可。
         * 避免每帧对 23 万像素重复做除法和 RGB565 往返转换。 */
        row_bytes = (uint32)(x1 - x0) * DIVOOM_APP_RGB565_BYTES_PER_PX;
        for (row = y0; row < y1; row++)
        {
            const uint8 *src;
            uint8 *dst;

            src = rgb565 + (((uint32)(row - dy) * (uint32)sw +
                (uint32)(x0 - dx)) * DIVOOM_APP_RGB565_BYTES_PER_PX);
            dst = g_native_lua_ctrl->fb + (((uint32)row * DIVOOM_GAME_FB_W +
                (uint32)x0) * DIVOOM_APP_RGB565_BYTES_PER_PX);
            memcpy(dst, src, row_bytes);
        }
        return;
    }

    for (row = y0; row < y1; row++)
    {
        sy = no_scale ? (row - dy) : ((row - dy) * sh / dh);
        if (sy < 0)
        {
            sy = 0;
        }
        else if (sy >= sh)
        {
            sy = sh - 1;
        }
        for (col = x0; col < x1; col++)
        {
            uint8 a;
            uint8 r;
            uint8 g;
            uint8 b;
            uint8 *dst;
            uint32 src_idx;
            lv_color_t src_color;

            sx = no_scale ? (col - dx) : ((col - dx) * sw / dw);
            if (sx < 0)
            {
                sx = 0;
            }
            else if (sx >= sw)
            {
                sx = sw - 1;
            }
            src_idx = (uint32)sy * (uint32)sw + (uint32)sx;
            if (src_idx >= count)
            {
                continue;
            }
            a = (alpha != NULL) ? alpha[src_idx] : 255;
            if (a < 8)
            {
                continue;
            }
            divoom_game_rgb565_to_rgb(divoom_game_rgb565_load_le(rgb565 + src_idx * 2U),
                &r, &g, &b);
            dst = g_native_lua_ctrl->fb + ((uint32)row * DIVOOM_GAME_FB_W +
                (uint32)col) * DIVOOM_APP_RGB565_BYTES_PER_PX;
            src_color = lv_color_make(r, g, b);
            if (a >= 250)
            {
                divoom_store_rgb565_le(dst, divoom_game_rgb_to_rgb565(r, g, b));
            }
            else
            {
                /* 全 alpha 混合：dst = src*a + dst*(1-a)，修复图标锯齿/支持柔光 */
                divoom_game_fb_blend_px(col, row, src_color, a);
            }
        }
    }
}

static int divoom_game_lua_draw_webp_dev(lua_State *L, const char *rel,
    int x, int y, int w, int h)
{
    char full_path[DIVOOM_GAME_PATH_MAX];
    divoom_game_img_cache_entry_t img;
    divoom_game_webp_anim_slot_t *slot;
    int frame_time;
    int end_flag;
    uint32 now_tick;

    if (rel == NULL)
    {
        lua_pushboolean(L, 0);
        return 1;
    }
    slot = &g_native_lua_ctrl->webp_anim;
    divoom_game_native_lua_resolve_asset_path(rel, full_path, (int)sizeof(full_path));
    if (!slot->valid || strcmp(slot->path, rel) != 0)
    {
        divoom_game_native_lua_release_webp_anim();
        slot = &g_native_lua_ctrl->webp_anim;
        slot->handle = divoom_libwebp_decode_init(full_path);
        if (slot->handle == NULL)
        {
            divoom_print("[native_lua] webp open failed: %s\n", full_path);
            lua_pushboolean(L, 0);
            return 1;
        }
        (void)divoom_libwebp_decode_get_frame_info(slot->handle, &slot->w, &slot->h);
        if (slot->w <= 0 || slot->h <= 0)
        {
            divoom_game_native_lua_release_webp_anim();
            divoom_print("[native_lua] webp info failed: %s\n", full_path);
            lua_pushboolean(L, 0);
            return 1;
        }
        lv_snprintf(slot->path, sizeof(slot->path), "%s", rel);
        slot->next_frame_tick = 0;
        slot->valid = 1;
        divoom_print("[native_lua] webp anim open path=%s w=%d h=%d\n",
            rel, slot->w, slot->h);
    }

    now_tick = lv_tick_get();
    if (slot->frame_buf == NULL || now_tick >= slot->next_frame_tick)
    {
        if (slot->frame_buf != NULL)
        {
            divoom_system_free(slot->frame_buf);
            slot->frame_buf = NULL;
        }
        frame_time = 0;
        end_flag = 0;
        slot->frame_buf = divoom_libwebp_decode_get_frame_rgb565a8_buffer(
            slot->handle, &frame_time, &end_flag, NULL);
        if (slot->frame_buf == NULL)
        {
            divoom_print("[native_lua] webp frame failed: %s\n", rel);
            lua_pushboolean(L, 0);
            return 1;
        }
        if (frame_time <= 0)
        {
            frame_time = (int)g_native_lua_ctrl->frame_ms;
        }
        slot->next_frame_tick = now_tick + (uint32)frame_time;
    }

    memset(&img, 0, sizeof(img));
    img.data = slot->frame_buf;
    img.w = slot->w;
    img.h = slot->h;
    img.format = DIVOOM_GAME_IMG_CACHE_FMT_RGB565A8;
    divoom_game_fb_blit_cached_scaled(x, y, w, h, &img);
    divoom_game_native_lua_mark_dirty();
    lua_pushboolean(L, 1);
    return 1;
}

static int divoom_game_lua_draw_image(lua_State *L)
{
    const char *rel;
    int lx;
    int ly;
    int lw;
    int lh;
    int dx;
    int dy;
    int dw;
    int dh;
    divoom_game_img_cache_entry_t *img;

    rel = luaL_checkstring(L, 1);
    /* 坐标用 checknumber 截断为 int：游戏实体坐标多为浮点（物理累加/随机/sin 运动），
     * Lua 5.4 的 checkinteger 对带小数的 number 会报 "number has no integer representation" */
    lx = (int)luaL_checknumber(L, 2);
    ly = (int)luaL_checknumber(L, 3);
    lw = 0;
    lh = 0;
    if (lua_gettop(L) >= 5)
    {
        lw = (int)luaL_checknumber(L, 4);
        lh = (int)luaL_checknumber(L, 5);
    }
    img = divoom_game_native_lua_img_cache_get(rel);
    if (img == NULL || img->data == NULL)
    {
        /* 图片缺失/解码失败：返回 false，让 Lua 侧据此决定是否绘制回退色块。 */
        lua_pushboolean(L, 0);
        return 1;
    }
    dx = lx * DIVOOM_GAME_LOGIC_SCALE;
    dy = ly * DIVOOM_GAME_LOGIC_SCALE;
    dw = (lw > 0) ? (lw * DIVOOM_GAME_LOGIC_SCALE) : (img->w * DIVOOM_GAME_LOGIC_SCALE);
    dh = (lh > 0) ? (lh * DIVOOM_GAME_LOGIC_SCALE) : (img->h * DIVOOM_GAME_LOGIC_SCALE);
    divoom_game_fb_blit_cached_scaled(dx, dy, dw, dh, img);
    divoom_game_native_lua_mark_dirty();
    /* 成功绘制：返回 true。 */
    lua_pushboolean(L, 1);
    return 1;
}

/* 5x7 位图字体：0-9 A-Z 空格的 8 行掩码（低 5 位有效） */
static const uint8 divoom_game_font5x7[41][8] =
{
    {0x3E,0x51,0x49,0x45,0x3E,0,0,0}, /* 0 */
    {0x00,0x42,0x7F,0x40,0x00,0,0,0}, /* 1 */
    {0x62,0x51,0x49,0x49,0x46,0,0,0}, /* 2 */
    {0x22,0x41,0x49,0x49,0x36,0,0,0}, /* 3 */
    {0x18,0x14,0x12,0x7F,0x10,0,0,0}, /* 4 */
    {0x27,0x45,0x45,0x45,0x39,0,0,0}, /* 5 */
    {0x3C,0x4A,0x49,0x49,0x30,0,0,0}, /* 6 */
    {0x01,0x71,0x09,0x05,0x03,0,0,0}, /* 7 */
    {0x36,0x49,0x49,0x49,0x36,0,0,0}, /* 8 */
    {0x06,0x49,0x49,0x29,0x1E,0,0,0}, /* 9 */
    {0x7C,0x12,0x11,0x12,0x7C,0,0,0}, /* A */
    {0x7F,0x49,0x49,0x49,0x36,0,0,0}, /* B */
    {0x3E,0x41,0x41,0x41,0x22,0,0,0}, /* C */
    {0x7F,0x41,0x41,0x22,0x1C,0,0,0}, /* D */
    {0x7F,0x49,0x49,0x49,0x41,0,0,0}, /* E */
    {0x7F,0x09,0x09,0x09,0x01,0,0,0}, /* F */
    {0x3E,0x41,0x49,0x49,0x7A,0,0,0}, /* G */
    {0x7F,0x08,0x08,0x08,0x7F,0,0,0}, /* H */
    {0x00,0x41,0x7F,0x41,0x00,0,0,0}, /* I */
    {0x20,0x40,0x41,0x3F,0x01,0,0,0}, /* J */
    {0x7F,0x08,0x14,0x22,0x41,0,0,0}, /* K */
    {0x7F,0x40,0x40,0x40,0x40,0,0,0}, /* L */
    {0x7F,0x02,0x0C,0x02,0x7F,0,0,0}, /* M */
    {0x7F,0x04,0x08,0x10,0x7F,0,0,0}, /* N */
    {0x3E,0x41,0x41,0x41,0x3E,0,0,0}, /* O */
    {0x7F,0x09,0x09,0x09,0x06,0,0,0}, /* P */
    {0x3E,0x41,0x51,0x21,0x5E,0,0,0}, /* Q */
    {0x7F,0x09,0x19,0x29,0x46,0,0,0}, /* R */
    {0x46,0x49,0x49,0x49,0x31,0,0,0}, /* S */
    {0x01,0x01,0x7F,0x01,0x01,0,0,0}, /* T */
    {0x3F,0x40,0x40,0x40,0x3F,0,0,0}, /* U */
    {0x1F,0x20,0x40,0x20,0x1F,0,0,0}, /* V */
    {0x3F,0x40,0x38,0x40,0x3F,0,0,0}, /* W */
    {0x63,0x14,0x08,0x14,0x63,0,0,0}, /* X */
    {0x07,0x08,0x70,0x08,0x07,0,0,0}, /* Y */
    {0x61,0x51,0x49,0x45,0x43,0,0,0}, /* Z */
    {0x00,0x00,0x00,0x00,0x00,0,0,0}, /* space */
    {0x00,0x36,0x36,0x00,0x00,0,0,0}, /* : */
    {0x08,0x08,0x08,0x08,0x08,0,0,0}, /* - */
    {0x08,0x1C,0x3E,0x1C,0x08,0,0,0}, /* ! */
    {0x00,0x22,0x14,0x08,0x14,0,0,0}, /* x */
};

static int divoom_game_font_index(char c)
{
    if (c >= '0' && c <= '9')
    {
        return c - '0';
    }
    if (c >= 'A' && c <= 'Z')
    {
        return 10 + (c - 'A');
    }
    if (c >= 'a' && c <= 'z')
    {
        return 10 + (c - 'a');
    }
    if (c == ' ')
    {
        return 36;
    }
    if (c == ':')
    {
        return 37;
    }
    if (c == '-')
    {
        return 38;
    }
    if (c == '!')
    {
        return 39;
    }
    if (c == 'x' || c == 'X')
    {
        return 40;
    }
    return 36;
}

static void divoom_game_fb_draw_char(int dx, int dy, char c, lv_color_t color, int scale)
{
    const uint8 *glyph;
    int row;
    int col;
    int idx;

    idx = divoom_game_font_index(c);
    if (idx < 0 || idx > 40)
    {
        return;
    }
    glyph = divoom_game_font5x7[idx];
    for (row = 0; row < 7; row++)
    {
        uint8 bits;

        bits = glyph[row];
        for (col = 0; col < 5; col++)
        {
            if (bits & (1 << (4 - col)))
            {
                divoom_game_fb_fill_rect(dx + col * scale, dy + row * scale, scale, scale, color);
            }
        }
    }
}

static int divoom_game_lua_draw_text(lua_State *L)
{
    const char *text;
    int lx;
    int ly;
    lv_color_t c;
    int fsize;
    int scale;
    int px;
    int py;
    void *font;
    lv_draw_label_dsc_t dsc;

    text = luaL_checkstring(L, 1);
    lx = (int)luaL_checknumber(L, 2);
    ly = (int)luaL_checknumber(L, 3);
    c = divoom_game_parse_color(L, 4);
    fsize = (int)luaL_optnumber(L, 5, 0); /* 第 5 参为逻辑字号，缺省用默认 */
    scale = DIVOOM_GAME_LOGIC_SCALE;
    if (fsize <= 0)
    {
        fsize = DIVOOM_GAME_TEXT_DEFAULT_SIZE;
    }
    px = lx * scale;
    py = ly * scale;

    /* 文字必须使用系统字体：divoom_menu_control_get_font_info 返回 lv_font_t*。
     * 字号按 fb 像素 = 逻辑字号 * scale，支持中文。 */
    font = divoom_menu_control_get_font_info(fsize * scale, DIVOOM_MENU_CONTROL_ID_ASTROTOO_GAMES);
    if (font != NULL)
    {
#if DIVOOM_GAME_MANUAL_FONT_DRAW
        if (divoom_game_native_lua_draw_text_to_fb(text, px, py,
                DIVOOM_GAME_FB_W - px, c, (const lv_font_t *)font))
        {
            return 0;
        }
#if !DIVOOM_LVGL9_IS_SIM
        font = NULL;
#endif
#endif
        if (font != NULL)
        {
            lv_draw_label_dsc_init(&dsc);
            dsc.color = c;
            dsc.font = (const lv_font_t *)font;
            dsc.opa = LV_OPA_COVER;
            divoom_game_native_lua_canvas_draw_text(g_native_lua_ctrl->canvas, px, py,
                DIVOOM_GAME_FB_W - px, &dsc, (char *)text);
            return 0;
        }
    }

    return 0;
}

/* i18n：game.tr(key) -> 当前游戏语言显示串（独立于系统语言池） */
static int divoom_game_lua_tr(lua_State *L)
{
    const char *key;
    const char *disp;

    key = luaL_checkstring(L, 1);
    disp = divoom_game_lang_get_value(key);
    /* 未命中（NULL/空串）回退英文 key，避免文字消失 */
    lua_pushstring(L, (disp != NULL && disp[0] != 0) ? disp : key);
    return 1;
}

/* game.text_width(text, size) -> 逻辑像素宽度（用真实 TTF 字体测量，支持中英混排居中） */
static int divoom_game_lua_text_width(lua_State *L)
{
    const char *text;
    int fsize;
    int scale;
    void *font;
    int w_fb;

    text = luaL_checkstring(L, 1);
    fsize = (int)luaL_optnumber(L, 2, 0);
    scale = DIVOOM_GAME_LOGIC_SCALE;
    if (fsize <= 0)
    {
        fsize = DIVOOM_GAME_TEXT_DEFAULT_SIZE;
    }
    font = divoom_menu_control_get_font_info(fsize * scale, DIVOOM_MENU_CONTROL_ID_ASTROTOO_GAMES);
    if (font != NULL)
    {
        w_fb = (int)lv_txt_get_width(text, (uint32_t)strlen(text), (const lv_font_t *)font, 0);
    }
    else
    {
        w_fb = (int)strlen(text) * 6 * scale;
    }
    /* 返回逻辑坐标宽度（render.lua 用逻辑 240 坐标系做居中） */
    lua_pushnumber(L, (double)w_fb / (double)scale);
    return 1;
}

/* ============================================================================
 * V5 设备空间（绝对 480）绘制原语：不做 240->480 缩放。
 * 用于 Astro Pet V5「整屏 PNG + 动态文字」模型，坐标/字号即设备像素。
 * ==========================================================================*/

/* game.draw_image_dev(path, x, y[, w, h]) -> bool
 * 在设备坐标按 1:1（默认原生尺寸）绘制 PNG；缺图返回 false。 */
static int divoom_game_lua_draw_image_dev(lua_State *L)
{
    const char *rel;
    int x;
    int y;
    int w;
    int h;
    divoom_game_img_cache_entry_t *img;

    rel = luaL_checkstring(L, 1);
    x = (int)luaL_checknumber(L, 2);
    y = (int)luaL_checknumber(L, 3);
    w = 0;
    h = 0;
    if (lua_gettop(L) >= 5)
    {
        w = (int)luaL_checknumber(L, 4);
        h = (int)luaL_checknumber(L, 5);
    }
    if (divoom_game_native_lua_path_has_ext(rel, ".webp"))
    {
        return divoom_game_lua_draw_webp_dev(L, rel, x, y, w, h);
    }
    img = divoom_game_native_lua_img_cache_get(rel);
    if (img == NULL || img->data == NULL)
    {
        lua_pushboolean(L, 0);
        return 1;
    }
    if (w <= 0)
    {
        w = img->w;
    }
    if (h <= 0)
    {
        h = img->h;
    }
    divoom_game_fb_blit_cached_scaled(x, y, w, h, img);
    divoom_game_native_lua_mark_dirty();
    lua_pushboolean(L, 1);
    return 1;
}

/* game.draw_text_dev(text, x, y, color, font_px)
 * 设备像素坐标与字号，不做缩放；文字必须使用系统字体。 */
static int divoom_game_lua_draw_text_dev(lua_State *L)
{
    const char *text;
    int x;
    int y;
    lv_color_t c;
    int fsize;
    void *font;
    lv_draw_label_dsc_t dsc;

    text = luaL_checkstring(L, 1);
    x = (int)luaL_checknumber(L, 2);
    y = (int)luaL_checknumber(L, 3);
    c = divoom_game_parse_color(L, 4);
    fsize = (int)luaL_optnumber(L, 5, 0);
    if (fsize <= 0)
    {
        fsize = DIVOOM_GAME_TEXT_DEFAULT_SIZE * DIVOOM_GAME_LOGIC_SCALE;
    }

    font = divoom_menu_control_get_font_info(fsize, DIVOOM_MENU_CONTROL_ID_ASTROTOO_GAMES);
    if (font != NULL)
    {
#if DIVOOM_GAME_MANUAL_FONT_DRAW
        if (divoom_game_native_lua_draw_text_to_fb(text, x, y,
                DIVOOM_GAME_FB_W - x, c, (const lv_font_t *)font))
        {
            return 0;
        }
#if !DIVOOM_LVGL9_IS_SIM
        font = NULL;
#endif
#endif
        if (font != NULL)
        {
            lv_draw_label_dsc_init(&dsc);
            dsc.color = c;
            dsc.font = (const lv_font_t *)font;
            dsc.opa = LV_OPA_COVER;
            divoom_game_native_lua_canvas_draw_text(g_native_lua_ctrl->canvas, x, y,
                DIVOOM_GAME_FB_W - x, &dsc, (char *)text);
            return 0;
        }
    }

    return 0;
}

/* game.text_width_dev(text, font_px) -> 设备像素宽度（供 align center/right） */
static int divoom_game_lua_text_width_dev(lua_State *L)
{
    const char *text;
    int fsize;
    void *font;
    int w_fb;

    text = luaL_checkstring(L, 1);
    fsize = (int)luaL_optnumber(L, 2, 0);
    if (fsize <= 0)
    {
        fsize = DIVOOM_GAME_TEXT_DEFAULT_SIZE * DIVOOM_GAME_LOGIC_SCALE;
    }
    font = divoom_menu_control_get_font_info(fsize, DIVOOM_MENU_CONTROL_ID_ASTROTOO_GAMES);
    if (font != NULL)
    {
        w_fb = (int)lv_txt_get_width(text, (uint32_t)strlen(text), (const lv_font_t *)font, 0);
    }
    else
    {
        w_fb = (int)strlen(text) * 6 * DIVOOM_GAME_LOGIC_SCALE;
    }
    lua_pushnumber(L, (double)w_fb);
    return 1;
}

/* game.label_set(id, text, x, y, color, font_px[, max_w]) -> bool
 * 少量稳定文字使用 LVGL label overlay；id 相同则复用对象，页面切换需 label_clear_all。 */
static int divoom_game_lua_label_set(lua_State *L)
{
    const char *id;
    const char *text;
    int x;
    int y;
    lv_color_t c;
    int fsize;
    int max_w;
    void *font;
    divoom_game_label_slot_t *slot;

    id = luaL_checkstring(L, 1);
    text = luaL_checkstring(L, 2);
    x = (int)luaL_checknumber(L, 3);
    y = (int)luaL_checknumber(L, 4);
    c = divoom_game_parse_color(L, 5);
    fsize = (int)luaL_optnumber(L, 6, DIVOOM_GAME_TEXT_DEFAULT_SIZE *
        DIVOOM_GAME_LOGIC_SCALE);
    max_w = (int)luaL_optnumber(L, 7, DIVOOM_GAME_FB_W - x);
    if (fsize <= 0)
    {
        fsize = DIVOOM_GAME_TEXT_DEFAULT_SIZE * DIVOOM_GAME_LOGIC_SCALE;
    }
    if (max_w <= 0)
    {
        max_w = DIVOOM_GAME_FB_W - x;
    }
    font = divoom_menu_control_get_font_info(fsize, DIVOOM_MENU_CONTROL_ID_ASTROTOO_GAMES);
    slot = divoom_game_native_lua_label_get_or_create(id);
    if (slot == NULL || slot->obj == NULL || font == NULL)
    {
        lua_pushboolean(L, 0);
        return 1;
    }
    lv_obj_set_pos(slot->obj, x, y);
    lv_obj_set_width(slot->obj, max_w);
    lv_obj_set_style_text_color(slot->obj, c, 0);
    lv_obj_set_style_text_font(slot->obj, (const lv_font_t *)font, 0);
    lv_label_set_text(slot->obj, text);
    lv_obj_clear_flag(slot->obj, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(slot->obj);
    lua_pushboolean(L, 1);
    return 1;
}

/* game.label_hide(id) —— 隐藏但保留对象，适合同一页面内临时状态切换。 */
static int divoom_game_lua_label_hide(lua_State *L)
{
    const char *id;
    divoom_game_label_slot_t *slot;

    id = luaL_checkstring(L, 1);
    slot = divoom_game_native_lua_label_find(id);
    if (slot != NULL && slot->obj != NULL)
    {
        lv_obj_add_flag(slot->obj, LV_OBJ_FLAG_HIDDEN);
    }
    return 0;
}

/* game.label_clear_all() —— 删除所有 overlay label，页面切换/游戏退出前必须可重复调用。 */
static int divoom_game_lua_label_clear_all(lua_State *L)
{
    (void)L;
    divoom_game_native_lua_label_clear_all();
    return 0;
}

/* game.free_images() —— 释放图片/WebP 播放缓存（切页时调用，满足按需加载释放规则） */
static int divoom_game_lua_free_images(lua_State *L)
{
    (void)L;
    divoom_game_native_lua_free_img_cache();
    divoom_game_native_lua_release_webp_anim();
    return 0;
}

/* game.release_image(path) —— 主动释放指定图片；WebP 动画则释放当前播放句柄。 */
static int divoom_game_lua_release_image(lua_State *L)
{
    const char *rel;
    int i;

    rel = luaL_checkstring(L, 1);
    if (divoom_game_native_lua_path_has_ext(rel, ".webp") &&
        g_native_lua_ctrl->webp_anim.valid &&
        strcmp(g_native_lua_ctrl->webp_anim.path, rel) == 0)
    {
        divoom_game_native_lua_release_webp_anim();
        lua_pushboolean(L, 1);
        return 1;
    }
    for (i = 0; i < DIVOOM_GAME_IMG_CACHE_MAX; i++)
    {
        divoom_game_img_cache_entry_t *slot;

        slot = &g_native_lua_ctrl->img_cache[i];
        if (slot->valid && !slot->external_data && strcmp(slot->path, rel) == 0)
        {
            divoom_game_native_lua_free_img_cache_slot(slot);
            lua_pushboolean(L, 1);
            return 1;
        }
    }
    lua_pushboolean(L, 0);
    return 1;
}

/*
 * game.image_cache_scene_prune()：状态切换时释放上一场景自有图片。
 * external_data 指向 base_rgb565_buf，保留到新底图真正开始解码，避免低内存
 * skip 先破坏当前可用底图；PNG owned buffer 和失败/重试 slot 均在此释放。
 */
static int divoom_game_lua_image_cache_scene_prune(lua_State *L)
{
    uint32 released_bytes;
    uint32 external_bytes;
    int before_count;
    int released_count;
    int i;

    (void)L;
    released_bytes = 0;
    released_count = 0;
    before_count = divoom_game_native_lua_get_img_cache_count();
    external_bytes = g_native_lua_ctrl->img_cache_external_bytes;
    divoom_game_native_lua_log_mem_stage("scene cache prune before");
    for (i = 0; i < DIVOOM_GAME_IMG_CACHE_MAX; i++)
    {
        divoom_game_img_cache_entry_t *slot;

        slot = &g_native_lua_ctrl->img_cache[i];
        if (!slot->valid || slot->external_data)
        {
            continue;
        }
        released_bytes += slot->byte_size;
        released_count++;
        divoom_game_native_lua_free_img_cache_slot(slot);
    }
    divoom_print("[native_lua] scene cache prune released_count=%d released_bytes=%u before_count=%d after_count=%d cache_owned_bytes=%u cache_external_bytes=%u external_before=%u\n",
        released_count, released_bytes, before_count,
        divoom_game_native_lua_get_img_cache_count(),
        g_native_lua_ctrl->img_cache_owned_bytes,
        g_native_lua_ctrl->img_cache_external_bytes, external_bytes);
    divoom_game_native_lua_log_mem_stage("scene cache prune after");
    return 0;
}

/* JSON 和共享计数没有整数类型；入口恢复可无损表示的整数，避免 UI 拼接出小数尾巴。
 * 转换由 Lua 检查范围，真实小数及超范围数仍保留浮点精度，不改存档和物理计算。 */
static void divoom_game_native_lua_push_number(lua_State *l, double value)
{
    lua_Integer integer_value;
    int is_integer;

    lua_pushnumber(l, value);
    integer_value = lua_tointegerx(l, -1, &is_integer);
    if (is_integer && (lua_Number)integer_value == (lua_Number)value)
    {
        lua_pop(l, 1);
        lua_pushinteger(l, integer_value);
    }
}

static void divoom_game_lua_push_json(lua_State *L, divoom_cJSON *item)
{
    divoom_cJSON *child;
    int t;
    int i;

    if (item == NULL)
    {
        lua_pushnil(L);
        return;
    }
    t = item->type & 0xFF;
    if (t == divoom_cJSON_Number)
    {
        divoom_game_native_lua_push_number(L, item->valuedouble);
    }
    else if (t == divoom_cJSON_String)
    {
        lua_pushstring(L, item->valuestring ? item->valuestring : "");
    }
    else if (t == divoom_cJSON_True)
    {
        lua_pushboolean(L, 1);
    }
    else if (t == divoom_cJSON_False)
    {
        lua_pushboolean(L, 0);
    }
    else if (t == divoom_cJSON_Array)
    {
        lua_newtable(L);
        i = 1;
        for (child = item->child; child != NULL; child = child->next)
        {
            divoom_game_lua_push_json(L, child);
            lua_rawseti(L, -2, i);
            i++;
        }
    }
    else if (t == divoom_cJSON_Object)
    {
        lua_newtable(L);
        for (child = item->child; child != NULL; child = child->next)
        {
            if (child->string)
            {
                divoom_game_lua_push_json(L, child);
                lua_setfield(L, -2, child->string);
            }
        }
    }
    else
    {
        lua_pushnil(L);
    }
}

static divoom_cJSON *divoom_game_lua_to_json(lua_State *L, int index, int depth)
{
    divoom_cJSON *node;
    divoom_cJSON *child;
    size_t len;
    int t;
    int abs_index;
    size_t i;

    if (depth > 16)
    {
        return divoom_cJSON_CreateNull();
    }

    abs_index = lua_absindex(L, index);
    t = lua_type(L, abs_index);
    if (t == LUA_TNUMBER)
    {
        return divoom_cJSON_CreateNumber(
            divoom_game_lua_safe_number(lua_tonumber(L, abs_index)));
    }
    if (t == LUA_TBOOLEAN)
    {
        return divoom_cJSON_CreateBool(lua_toboolean(L, abs_index));
    }
    if (t == LUA_TSTRING)
    {
        return divoom_cJSON_CreateString(lua_tostring(L, abs_index));
    }
    if (t != LUA_TTABLE)
    {
        return divoom_cJSON_CreateNull();
    }

    len = lua_rawlen(L, abs_index);
    if (len > 0)
    {
        node = divoom_cJSON_CreateArray();
        if (node == NULL)
        {
            return NULL;
        }
        for (i = 1; i <= len; i++)
        {
            lua_rawgeti(L, abs_index, (lua_Integer)i);
            child = divoom_game_lua_to_json(L, -1, depth + 1);
            lua_pop(L, 1);
            if (child == NULL)
            {
                child = divoom_cJSON_CreateNull();
            }
            divoom_cJSON_AddItemToArray(node, child);
        }
        return node;
    }

    node = divoom_cJSON_CreateObject();
    if (node == NULL)
    {
        return NULL;
    }
    lua_pushnil(L);
    while (lua_next(L, abs_index) != 0)
    {
        const char *key;

        key = lua_tostring(L, -2);
        if (key != NULL)
        {
            child = divoom_game_lua_to_json(L, -1, depth + 1);
            if (child != NULL)
            {
                divoom_cJSON_AddItemToObject(node, key, child);
            }
        }
        lua_pop(L, 1);
    }
    return node;
}

static void divoom_game_native_lua_save_defaults(lua_State *L)
{
    lua_newtable(L);
    lua_pushinteger(L, 0);
    lua_setfield(L, -2, "best_score");
    lua_pushinteger(L, 0);
    lua_setfield(L, -2, "total_stars");
    lua_pushinteger(L, 0);
    lua_setfield(L, -2, "total_distance");
    lua_pushinteger(L, 0);
    lua_setfield(L, -2, "play_count");
    lua_pushinteger(L, 0);
    lua_setfield(L, -2, "total_play_time_sec");
    lua_pushinteger(L, 0);
    lua_setfield(L, -2, "last_played_at");
}

static int divoom_game_lua_read_save(lua_State *L)
{
    char save_path[DIVOOM_GAME_SAVE_PATH_MAX];
    char *buf;
    divoom_cJSON *root;
    int file_len;

    lv_snprintf(save_path, sizeof(save_path), "%s/%s/%s.json",
        DIVOOM_GAMES_ROOT_USER, DIVOOM_GAMES_SAVES_SUB, g_native_lua_ctrl->entry.id);
    file_len = 0;
    buf = divoom_game_native_lua_read_file(save_path, &file_len);
    if (buf == NULL)
    {
        divoom_print("[native_lua] read_save missing id=%s path=%s\n",
            g_native_lua_ctrl->entry.id, save_path);
        divoom_game_native_lua_save_defaults(L);
        return 1;
    }
    root = divoom_cJSON_Parse(buf);
    divoom_game_native_lua_free_file_buf(buf);
    if (root == NULL)
    {
        divoom_game_lua_log_json_error("read_save", buf,
            divoom_cJSON_GetErrorPtr(), file_len);
        divoom_print("[native_lua] read_save parse failed id=%s path=%s len=%d\n",
            g_native_lua_ctrl->entry.id, save_path, file_len);
        divoom_game_native_lua_save_defaults(L);
        return 1;
    }
    divoom_print("[native_lua] read_save ok id=%s path=%s len=%d\n",
        g_native_lua_ctrl->entry.id, save_path, file_len);
    divoom_game_lua_push_json(L, root);
    divoom_cJSON_Delete(root);
    return 1;
}

static int divoom_game_lua_write_save(lua_State *L)
{
    char save_path[DIVOOM_GAME_SAVE_PATH_MAX];
    divoom_cJSON *root;
    char *json_str;
    uint32 json_len;

    luaL_checktype(L, 1, LUA_TTABLE);
    root = divoom_game_lua_to_json(L, 1, 0);
    if (root == NULL)
    {
        return 0;
    }
    json_str = divoom_cJSON_PrintUnformatted(root);
    divoom_cJSON_Delete(root);
    if (json_str == NULL)
    {
        return 0;
    }
    json_len = (uint32)strlen(json_str);
    lv_snprintf(save_path, sizeof(save_path), "%s/%s/%s.json",
        DIVOOM_GAMES_ROOT_USER, DIVOOM_GAMES_SAVES_SUB, g_native_lua_ctrl->entry.id);
    divoom_game_runtime_ensure_dirs();
    (void)divoom_game_native_lua_write_file(save_path, (uint8 *)json_str, json_len);
    divoom_system_free(json_str);
    return 0;
}

static int divoom_game_now_day_id(void)
{
    time_t t;
    struct tm tm_local;

    t = time(NULL);
#if defined(_WIN32) || defined(_WIN64)
    localtime_s(&tm_local, &t);
#else
    localtime_r(&t, &tm_local);
#endif
    return (tm_local.tm_year + 1900) * 10000 + (tm_local.tm_mon + 1) * 100 + tm_local.tm_mday;
}

static int divoom_game_counter_name_is_today(const char *name)
{
    size_t n;

    if (name == NULL)
    {
        return 0;
    }
    n = strlen(name);
    if (n >= 6 && strcmp(name + n - 6, "_today") == 0)
    {
        return 1;
    }
    return 0;
}

static divoom_cJSON *divoom_game_shared_counters_load(void)
{
    char *buf;
    divoom_cJSON *root;

    buf = divoom_game_native_lua_read_file(DIVOOM_GAME_SHARED_COUNTERS_FILE, NULL);
    if (buf == NULL)
    {
        root = divoom_cJSON_CreateObject();
        if (root != NULL)
        {
            divoom_cJSON_AddNumberToObject(root, DIVOOM_GAME_SHARED_DAY_FIELD, 0);
        }
        return root;
    }
    root = divoom_cJSON_Parse(buf);
    divoom_game_native_lua_free_file_buf(buf);
    if (root == NULL)
    {
        root = divoom_cJSON_CreateObject();
        if (root != NULL)
        {
            divoom_cJSON_AddNumberToObject(root, DIVOOM_GAME_SHARED_DAY_FIELD, 0);
        }
    }
    return root;
}

static void divoom_game_shared_counters_save(divoom_cJSON *root)
{
    char *json_str;

    if (root == NULL)
    {
        return;
    }
    divoom_game_runtime_ensure_dirs();
    json_str = divoom_cJSON_PrintUnformatted(root);
    if (json_str == NULL)
    {
        return;
    }
    (void)divoom_game_native_lua_write_file(DIVOOM_GAME_SHARED_COUNTERS_FILE,
        (uint8 *)json_str, (uint32)strlen(json_str));
    divoom_system_free(json_str);
}

static void divoom_game_shared_reset_today(divoom_cJSON *root, int day_id)
{
    divoom_cJSON *child;
    char names[64][128];
    int n;
    int i;

    if (root == NULL)
    {
        return;
    }
    n = 0;
    for (child = root->child; child != NULL && n < 64; child = child->next)
    {
        if (child->string != NULL
            && strcmp(child->string, DIVOOM_GAME_SHARED_DAY_FIELD) != 0
            && divoom_game_counter_name_is_today(child->string))
        {
            lv_snprintf(names[n], sizeof(names[n]), "%s", child->string);
            n++;
        }
    }
    for (i = 0; i < n; i++)
    {
        divoom_cJSON_DeleteItemFromObject(root, names[i]);
    }
    divoom_cJSON_DeleteItemFromObject(root, DIVOOM_GAME_SHARED_DAY_FIELD);
    divoom_cJSON_AddNumberToObject(root, DIVOOM_GAME_SHARED_DAY_FIELD, day_id);
}

static int divoom_game_shared_ensure_day(divoom_cJSON *root)
{
    int today;
    int stored;
    divoom_cJSON *item;

    if (root == NULL)
    {
        return 0;
    }
    today = divoom_game_now_day_id();
    item = divoom_cJSON_GetObjectItem(root, DIVOOM_GAME_SHARED_DAY_FIELD);
    stored = 0;
    if (item != NULL && (item->type & 0xFF) == divoom_cJSON_Number)
    {
        stored = (int)item->valuedouble;
    }
    if (stored != today)
    {
        divoom_game_shared_reset_today(root, today);
        return 1;
    }
    return 0;
}

static double divoom_game_shared_get(divoom_cJSON *root, const char *name)
{
    divoom_cJSON *item;

    if (root == NULL || name == NULL)
    {
        return 0;
    }
    item = divoom_cJSON_GetObjectItem(root, name);
    if (item != NULL && (item->type & 0xFF) == divoom_cJSON_Number)
    {
        return item->valuedouble;
    }
    return 0;
}

static int divoom_game_lua_now_day(lua_State *L)
{
    lua_pushinteger(L, divoom_game_now_day_id());
    return 1;
}

static int divoom_game_lua_now(lua_State *L)
{
    lua_pushinteger(L, (lua_Integer)time(NULL));
    return 1;
}

static int divoom_game_lua_save_mtime(lua_State *L)
{
    char save_path[DIVOOM_GAME_SAVE_PATH_MAX];
    divoom_port_stat_t st;

    lv_snprintf(save_path, sizeof(save_path), "%s/%s/%s.json",
        DIVOOM_GAMES_ROOT_USER, DIVOOM_GAMES_SAVES_SUB, g_native_lua_ctrl->entry.id);
    if (divoom_port_stat(save_path, &st) == 0)
    {
        lua_pushinteger(L, (lua_Integer)st.mtime);
    }
    else
    {
        lua_pushinteger(L, 0);
    }
    return 1;
}

static int divoom_game_lua_get_counter(lua_State *L)
{
    const char *name;
    divoom_cJSON *root;
    int day_changed;

    name = luaL_checkstring(L, 1);
    root = divoom_game_shared_counters_load();
    if (root == NULL)
    {
        lua_pushinteger(L, 0);
        return 1;
    }
    day_changed = divoom_game_shared_ensure_day(root);
    if (day_changed)
    {
        divoom_game_shared_counters_save(root);
    }
    divoom_game_native_lua_push_number(L, divoom_game_shared_get(root, name));
    divoom_cJSON_Delete(root);
    return 1;
}

static int divoom_game_lua_add_counter(lua_State *L)
{
    const char *name;
    double delta;
    divoom_cJSON *root;
    double cur;

    name = luaL_checkstring(L, 1);
    delta = luaL_checknumber(L, 2);
    root = divoom_game_shared_counters_load();
    if (root == NULL)
    {
        return 0;
    }
    divoom_game_shared_ensure_day(root);
    cur = divoom_game_shared_get(root, name);
    divoom_cJSON_DeleteItemFromObject(root, name);
    divoom_cJSON_AddNumberToObject(root, name, cur + delta);
    divoom_game_shared_counters_save(root);
    divoom_cJSON_Delete(root);
    return 0;
}

static int divoom_game_lua_play_sfx(lua_State *L)
{
#if !DIVOOM_GAME_NATIVE_LUA_AUDIO_ENABLE
    (void)luaL_checkstring(L, 1);
    return 0;
#else
    char full_path[DIVOOM_GAME_PATH_MAX];
    const char *name;

    name = luaL_checkstring(L, 1);
    if (strstr(name, "assets/") == name)
    {
        lv_snprintf(full_path, sizeof(full_path), "%s/%s",
            g_native_lua_ctrl->entry.dir_abs, name);
    }
    else if (strstr(name, ".wav") != NULL)
    {
        lv_snprintf(full_path, sizeof(full_path), "%s/assets/audio/%s",
            g_native_lua_ctrl->entry.dir_abs, name);
    }
    else
    {
        lv_snprintf(full_path, sizeof(full_path), "%s/assets/audio/%s.wav",
            g_native_lua_ctrl->entry.dir_abs, name);
    }
    divoom_local_music_ctrl_tone_play(full_path, 80, NULL, 0);
    return 0;
#endif
}

static int divoom_game_lua_stop_sfx(lua_State *L)
{
    (void)L;
#if DIVOOM_GAME_NATIVE_LUA_AUDIO_ENABLE
    divoom_local_music_ctrl_stop_tone();
#endif
    return 0;
}

static int divoom_game_native_lua_load_modules(lua_State *L, const char *game_dir)
{
    char json_path[DIVOOM_GAME_PATH_MAX];
    char *buf;
    divoom_cJSON *root;
    divoom_cJSON *mods;
    int n;
    int i;

    lv_snprintf(json_path, sizeof(json_path), "%s/game.json", game_dir);
    buf = divoom_game_native_lua_read_file(json_path, NULL);
    if (buf == NULL)
    {
        return 0;
    }
    root = divoom_cJSON_Parse(buf);
    divoom_game_native_lua_free_file_buf(buf);
    if (root == NULL)
    {
        return 0;
    }
    mods = divoom_cJSON_GetObjectItem(root, "modules");
    if (mods == NULL || (mods->type & 0xFF) != divoom_cJSON_Array)
    {
        divoom_cJSON_Delete(root);
        return 0;
    }
    n = divoom_cJSON_GetArraySize(mods);
    for (i = 0; i < n; i++)
    {
        divoom_cJSON *item;
        const char *mod;
        char script_path[DIVOOM_GAME_PATH_MAX];

        item = divoom_cJSON_GetArrayItem(mods, i);
        if (item == NULL || item->valuestring == NULL)
        {
            continue;
        }
        mod = item->valuestring;
        lv_snprintf(script_path, sizeof(script_path), "%s/%s", game_dir, mod);
        if (divoom_game_native_lua_load_script(L, script_path) != 0)
        {
            divoom_cJSON_Delete(root);
            return -1;
        }
    }
    divoom_cJSON_Delete(root);
    return 0;
}

static void divoom_game_native_lua_register_api(lua_State *L)
{
    lua_newtable(L);
    lua_pushinteger(L, DIVOOM_GAME_FB_W);
    lua_setfield(L, -2, "w");
    lua_pushinteger(L, DIVOOM_GAME_FB_H);
    lua_setfield(L, -2, "h");
    lua_pushinteger(L, DIVOOM_GAME_LOGIC_W);
    lua_setfield(L, -2, "logic_w");
    lua_pushinteger(L, DIVOOM_GAME_LOGIC_H);
    lua_setfield(L, -2, "logic_h");
    lua_pushinteger(L, DIVOOM_GAME_LOGIC_SCALE);
    lua_setfield(L, -2, "scale");
    lua_pushcfunction(L, divoom_game_lua_clear);
    lua_setfield(L, -2, "clear");
    lua_pushcfunction(L, divoom_game_lua_fill_rect);
    lua_setfield(L, -2, "fill_rect");
    lua_pushcfunction(L, divoom_game_lua_draw_rect);
    lua_setfield(L, -2, "draw_rect");
    lua_pushcfunction(L, divoom_game_lua_fill_round_rect);
    lua_setfield(L, -2, "fill_round_rect");
    lua_pushcfunction(L, divoom_game_lua_draw_round_rect);
    lua_setfield(L, -2, "draw_round_rect");
    lua_pushcfunction(L, divoom_game_lua_fill_grad_rect);
    lua_setfield(L, -2, "fill_grad_rect");
    lua_pushcfunction(L, divoom_game_lua_draw_image);
    lua_setfield(L, -2, "draw_image");
    lua_pushcfunction(L, divoom_game_lua_draw_text);
    lua_setfield(L, -2, "draw_text");
    lua_pushcfunction(L, divoom_game_lua_draw_image_dev);
    lua_setfield(L, -2, "draw_image_dev");
    lua_pushcfunction(L, divoom_game_lua_draw_text_dev);
    lua_setfield(L, -2, "draw_text_dev");
    lua_pushcfunction(L, divoom_game_lua_text_width_dev);
    lua_setfield(L, -2, "text_width_dev");
    lua_pushcfunction(L, divoom_game_lua_label_set);
    lua_setfield(L, -2, "label_set");
    lua_pushcfunction(L, divoom_game_lua_label_hide);
    lua_setfield(L, -2, "label_hide");
    lua_pushcfunction(L, divoom_game_lua_label_clear_all);
    lua_setfield(L, -2, "label_clear_all");
    lua_pushcfunction(L, divoom_game_lua_free_images);
    lua_setfield(L, -2, "free_images");
    lua_pushcfunction(L, divoom_game_lua_release_image);
    lua_setfield(L, -2, "release_image");
    lua_pushcfunction(L, divoom_game_lua_image_cache_scene_prune);
    lua_setfield(L, -2, "image_cache_scene_prune");
    lua_pushcfunction(L, divoom_game_lua_tr);
    lua_setfield(L, -2, "tr");
    lua_pushcfunction(L, divoom_game_lua_text_width);
    lua_setfield(L, -2, "text_width");
    lua_pushcfunction(L, divoom_game_lua_read_save);
    lua_setfield(L, -2, "read_save");
    lua_pushcfunction(L, divoom_game_lua_write_save);
    lua_setfield(L, -2, "write_save");
    lua_pushcfunction(L, divoom_game_lua_play_sfx);
    lua_setfield(L, -2, "play_sfx");
    lua_pushcfunction(L, divoom_game_lua_stop_sfx);
    lua_setfield(L, -2, "stop_sfx");
    lua_pushcfunction(L, divoom_game_lua_input_down);
    lua_setfield(L, -2, "input_down");
    lua_pushcfunction(L, divoom_game_lua_input_pressed);
    lua_setfield(L, -2, "input_pressed");
    lua_pushcfunction(L, divoom_game_lua_quit);
    lua_setfield(L, -2, "quit");
    lua_pushcfunction(L, divoom_game_lua_consume_back);
    lua_setfield(L, -2, "consume_back");
    lua_pushcfunction(L, divoom_game_lua_log);
    lua_setfield(L, -2, "log");
    lua_pushcfunction(L, divoom_game_lua_log_mem);
    lua_setfield(L, -2, "log_mem");
    lua_pushcfunction(L, divoom_game_lua_getenv);
    lua_setfield(L, -2, "getenv");
    lua_pushcfunction(L, divoom_game_lua_now_day);
    lua_setfield(L, -2, "now_day");
    lua_pushcfunction(L, divoom_game_lua_now);
    lua_setfield(L, -2, "now");
    lua_pushcfunction(L, divoom_game_lua_save_mtime);
    lua_setfield(L, -2, "save_mtime");
    lua_pushcfunction(L, divoom_game_lua_get_counter);
    lua_setfield(L, -2, "get_counter");
    lua_pushcfunction(L, divoom_game_lua_add_counter);
    lua_setfield(L, -2, "add_counter");
    lua_setglobal(L, "game");
}

static int divoom_game_native_lua_load_script(lua_State *L, const char *script_path)
{
    char *script_buf;
    int file_len;
    int load_ret;

    script_buf = divoom_game_native_lua_read_file(script_path, &file_len);
    if (script_buf == NULL || file_len <= 0)
    {
        divoom_print("[native_lua] read failed: %s\n", script_path);
        return -1;
    }

    load_ret = luaL_loadbuffer(L, script_buf, (size_t)file_len, script_path);
    divoom_game_native_lua_free_file_buf(script_buf);
    if (load_ret != LUA_OK)
    {
        divoom_print("[native_lua] load %s: %s\n", script_path, lua_tostring(L, -1));
        lua_pop(L, 1);
        return -1;
    }

    if (lua_pcall(L, 0, 0, 0) != LUA_OK)
    {
        divoom_print("[native_lua] run %s: %s\n", script_path, lua_tostring(L, -1));
        lua_pop(L, 1);
        return -1;
    }

    return 0;
}

static void divoom_game_native_lua_open_safe_libs(lua_State *L)
{
    luaL_requiref(L, "_G", luaopen_base, 0);
    lua_pop(L, 1);
    luaL_requiref(L, LUA_TABLIBNAME, luaopen_table, 1);
    lua_pop(L, 1);
    luaL_requiref(L, LUA_STRLIBNAME, luaopen_string, 1);
    lua_pop(L, 1);
    luaL_requiref(L, LUA_MATHLIBNAME, luaopen_math, 1);
    lua_pop(L, 1);
    luaL_requiref(L, LUA_UTF8LIBNAME, luaopen_utf8, 1);
    lua_pop(L, 1);
}

static int divoom_game_native_lua_call_func(const char *func_name, int arg_count)
{
    lua_State *L;

    L = g_native_lua_ctrl->L;
    if (L == NULL)
    {
        return -1;
    }
    lua_getglobal(L, func_name);
    if (!lua_isfunction(L, -1))
    {
        lua_pop(L, 1);
        return -1;
    }
    if (lua_pcall(L, arg_count, 0, 0) != LUA_OK)
    {
        divoom_print("[native_lua] %s: %s\n", func_name, lua_tostring(L, -1));
        lua_pop(L, 1);
        return -1;
    }
    return 0;
}

static void divoom_game_native_lua_clear_pressed(void)
{
    int i;

    for (i = 0; i < DIVOOM_GAME_KEY_COUNT; i++)
    {
        g_native_lua_ctrl->key_pressed[i] = 0;
    }
}

static void divoom_game_native_lua_timer_cb(lv_timer_t *t)
{
    (void)t;

    if (!g_native_lua_ctrl || !g_native_lua_ctrl->running || g_native_lua_ctrl->paused)
    {
        return;
    }
    if (g_native_lua_ctrl->quit_request)
    {
        if (!g_native_lua_ctrl->quit_notified && g_native_lua_ctrl->quit_cb)
        {
            g_native_lua_ctrl->quit_notified = 1;
            g_native_lua_ctrl->quit_cb(g_native_lua_ctrl->quit_user_data);
        }
        return;
    }

    if (g_native_lua_ctrl->L)
    {
        lua_State *L;

        g_native_lua_ctrl->frame_seq++;
        L = g_native_lua_ctrl->L;
        divoom_game_native_lua_clear_text_cmds();
        g_native_lua_ctrl->defer_invalidate = 1;
        lua_getglobal(L, "on_frame");
        if (lua_isfunction(L, -1))
        {
            lua_pushinteger(L, (lua_Integer)g_native_lua_ctrl->frame_ms);
            if (lua_pcall(L, 1, 0, 0) != LUA_OK)
            {
                divoom_print("[native_lua] on_frame: %s\n", lua_tostring(L, -1));
                lua_pop(L, 1);
            }
        }
        else
        {
            lua_pop(L, 1);
        }
        lua_gc(L, LUA_GCSTEP, 1);
        divoom_game_native_lua_sweep_img_cache();
        if ((g_native_lua_ctrl->frame_seq % 125U) == 0U)
        {
            divoom_game_native_lua_log_mem_stage("runtime heartbeat");
        }
        g_native_lua_ctrl->defer_invalidate = 0;
        divoom_game_native_lua_flush_dirty();
    }
    divoom_game_native_lua_clear_pressed();
}

static int divoom_game_native_key_is_left(uint8_t key_code)
{
    /* Native Lua 游戏运行时使用实体方向语义：PC/真机左右键会经输入层映射为 LIGHT_LEFT/RIGHT。
     * 通用菜单的二维导航保留原有规则，这里只补齐游戏内方向输入。 */
    if (divoom_astrootoo_ui_key_is_left(key_code) || key_code == DIVOOM_KEY_LIGHT_LEFT)
    {
        return 1;
    }

    return 0;
}

static int divoom_game_native_key_is_right(uint8_t key_code)
{
    /* 与 left 对称，确保 native Lua 游戏能收到实体右键并派发为 on_key("right")。 */
    if (divoom_astrootoo_ui_key_is_right(key_code) || key_code == DIVOOM_KEY_LIGHT_RIGHT)
    {
        return 1;
    }

    return 0;
}

static divoom_game_key_slot_t divoom_game_native_key_slot_from_code(uint8_t key_code)
{
    if (divoom_astrootoo_ui_key_is_up(key_code))
    {
        return DIVOOM_GAME_KEY_UP;
    }
    if (divoom_astrootoo_ui_key_is_down(key_code))
    {
        return DIVOOM_GAME_KEY_DOWN;
    }
    if (divoom_game_native_key_is_left(key_code))
    {
        return DIVOOM_GAME_KEY_LEFT;
    }
    if (divoom_game_native_key_is_right(key_code))
    {
        return DIVOOM_GAME_KEY_RIGHT;
    }
    if (divoom_astrootoo_ui_key_is_enter(key_code))
    {
        return DIVOOM_GAME_KEY_A;
    }
    if (divoom_astrootoo_ui_key_is_esc(key_code))
    {
        return DIVOOM_GAME_KEY_B;
    }
    return DIVOOM_GAME_KEY_COUNT;
}

/* 按住重复事件：仅刷新 key_down 与时间戳，不触发 Lua on_key（避免离散动作被重复触发）。
 * 配合 input_down 的超时机制，实现"按住持续判定为按下、松开后自动复位" */
void divoom_game_native_lua_key_hold(uint8_t key_code)
{
    divoom_game_key_slot_t slot;

    if (!g_native_lua_ctrl || !g_native_lua_ctrl->running)
    {
        return;
    }
    slot = divoom_game_native_key_slot_from_code(key_code);
    if (slot >= DIVOOM_GAME_KEY_COUNT)
    {
        return;
    }
    g_native_lua_ctrl->key_down[slot] = 1;
    g_native_lua_ctrl->key_last_ms[slot] = lv_tick_get();
}

void divoom_game_native_lua_on_key(uint8_t key_code, uint8_t pressed)
{
    divoom_game_key_slot_t slot;
    const char *key_name;

    if (!g_native_lua_ctrl || !g_native_lua_ctrl->running)
    {
        return;
    }

    slot = divoom_game_native_key_slot_from_code(key_code);
    if (slot >= DIVOOM_GAME_KEY_COUNT)
    {
        return;
    }

    if (pressed)
    {
        g_native_lua_ctrl->key_down[slot] = 1;
        g_native_lua_ctrl->key_pressed[slot] = 1;
        g_native_lua_ctrl->key_last_ms[slot] = lv_tick_get();
    }
    else
    {
        g_native_lua_ctrl->key_down[slot] = 0;
    }

    if (g_native_lua_ctrl->L)
    {
        lua_State *L;

        L = g_native_lua_ctrl->L;
        switch (slot)
        {
        case DIVOOM_GAME_KEY_UP:
        {
            key_name = "up";
            break;
        }
        case DIVOOM_GAME_KEY_DOWN:
        {
            key_name = "down";
            break;
        }
        case DIVOOM_GAME_KEY_LEFT:
        {
            key_name = "left";
            break;
        }
        case DIVOOM_GAME_KEY_RIGHT:
        {
            key_name = "right";
            break;
        }
        case DIVOOM_GAME_KEY_A:
        {
            key_name = "a";
            break;
        }
        case DIVOOM_GAME_KEY_B:
        {
            key_name = "b";
            break;
        }
        default:
        {
            key_name = "start";
            break;
        }
        }

        lua_getglobal(L, "on_key");
        if (lua_isfunction(L, -1))
        {
            lua_pushstring(L, key_name);
            lua_pushboolean(L, pressed ? 1 : 0);
            if (lua_pcall(L, 2, 0, 0) != LUA_OK)
            {
                divoom_print("[native_lua] on_key: %s\n", lua_tostring(L, -1));
                lua_pop(L, 1);
            }
        }
        else
        {
            lua_pop(L, 1);
        }
    }
}

int divoom_game_native_lua_dispatch_back(uint8_t key_code)
{
    if (!g_native_lua_ctrl || !g_native_lua_ctrl->running)
    {
        return 0;
    }
    g_native_lua_ctrl->back_consumed = 0;
    /* 复用统一的 on_key 通道，把 ESC 作为 "b" 键边沿派发给 Lua。
     * Lua 若逐级返回会调用 game.consume_back()，若在顶层会调用 game.quit()。 */
    divoom_game_native_lua_on_key(key_code, 1);
    return (g_native_lua_ctrl->back_consumed || g_native_lua_ctrl->quit_request) ? 1 : 0;
}

int divoom_game_native_lua_start(const divoom_game_entry_t *entry, lv_obj_t *parent,
    divoom_game_native_quit_cb quit_cb, void *quit_user_data)
{
    char script_path[DIVOOM_GAME_PATH_MAX];
    lua_State *L;
    uint32 period;

    if (entry == NULL || parent == NULL || g_native_lua_ctrl)
    {
        return -1;
    }
    g_native_lua_ctrl = divoom_system_malloc(sizeof(*g_native_lua_ctrl));
    if (!g_native_lua_ctrl)
    {
        divoom_print("[native_lua] control alloc failed size=%u\n", (unsigned)sizeof(*g_native_lua_ctrl));
        return -1;
    }
    memset(g_native_lua_ctrl, 0, sizeof(*g_native_lua_ctrl));
    g_native_lua_ctrl->entry = *entry;
    g_native_lua_ctrl->quit_cb = quit_cb;
    g_native_lua_ctrl->quit_user_data = quit_user_data;
    g_native_lua_ctrl->root = parent;
    divoom_game_native_lua_log_mem_stage("start before lvgl cache drop");
    divoom_lvgl9_image_cache_drop_all();
    divoom_lvgl9_image_cache_set_game_limit(DIVOOM_GAME_LVGL_IMAGE_CACHE_GAME_BYTES);
    g_native_lua_ctrl->lvgl_cache_limited = 1;
    divoom_game_native_lua_log_mem_stage("start after lvgl cache drop");

    g_native_lua_ctrl->base_rgb565_buf = (uint8 *)divoom_system_malloc(
        DIVOOM_GAME_BASE_RGB565_POOL_BYTES);
    if (g_native_lua_ctrl->base_rgb565_buf == NULL)
    {
        divoom_print("[native_lua] base rgb565 pool alloc failed size=%u\n",
            (uint32)DIVOOM_GAME_BASE_RGB565_POOL_BYTES);
        divoom_game_native_lua_stop();
        return -1;
    }
    g_native_lua_ctrl->base_rgb565_buf_size = DIVOOM_GAME_BASE_RGB565_POOL_BYTES;
    divoom_print("[native_lua] base rgb565 pool ready ptr=%p size=%u\n",
        g_native_lua_ctrl->base_rgb565_buf, g_native_lua_ctrl->base_rgb565_buf_size);
    divoom_game_native_lua_log_mem_stage("start after base pool");

    g_native_lua_ctrl->fb = (uint8 *)divoom_system_malloc((uint32)DIVOOM_GAME_FB_BYTES);
    if (g_native_lua_ctrl->fb == NULL)
    {
        divoom_game_native_lua_stop();
        return -1;
    }
    memset(g_native_lua_ctrl->fb, 0, (size_t)DIVOOM_GAME_FB_BYTES);
    g_native_lua_ctrl->canvas_fb = g_native_lua_ctrl->fb;
#if DIVOOM_GAME_NATIVE_LUA_DOUBLE_BUFFER
    g_native_lua_ctrl->canvas_fb = (uint8 *)divoom_system_malloc((uint32)DIVOOM_GAME_FB_BYTES);
    if (g_native_lua_ctrl->canvas_fb != NULL)
    {
        memset(g_native_lua_ctrl->canvas_fb, 0, (size_t)DIVOOM_GAME_FB_BYTES);
        g_native_lua_ctrl->use_double_buffer = 1;
    }
    else
    {
        g_native_lua_ctrl->canvas_fb = g_native_lua_ctrl->fb;
        g_native_lua_ctrl->use_double_buffer = 0;
        divoom_print("[native_lua] canvas fb alloc failed size=%u, fallback single buffer\n",
            (uint32)DIVOOM_GAME_FB_BYTES);
    }
#endif
    divoom_print("[native_lua] fb ready draw=%p canvas=%p size=%u format=RGB565 double=%d\n",
        g_native_lua_ctrl->fb, g_native_lua_ctrl->canvas_fb,
        (uint32)DIVOOM_GAME_FB_BYTES, g_native_lua_ctrl->use_double_buffer);
    divoom_game_native_lua_log_mem_stage("start after fb");

    divoom_game_lang_load_current();
    divoom_game_native_lua_log_mem_stage("start after lang");

    /* 先清后缓冲；双缓冲时随第一帧提交到 canvas_fb，单缓冲时直接显示。 */
    divoom_game_fb_fill_rect(0, 0, DIVOOM_GAME_FB_W, DIVOOM_GAME_FB_H,
        lv_color_hex(0x1A2430));
    if (g_native_lua_ctrl->use_double_buffer && g_native_lua_ctrl->canvas_fb != NULL)
    {
        memcpy(g_native_lua_ctrl->canvas_fb, g_native_lua_ctrl->fb,
            (size_t)DIVOOM_GAME_FB_BYTES);
    }

    g_native_lua_ctrl->canvas = lv_canvas_create(parent);
    if (g_native_lua_ctrl->canvas == NULL)
    {
        divoom_game_native_lua_stop();
        return -1;
    }
    lv_obj_set_style_bg_opa(g_native_lua_ctrl->canvas, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(g_native_lua_ctrl->canvas, 0, 0);
    lv_obj_set_style_pad_all(g_native_lua_ctrl->canvas, 0, 0);
    lv_obj_set_size(g_native_lua_ctrl->canvas, DIVOOM_GAME_FB_W, DIVOOM_GAME_FB_H);
    lv_obj_set_pos(g_native_lua_ctrl->canvas, 0, 0);
    /* 游戏画面全屏不透明输出，使用纯 RGB565 避开 RGB565A8 平面布局差异。 */
    lv_canvas_set_buffer(g_native_lua_ctrl->canvas, g_native_lua_ctrl->canvas_fb,
        DIVOOM_GAME_FB_W, DIVOOM_GAME_FB_H, DIVOOM_GAME_FRONT_COLOR_FORMAT);

    L = lua_newstate(divoom_game_lua_alloc, g_native_lua_ctrl);
    if (L == NULL)
    {
        divoom_game_native_lua_stop();
        return -1;
    }
    g_native_lua_ctrl->L = L;

    divoom_game_native_lua_open_safe_libs(L);
    divoom_game_native_lua_register_api(L);
    divoom_game_native_lua_log_mem_stage("start after lua init");

    if (divoom_game_native_lua_load_modules(L, entry->dir_abs) != 0)
    {
        divoom_game_native_lua_stop();
        return -1;
    }

    lv_snprintf(script_path, sizeof(script_path), "%s/%s", entry->dir_abs, entry->entry);
    if (divoom_game_native_lua_load_script(L, script_path) != 0)
    {
        divoom_game_native_lua_stop();
        return -1;
    }

    divoom_game_native_lua_call_func("on_init", 0);

    /* 首帧绘制，避免 on_frame 节流导致进入后长时间空白 */
    if (g_native_lua_ctrl->L)
    {
        lua_State *L;

        g_native_lua_ctrl->frame_seq++;
        L = g_native_lua_ctrl->L;
        divoom_game_native_lua_clear_text_cmds();
        g_native_lua_ctrl->defer_invalidate = 1;
        lua_getglobal(L, "on_frame");
        if (lua_isfunction(L, -1))
        {
            lua_pushinteger(L, 0);
            if (lua_pcall(L, 1, 0, 0) != LUA_OK)
            {
                divoom_print("[native_lua] on_frame(init): %s\n", lua_tostring(L, -1));
                lua_pop(L, 1);
            }
        }
        else
        {
            lua_pop(L, 1);
        }
        g_native_lua_ctrl->defer_invalidate = 0;
        divoom_game_native_lua_flush_dirty();
    }

    period = entry->fps ? (1000 / entry->fps) : DIVOOM_GAME_FRAME_MS_DEFAULT;
    if (period < 16)
    {
        period = 16;
    }
    g_native_lua_ctrl->frame_ms = period;
    g_native_lua_ctrl->timer = lv_timer_create(divoom_game_native_lua_timer_cb, period, NULL);
    if (!g_native_lua_ctrl->timer)
    {
        divoom_game_native_lua_stop();
        return -1;
    }
    g_native_lua_ctrl->running = 1;
    divoom_print("[native_lua] started id=%s script=%s\n", entry->id, script_path);
    return 0;
}

void divoom_game_native_lua_stop(void)
{
    uint8 had_state;

    if (!g_native_lua_ctrl)
    {
        return;
    }
    had_state = (g_native_lua_ctrl->running || g_native_lua_ctrl->L || g_native_lua_ctrl->fb ||
        g_native_lua_ctrl->canvas_fb ||
        g_native_lua_ctrl->canvas || g_native_lua_ctrl->draw_canvas ||
        g_native_lua_ctrl->img_cache_owned_bytes > 0 ||
        g_native_lua_ctrl->img_cache_external_bytes > 0 ||
        g_native_lua_ctrl->base_rgb565_buf != NULL ||
        g_native_lua_ctrl->lvgl_cache_limited) ? 1 : 0;
    if (had_state)
    {
        divoom_print("[native_lua] stop begin id=%s cache_count=%d cache_owned_bytes=%u cache_external_bytes=%u lua_heap_used=%u fb=%p canvas_fb=%p double=%d\n",
            g_native_lua_ctrl->entry.id,
            divoom_game_native_lua_get_img_cache_count(),
            g_native_lua_ctrl->img_cache_owned_bytes,
            g_native_lua_ctrl->img_cache_external_bytes,
            g_native_lua_ctrl->lua_heap_used,
            g_native_lua_ctrl->fb,
            g_native_lua_ctrl->canvas_fb,
            g_native_lua_ctrl->use_double_buffer);
    }
    if (g_native_lua_ctrl->timer)
    {
        lv_timer_del(g_native_lua_ctrl->timer);
        g_native_lua_ctrl->timer = NULL;
    }
    g_native_lua_ctrl->running = 0;
    /* Lua finalizers and its allocator may still access the game data. */
    if (g_native_lua_ctrl->L)
    {
        lua_close(g_native_lua_ctrl->L);
        g_native_lua_ctrl->L = NULL;
    }
    divoom_game_native_lua_label_clear_all();
    divoom_game_native_lua_free_img_cache();
    if (g_native_lua_ctrl->base_rgb565_buf)
    {
        divoom_print("[native_lua] base rgb565 pool free ptr=%p size=%u\n",
            g_native_lua_ctrl->base_rgb565_buf, g_native_lua_ctrl->base_rgb565_buf_size);
        divoom_system_free(g_native_lua_ctrl->base_rgb565_buf);
        g_native_lua_ctrl->base_rgb565_buf = NULL;
        g_native_lua_ctrl->base_rgb565_buf_size = 0;
    }
    if (g_native_lua_ctrl->canvas)
    {
        lv_obj_del(g_native_lua_ctrl->canvas);
        g_native_lua_ctrl->canvas = NULL;
    }
    if (g_native_lua_ctrl->draw_canvas)
    {
        lv_obj_del(g_native_lua_ctrl->draw_canvas);
        g_native_lua_ctrl->draw_canvas = NULL;
    }
    divoom_game_native_lua_clear_text_cmds();
    if (g_native_lua_ctrl->canvas_fb && g_native_lua_ctrl->canvas_fb != g_native_lua_ctrl->fb)
    {
        divoom_system_free(g_native_lua_ctrl->canvas_fb);
        g_native_lua_ctrl->canvas_fb = NULL;
    }
    if (g_native_lua_ctrl->fb)
    {
        divoom_system_free(g_native_lua_ctrl->fb);
        g_native_lua_ctrl->fb = NULL;
    }
    g_native_lua_ctrl->canvas_fb = NULL;
    g_native_lua_ctrl->use_double_buffer = 0;
    if (g_native_lua_ctrl->lvgl_cache_limited)
    {
        divoom_lvgl9_image_cache_restore_default();
        divoom_lvgl9_image_cache_drop_all();
        g_native_lua_ctrl->lvgl_cache_limited = 0;
        divoom_game_native_lua_log_mem_stage("stop after lvgl cache restore");
    }
#if DIVOOM_GAME_MANUAL_FONT_DRAW
    if (g_native_lua_ctrl->font_bitmap_buf)
    {
        divoom_system_free(g_native_lua_ctrl->font_bitmap_buf);
        g_native_lua_ctrl->font_bitmap_buf = NULL;
        g_native_lua_ctrl->font_bitmap_buf_size = 0;
    }
#endif
    divoom_game_lang_free();
    divoom_system_free(g_native_lua_ctrl);
    g_native_lua_ctrl = NULL;
    divoom_print("[native_lua] stop end\n");
}

void divoom_game_native_lua_pause(int paused)
{
    if (!g_native_lua_ctrl)
    {
        return;
    }
    g_native_lua_ctrl->paused = paused ? 1 : 0;
    if (g_native_lua_ctrl->timer)
    {
        if (paused)
        {
            lv_timer_pause(g_native_lua_ctrl->timer);
        }
        else
        {
            lv_timer_resume(g_native_lua_ctrl->timer);
        }
    }
}

int divoom_game_native_lua_is_running(void)
{
    return g_native_lua_ctrl && g_native_lua_ctrl->running ? 1 : 0;
}

/* 返回当前正在运行的游戏 id（来自 entry.id）；未运行返回空串。用于自动化验证确认启动了正确的游戏。 */
const char *divoom_game_native_lua_get_id(void)
{
    static char id[DIVOOM_GAME_ID_MAX];
    if (!g_native_lua_ctrl || !g_native_lua_ctrl->running)
    {
        return "";
    }
    memcpy(id, g_native_lua_ctrl->entry.id, sizeof(id));
    return id;
}

/* 返回当前 Lua 全局 G.state 字符串（用于自动化验证按状态驱动）；
 * 与 timer_cb/on_key 同在 LVGL 单线程顺序执行，不会重入破坏 lua 栈 */
const char *divoom_game_native_lua_get_state(void)
{
    static char buf[32];
    lua_State *L;

    buf[0] = 0;
    if (!g_native_lua_ctrl || !g_native_lua_ctrl->running || !g_native_lua_ctrl->L)
    {
        return buf;
    }
    L = g_native_lua_ctrl->L;
    lua_getglobal(L, "G");
    if (lua_istable(L, -1))
    {
        const char *s;
        lua_getfield(L, -1, "state");
        s = lua_tostring(L, -1);
        if (s != NULL)
        {
            strncpy(buf, s, sizeof(buf) - 1);
            buf[sizeof(buf) - 1] = 0;
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
    return buf;
}

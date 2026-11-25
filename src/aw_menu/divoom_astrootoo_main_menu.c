#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_astrootoo_ui_base.h"
#include "divoom_astrootoo_main_menu.h"
#include "divoom_astrootoo_time_pick.h"
#include "divoom_astrootoo_theme.h"
#include "divoom_music_ctrl.h"
#include "divoom_bt_music.h"
#include "divoom_pixel_format.h"
#include "divoom_wifi_con.h"
#include "divoom_power_monitor.h"
#include "divoom_tws_host.h"

#include <time.h>
#include <math.h>

#define DIVOOM_MAIN_MENU_SHOW_BAT_MV  0     // 0: 不显示电压，1: 显示电压（调试用）

/* === 单图 lg + zoom 缩放 Cover Flow（试验：替代三档 crossfade）=== */
#define DIVOOM_ASTRO_MAIN_ICON_MAX          16
#define DIVOOM_ASTRO_MAIN_CENTER_X          (DIVOOM_SCREEN_WIDTH / 2)
#define DIVOOM_ASTRO_MAIN_CARD_GAP_PX       10
#define DIVOOM_ASTRO_MAIN_CARD_LG_W         118
#define DIVOOM_ASTRO_MAIN_CARD_LG_H         200
#define DIVOOM_ASTRO_MAIN_CARD_MD_W         82
#define DIVOOM_ASTRO_MAIN_CARD_MD_H         140
#define DIVOOM_ASTRO_MAIN_CARD_SM_W         62
#define DIVOOM_ASTRO_MAIN_CARD_SM_H         106
#define DIVOOM_ASTRO_MAIN_PEEK_OPA          122
#define DIVOOM_ASTRO_MAIN_VISIBLE_D         2.0f
#define DIVOOM_ASTRO_MAIN_ANIM_BLEED        1.0f
#define DIVOOM_ASTRO_MAIN_ANIM_MS           340
#define DIVOOM_ASTRO_MAIN_ANIM_REPEAT_MS    120
#define DIVOOM_ASTRO_MAIN_CUR_SCALE         1000
#define DIVOOM_ASTRO_MAIN_FOCUS_FRAME_OUTSET 1
#define DIVOOM_ASTRO_MAIN_MENU_HEADER_SOURCE_CNT  5
#define DIVOOM_ASTRO_MAIN_MENU_HEADER_WIFI_CNT     5

/* 出厂 25×14 只加长 1px：容器 26×14，内腔 20×10，满电左右各留 1px */
#define DIVOOM_ASTRO_MAIN_BAT_FRAME_COLOR   0xE9C5AE
#define DIVOOM_ASTRO_MAIN_BAT_RED_COLOR     0xA21E1E
#define DIVOOM_ASTRO_MAIN_BAT_GREEN_COLOR   0x78C26B
#define DIVOOM_ASTRO_MAIN_BAT_FILL_X        2
#define DIVOOM_ASTRO_MAIN_BAT_FILL_Y        2
#define DIVOOM_ASTRO_MAIN_BAT_FILL_W        20
#define DIVOOM_ASTRO_MAIN_BAT_FILL_H        10
#define DIVOOM_ASTRO_MAIN_BAT_LEVEL_MAX     9
/* 首屏失败的 PNG：后台等到电影 idle 后再解一次，封顶 2s，对齐 PNG 层 500ms 冷却。 */
#define DIVOOM_ASTRO_MAIN_ICON_RETRY_WAIT_MS      2000u
#define DIVOOM_ASTRO_MAIN_ICON_RETRY_PERIOD_MS    80
#define DIVOOM_ASTRO_MAIN_ICON_RETRY_COOLDOWN_MS  500u

typedef struct
{
    const char *label_key;
    const char *slug;
    int menu_id;
} divoom_astrootoo_main_item_t;

typedef struct
{
    lv_obj_t *slide;
    lv_obj_t *focus_frame;
    lv_obj_t *card_img;
} divoom_astrootoo_main_slide_t;

typedef struct _divoom_astrootoo_main_menu_ctrl
{
    divoom_astrootoo_ui_base ui;
    lv_obj_t *root_obj;
    lv_obj_t *bg_img;
    lv_obj_t *key_btn;
    lv_obj_t *header_bar;
    lv_obj_t *header_source_img;
    lv_obj_t *header_tws_img;
    lv_obj_t *header_wifi_img;
    lv_obj_t *header_battery_img;
#if DIVOOM_MAIN_MENU_SHOW_BAT_MV
    lv_obj_t *header_volt_label;
    uint16 header_volt_mv;
#endif
    lv_obj_t *status_time_label;
    divoom_astrootoo_main_slide_t slides[DIVOOM_ASTRO_MAIN_ICON_MAX];
    lv_obj_t *caption_label;
    lv_obj_t *nav_hint_cont;
    lv_obj_t *nav_chevron_left;
    lv_obj_t *nav_chevron_right;
    lv_obj_t *nav_hint_label;
    lv_img_dsc_t bg_dsc;
    lv_img_dsc_t focus_frame_dsc;
    lv_img_dsc_t card_dsc[DIVOOM_ASTRO_MAIN_ICON_MAX];
    lv_img_dsc_t nav_chevron_l_dsc;
    lv_img_dsc_t nav_chevron_r_dsc;
    lv_img_dsc_t header_source_dsc[DIVOOM_ASTRO_MAIN_MENU_HEADER_SOURCE_CNT];
    lv_img_dsc_t header_tws_dsc;
    lv_img_dsc_t header_wifi_dsc[DIVOOM_ASTRO_MAIN_MENU_HEADER_WIFI_CNT];
    lv_timer_t *tick_timer;
    lv_timer_t *load_timer;
    lv_timer_t *free_timer;
    lv_obj_t *scan_bar;
    uint32_t enter_guard_ms;
    int menu_id;
    int focus_index;
    int item_count;
    uint8 icons_loaded;
    uint8 icon_retry_pending;
    uint32_t icon_retry_start_ms;
    uint8 load_bg_pending;
    uint8 load_nav_pending;
    uint8 focus_load_done;
    int icon_load_item;
    int header_source_load_idx;
    int header_wifi_load_idx;
    int header_source_icon_idx;
    uint8 header_tws_role;
    uint8 header_wifi_icon_sig;
    uint8 header_battery_icon_sig;
    uint8 anim_active;
    float cur;
    float cur_target;
} divoom_astrootoo_main_menu_ctrl, *Pdivoom_astrootoo_main_menu_ctrl;

typedef enum
{
    DIVOOM_ASTRO_MAIN_MENU_SRC_ICON_BT_CONNECTED = 0,
    DIVOOM_ASTRO_MAIN_MENU_SRC_ICON_BT_DISCONNECTED,
    DIVOOM_ASTRO_MAIN_MENU_SRC_ICON_TF,
    DIVOOM_ASTRO_MAIN_MENU_SRC_ICON_AUX,
    DIVOOM_ASTRO_MAIN_MENU_SRC_ICON_UAC,
} divoom_astrootoo_main_menu_src_icon_t;

typedef struct
{
    uint8 x;
    uint8 y;
    uint8 w;
    uint8 h;
} divoom_astrootoo_main_tws_rect_t;

#define DIVOOM_ASTROTOO_MAIN_MENU_UI_DIR    "1/"

static const char *g_astrootoo_main_header_source_files[DIVOOM_ASTRO_MAIN_MENU_HEADER_SOURCE_CNT] =
{
    "main_menu_status_source_bt_connected.png",
    "main_menu_status_source_bt_disconnected.png",
    "main_menu_status_source_tf.png",
    "main_menu_status_source_aux.png",
    "main_menu_status_source_uac.png",
};

static const char *g_astrootoo_main_header_wifi_files[DIVOOM_ASTRO_MAIN_MENU_HEADER_WIFI_CNT] =
{
    "main_menu_status_wifi_0.png",
    "main_menu_status_wifi_1.png",
    "main_menu_status_wifi_2.png",
    "main_menu_status_wifi_3.png",
    "main_menu_status_wifi_4.png",
};

/* 真源: flowtooUiPresets.ts FLOWTOO_MENU_NEXT_CAROUSEL_KEYS + flowtooMainMenuCardAssets slug */
static const divoom_astrootoo_main_item_t g_astrootoo_main_items[] =
{
    { "WATCH FACE MGMT", "watch-face", DIVOOM_MENU_CONTROL_ID_STORE_CLOCK_MARKET },
    { "ALBUMS", "gallery", DIVOOM_MENU_CONTROL_ID_SELECT_ALBUM_PLAY },
    { "Alarm", "alarm", DIVOOM_MENU_CONTROL_ID_ALARM_LIST_MENU },
    { "SleepAid", "sleep", DIVOOM_MENU_CONTROL_ID_SLEEP_AID },
    { "Tool Box", "tools", DIVOOM_MENU_CONTROL_ID_TOOL },
    { "Games", "astro-world", DIVOOM_MENU_CONTROL_ID_ASTROTOO_GAMES },
    { "Setting", "settings", DIVOOM_MENU_CONTROL_ID_SYSTEM_SETING_MENU },
    { "AudioSource2", "source", DIVOOM_MENU_CONTROL_ID_AUDIO_SOURCE_MENU },
};

#define DIVOOM_ASTROTOO_MAIN_ITEM_MAX \
    ((int)(sizeof(g_astrootoo_main_items) / sizeof(g_astrootoo_main_items[0])))

#define DIVOOM_ASTROTOO_MAIN_MENU_BG_FILE           "main_menu_background.png"
#define DIVOOM_ASTROTOO_MAIN_MENU_FOCUS_FRAME_FILE   "main_menu_card_focus_frame.png"
#define DIVOOM_ASTROTOO_MAIN_MENU_NAV_CHEVRON_L_FILE  "main_menu_nav_chevron_left.png"
#define DIVOOM_ASTROTOO_MAIN_MENU_NAV_CHEVRON_R_FILE "main_menu_nav_chevron_right.png"

static lv_img_dsc_t g_astrootoo_main_menu_detached_dsc;
static uint8 g_astrootoo_main_menu_detached_pixel[3];
static uint8 g_astrootoo_main_menu_detached_dsc_ready;

static const lv_img_dsc_t *divoom_astrootoo_main_menu_get_detached_dsc(void)
{
    if (g_astrootoo_main_menu_detached_dsc_ready == 0)
    {
        /* RGB565A8 的第三字节为 alpha；静态零值像素是一张完全透明的 1x1 图。 */
        divoom_lv_img_dsc_init_fmt(&g_astrootoo_main_menu_detached_dsc, 1, 1,
            LV_COLOR_FORMAT_RGB565A8, g_astrootoo_main_menu_detached_pixel);
        g_astrootoo_main_menu_detached_dsc_ready = 1;
    }
    return &g_astrootoo_main_menu_detached_dsc;
}

static void divoom_astrootoo_main_menu_free_img_dsc(lv_img_dsc_t *img_cache)
{
    if (!img_cache || !img_cache->data)
    {
        return;
    }
    divoom_system_free((void *)img_cache->data);
    img_cache->data = NULL;
    img_cache->data_size = 0;
}

#if !DIVOOM_LVGL9_IS_SIM
static const char *divoom_astrootoo_main_menu_png_decoder_name(const lv_img_decoder_dsc_t *decoder_dsc)
{
    if (decoder_dsc == NULL || decoder_dsc->dsc.decoder == NULL ||
        decoder_dsc->dsc.decoder->name == NULL)
    {
        return "unknown";
    }

    return decoder_dsc->dsc.decoder->name;
}

static lv_color_format_t divoom_astrootoo_main_menu_png_mem_cf(
    const lv_img_decoder_dsc_t *decoder_dsc)
{
    const char *name;

    if (decoder_dsc == NULL)
    {
        return LV_COLOR_FORMAT_UNKNOWN;
    }

    name = divoom_astrootoo_main_menu_png_decoder_name(decoder_dsc);
    /*
     * lodepng_decode32() first exports RGBA, then lv_lodepng.c swaps red/blue.
     * libpng's simplified API is also configured as BGRA. Both match LVGL
     * ARGB8888 memory on this target.
     */
    if (strcmp(name, "LODEPNG") == 0)
    {
        return LV_COLOR_FORMAT_ARGB8888;
    }
    if (strcmp(name, "PNG") == 0)
    {
        return LV_COLOR_FORMAT_ARGB8888;
    }

    return (lv_color_format_t)decoder_dsc->header.cf;
}

static void divoom_astrootoo_main_menu_abgr8888_to_rgb565a8_buf(
    const uint8 *src_abgr, uint8 *dst_rgb565a8, uint32_t px_count)
{
    uint8 *alpha;
    uint32_t i;

    if (src_abgr == NULL || dst_rgb565a8 == NULL)
    {
        return;
    }

    alpha = dst_rgb565a8 + px_count * DIVOOM_APP_RGB565_BYTES_PER_PX;
    for (i = 0; i < px_count; i++)
    {
        const uint8 *src = src_abgr + i * 4U;
        uint16 rgb565;

        rgb565 = divoom_rgb888_to_rgb565(src[3], src[2], src[1]);
        divoom_store_rgb565_le(dst_rgb565a8 + i * DIVOOM_APP_RGB565_BYTES_PER_PX,
            rgb565);
        alpha[i] = src[0];
    }
}
#endif

static void divoom_astrootoo_main_menu_reset_load_state(Pdivoom_astrootoo_main_menu_ctrl phandle)
{
    if (!phandle)
    {
        return;
    }

    phandle->icons_loaded = 0;
    phandle->icon_retry_pending = 0;
    phandle->icon_retry_start_ms = 0;
    phandle->load_bg_pending = 1;
    phandle->load_nav_pending = 1;
    phandle->focus_load_done = 0;
    phandle->icon_load_item = 0;
    phandle->header_source_load_idx = 0;
    phandle->header_wifi_load_idx = 0;
    phandle->header_source_icon_idx = -1;
    phandle->header_tws_role = 0xFF;
    phandle->header_wifi_icon_sig = 0xFF;
    phandle->header_battery_icon_sig = 0xFF;
#if DIVOOM_MAIN_MENU_SHOW_BAT_MV
    phandle->header_volt_mv = 0xFFFF;
#endif
}

static void divoom_astrootoo_main_menu_clear_img_sources(Pdivoom_astrootoo_main_menu_ctrl phandle)
{
    const lv_img_dsc_t *detached_dsc;
    int i;

    if (!phandle)
    {
        return;
    }

    /* pause 会保留对象树，先换成模块常驻的有效透明图，再释放自有图片缓存。 */
    detached_dsc = divoom_astrootoo_main_menu_get_detached_dsc();
    if (phandle->bg_img && lv_obj_is_valid(phandle->bg_img))
    {
        lv_img_set_src(phandle->bg_img, detached_dsc);
    }
    if (phandle->nav_chevron_left && lv_obj_is_valid(phandle->nav_chevron_left))
    {
        lv_img_set_src(phandle->nav_chevron_left, detached_dsc);
    }
    if (phandle->nav_chevron_right && lv_obj_is_valid(phandle->nav_chevron_right))
    {
        lv_img_set_src(phandle->nav_chevron_right, detached_dsc);
    }
    if (phandle->header_source_img && lv_obj_is_valid(phandle->header_source_img))
    {
        lv_img_set_src(phandle->header_source_img, detached_dsc);
    }
    if (phandle->header_tws_img && lv_obj_is_valid(phandle->header_tws_img))
    {
        lv_img_set_src(phandle->header_tws_img, detached_dsc);
    }
    if (phandle->header_wifi_img && lv_obj_is_valid(phandle->header_wifi_img))
    {
        lv_img_set_src(phandle->header_wifi_img, detached_dsc);
    }
    for (i = 0; i < phandle->item_count; i++)
    {
        if (phandle->slides[i].card_img && lv_obj_is_valid(phandle->slides[i].card_img))
        {
            lv_img_set_src(phandle->slides[i].card_img, detached_dsc);
        }
        if (phandle->slides[i].focus_frame && lv_obj_is_valid(phandle->slides[i].focus_frame))
        {
            lv_img_set_src(phandle->slides[i].focus_frame, detached_dsc);
            lv_obj_add_flag(phandle->slides[i].focus_frame, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static int divoom_astrootoo_main_menu_cache_png_rel(const char *rel, lv_img_dsc_t *img_cache)
{
    char path_buf[160];
    uint32_t size;

    if (!rel || !img_cache)
    {
        return 0;
    }

    divoom_astrootoo_main_menu_free_img_dsc(img_cache);
    divoom_menu_control_get_img_path(rel, path_buf);

#if DIVOOM_LVGL9_IS_SIM
    {
        lv_img_decoder_dsc_t decoder_dsc;
        lv_img_header_t header;
        lv_img_dsc_t src_dsc;

        if (lv_img_decoder_open(&decoder_dsc, path_buf, lv_color_hex(0), 0) != LV_RES_OK)
        {
            return 0;
        }

        lv_img_decoder_get_info(path_buf, &header);
        size = divoom_pixel_format_size((uint32_t)header.w, (uint32_t)header.h, LV_COLOR_FORMAT_RGB565A8);
        if (size == 0 || decoder_dsc.img_data == NULL)
        {
            lv_img_decoder_close(&decoder_dsc);
            return 0;
        }
        img_cache->data = divoom_system_malloc(size);
        if (!img_cache->data)
        {
            lv_img_decoder_close(&decoder_dsc);
            return 0;
        }

        divoom_lv_img_dsc_init_fmt(&src_dsc, header.w, header.h,
            (lv_color_format_t)decoder_dsc.header.cf, decoder_dsc.img_data);
        if(!divoom_lv_img_dsc_convert_buf(&src_dsc, (uint8 *)img_cache->data, LV_COLOR_FORMAT_RGB565A8))
        {
            divoom_system_free((void *)img_cache->data);
            img_cache->data = NULL;
            lv_img_decoder_close(&decoder_dsc);
            return 0;
        }
        divoom_lv_img_dsc_init_fmt(img_cache, header.w, header.h, LV_COLOR_FORMAT_RGB565A8, img_cache->data);
        lv_img_decoder_close(&decoder_dsc);
    }
#else
    {
        lv_img_decoder_dsc_t decoder_dsc;
        lv_img_header_t header;
        lv_img_dsc_t src_dsc = {0};
        lv_color_format_t src_cf;
        const char *decoder_name;
        uint32_t px_count;

        if (lv_img_decoder_open(&decoder_dsc, path_buf, lv_color_hex(0), 0) != LV_RES_OK)
        {
            return 0;
        }

        if (lv_img_decoder_get_info(path_buf, &header) != LV_RES_OK ||
            decoder_dsc.img_data == NULL)
        {
            lv_img_decoder_close(&decoder_dsc);
            return 0;
        }

        decoder_name = divoom_astrootoo_main_menu_png_decoder_name(&decoder_dsc);
        src_cf = divoom_astrootoo_main_menu_png_mem_cf(&decoder_dsc);
        px_count = (uint32_t)header.w * (uint32_t)header.h;

        size = divoom_pixel_format_size((uint32_t)header.w, (uint32_t)header.h,
            LV_COLOR_FORMAT_RGB565A8);
        if (size == 0)
        {
            lv_img_decoder_close(&decoder_dsc);
            return 0;
        }
        img_cache->data = divoom_system_malloc(size);
        if (!img_cache->data)
        {
            lv_img_decoder_close(&decoder_dsc);
            return 0;
        }

        if (strcmp(decoder_name, "SUNXI_PNG") == 0)
        {
            divoom_astrootoo_main_menu_abgr8888_to_rgb565a8_buf(
                decoder_dsc.img_data, (uint8 *)img_cache->data, px_count);
        }
        else
        {
            divoom_lv_img_dsc_init_fmt(&src_dsc, header.w, header.h,
                src_cf, decoder_dsc.img_data);
            if(!divoom_lv_img_dsc_convert_buf(&src_dsc, (uint8 *)img_cache->data,
                LV_COLOR_FORMAT_RGB565A8))
            {
                divoom_system_free((void *)img_cache->data);
                img_cache->data = NULL;
                lv_img_decoder_close(&decoder_dsc);
                return 0;
            }
        }
        divoom_lv_img_dsc_init_fmt(img_cache, header.w, header.h,
            LV_COLOR_FORMAT_RGB565A8, img_cache->data);
        img_cache->header.stride = divoom_pixel_format_stride((uint32_t)header.w,
            LV_COLOR_FORMAT_RGB565A8);
        lv_img_decoder_close(&decoder_dsc);
    }
#endif

    return 1;
}

static void divoom_astrootoo_main_menu_free_img_cache(
    Pdivoom_astrootoo_main_menu_ctrl phandle, uint8 detach_sources)
{
    int i;

    if (!phandle)
    {
        return;
    }

    if (detach_sources)
    {
        divoom_astrootoo_main_menu_clear_img_sources(phandle);
    }
    divoom_astrootoo_main_menu_free_img_dsc(&phandle->bg_dsc);
    divoom_astrootoo_main_menu_free_img_dsc(&phandle->focus_frame_dsc);
    divoom_astrootoo_main_menu_free_img_dsc(&phandle->nav_chevron_l_dsc);
    divoom_astrootoo_main_menu_free_img_dsc(&phandle->nav_chevron_r_dsc);
    for (i = 0; i < DIVOOM_ASTRO_MAIN_MENU_HEADER_SOURCE_CNT; i++)
    {
        divoom_astrootoo_main_menu_free_img_dsc(&phandle->header_source_dsc[i]);
    }
    divoom_astrootoo_main_menu_free_img_dsc(&phandle->header_tws_dsc);
    for (i = 0; i < DIVOOM_ASTRO_MAIN_MENU_HEADER_WIFI_CNT; i++)
    {
        divoom_astrootoo_main_menu_free_img_dsc(&phandle->header_wifi_dsc[i]);
    }
    for (i = 0; i < phandle->item_count; i++)
    {
        divoom_astrootoo_main_menu_free_img_dsc(&phandle->card_dsc[i]);
    }
    divoom_astrootoo_main_menu_reset_load_state(phandle);
}

static void divoom_astrootoo_main_menu_deferred_free_cb(lv_timer_t *timer)
{
    Pdivoom_astrootoo_main_menu_ctrl phandle;

    phandle = (Pdivoom_astrootoo_main_menu_ctrl)lv_timer_get_user_data(timer);
    if (phandle == NULL)
    {
        lv_timer_del(timer);
        return;
    }

    if (phandle->free_timer == timer)
    {
        phandle->free_timer = NULL;
    }
    /*
     * pause() 发生时主菜单仍可能是当前 screen。真机 LCD flush 可能还会
     * 刷完当前帧，所以把释放推迟到切屏后的 LVGL timer 中执行，避免旧帧
     * 继续引用已经释放的 lv_img_dsc 数据导致花屏。
     */
    divoom_astrootoo_main_menu_free_img_cache(phandle, 1);
    lv_timer_del(timer);
}

static void divoom_astrootoo_main_menu_force_full_refresh(lv_obj_t *root_obj)
{
    lv_disp_t *disp;

    if (!root_obj || !lv_obj_is_valid(root_obj))
    {
        return;
    }

    /*
     * 真机从游戏/分类页退回主菜单时，LCD 可能还残留上一屏最后一次 flush。
     * 切屏点主动让新 screen 全屏失效并同步刷新一次，避免透明区域或刚恢复的
     * 图片缓存尚未进入下一帧时露出旧 framebuffer。
     */
    lv_obj_invalidate(root_obj);
    disp = lv_obj_get_disp(root_obj);
    lv_refr_now(disp);
}

static void divoom_astrootoo_main_menu_ensure_bg_ready(Pdivoom_astrootoo_main_menu_ctrl phandle)
{
    char rel[48];

    if (!phandle || !phandle->bg_img || !lv_obj_is_valid(phandle->bg_img))
    {
        return;
    }
    if (phandle->bg_dsc.data != NULL)
    {
        lv_img_set_src(phandle->bg_img, &phandle->bg_dsc);
        return;
    }

    snprintf(rel, sizeof(rel), "%s%s",
        DIVOOM_ASTROTOO_MAIN_MENU_UI_DIR, DIVOOM_ASTROTOO_MAIN_MENU_BG_FILE);
    if (divoom_astrootoo_main_menu_cache_png_rel(rel, &phandle->bg_dsc)
        && phandle->bg_dsc.data != NULL)
    {
        lv_img_set_src(phandle->bg_img, &phandle->bg_dsc);
        phandle->load_bg_pending = 0;
    }
}

static void divoom_astrootoo_main_menu_build_card_src(char *buf, int buf_len, const char *slug)
{
    snprintf(buf, (size_t)buf_len, "%slg-%s.png",
        DIVOOM_ASTROTOO_MAIN_MENU_UI_DIR, slug);
}

static void divoom_astrootoo_main_menu_interpolated_dims(float abs_d, lv_coord_t *w, lv_coord_t *h)
{
    float d = abs_d;
    float lg_w = (float)DIVOOM_ASTRO_MAIN_CARD_LG_W;
    float lg_h = (float)DIVOOM_ASTRO_MAIN_CARD_LG_H;
    float md_w = (float)DIVOOM_ASTRO_MAIN_CARD_MD_W;
    float md_h = (float)DIVOOM_ASTRO_MAIN_CARD_MD_H;
    float sm_w = (float)DIVOOM_ASTRO_MAIN_CARD_SM_W;
    float sm_h = (float)DIVOOM_ASTRO_MAIN_CARD_SM_H;
    float t;

    if (d < 0.0f)
    {
        d = 0.0f;
    }
    if (d <= 1.0f)
    {
        t = (d > 1.0f) ? 1.0f : d;
        *w = (lv_coord_t)(lg_w + (md_w - lg_w) * t);
        *h = (lv_coord_t)(lg_h + (md_h - lg_h) * t);
        return;
    }
    if (d <= 2.0f)
    {
        t = d - 1.0f;
        *w = (lv_coord_t)(md_w + (sm_w - md_w) * t);
        *h = (lv_coord_t)(md_h + (sm_h - md_h) * t);
        return;
    }
    *w = DIVOOM_ASTRO_MAIN_CARD_SM_W;
    *h = DIVOOM_ASTRO_MAIN_CARD_SM_H;
}

static int divoom_astrootoo_main_menu_half_width(float abs_d)
{
    lv_coord_t w;
    lv_coord_t h;

    divoom_astrootoo_main_menu_interpolated_dims(abs_d, &w, &h);
    return w / 2;
}

static float divoom_astrootoo_main_menu_segment_center_span(float abs_from, float abs_to)
{
    return (float)divoom_astrootoo_main_menu_half_width(abs_from)
        + (float)DIVOOM_ASTRO_MAIN_CARD_GAP_PX
        + (float)divoom_astrootoo_main_menu_half_width(abs_to);
}

static float divoom_astrootoo_main_menu_center_offset_for_distance(float d)
{
    float sign;
    float abs_d;
    int full_steps;
    float frac;
    float off = 0.0f;
    int i;

    if (fabsf(d) < 1e-6f)
    {
        return 0.0f;
    }
    sign = (d > 0.0f) ? 1.0f : -1.0f;
    abs_d = fabsf(d);
    full_steps = (int)floorf(abs_d);
    frac = abs_d - (float)full_steps;
    for (i = 0; i < full_steps; i++)
    {
        off += divoom_astrootoo_main_menu_segment_center_span((float)i, (float)(i + 1));
    }
    if (frac > 1e-6f)
    {
        off += frac * divoom_astrootoo_main_menu_segment_center_span((float)full_steps,
            (float)(full_steps + 1));
    }
    return sign * off;
}

static float divoom_astrootoo_main_menu_edge_reveal(float abs_d, uint8 anim_active)
{
    float limit = DIVOOM_ASTRO_MAIN_VISIBLE_D;
    float bleed = DIVOOM_ASTRO_MAIN_ANIM_BLEED;

    if (abs_d <= limit)
    {
        return 1.0f;
    }
    if (abs_d > limit + bleed)
    {
        return 0.0f;
    }
    if (!anim_active)
    {
        return 0.0f;
    }
    return (limit + bleed - abs_d) / bleed;
}

static void divoom_astrootoo_main_menu_layout_nav_hint(Pdivoom_astrootoo_main_menu_ctrl phandle)
{
    lv_coord_t row_w;
    lv_coord_t x;
    lv_coord_t bar_h = DIVOOM_ASTRO_MAIN_NAV_HINT_H;
    lv_coord_t chev_w = DIVOOM_ASTRO_MAIN_NAV_CHEVRON_W;
    lv_coord_t chev_h = DIVOOM_ASTRO_MAIN_NAV_CHEVRON_H;
    lv_coord_t gap = DIVOOM_ASTRO_MAIN_NAV_HINT_GAP;
    const lv_font_t *font;
    lv_point_t text_size;
    const char *txt;

    if (!phandle || !phandle->nav_hint_label)
    {
        return;
    }

    txt = lv_label_get_text(phandle->nav_hint_label);
    font = lv_obj_get_style_text_font(phandle->nav_hint_label, 0);
    if (!txt || !font)
    {
        return;
    }

    lv_txt_get_size(&text_size, txt, font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    row_w = chev_w + gap + chev_w + gap + text_size.x;
    x = (DIVOOM_ASTRO_MAIN_NAV_HINT_W - row_w) / 2;

    if (phandle->nav_chevron_left)
    {
        lv_obj_set_pos(phandle->nav_chevron_left, x, (bar_h - chev_h) / 2);
        x += chev_w + gap;
    }
    if (phandle->nav_chevron_right)
    {
        lv_obj_set_pos(phandle->nav_chevron_right, x, (bar_h - chev_h) / 2);
        x += chev_w + gap;
    }
    lv_obj_set_pos(phandle->nav_hint_label, x, (bar_h - text_size.y) / 2);
}

static int divoom_astrootoo_main_menu_pick_source_icon(void)
{
    uint8 mode;

    mode = divoom_music_ctrl_get_mode();
    if (mode == DIVOOM_MUSIC_CTRL_MODE_BT)
    {
        if (divoom_bt_music_is_connected())
        {
            return DIVOOM_ASTRO_MAIN_MENU_SRC_ICON_BT_CONNECTED;
        }
        return DIVOOM_ASTRO_MAIN_MENU_SRC_ICON_BT_DISCONNECTED;
    }
    if (mode == DIVOOM_MUSIC_CTRL_MODE_LOCAL)
    {
        return DIVOOM_ASTRO_MAIN_MENU_SRC_ICON_TF;
    }
#if DIVOOM_MUSIC_CTRL_LINEIN_ENABLE
    if (mode == DIVOOM_MUSIC_CTRL_MODE_LINEIN)
    {
        return DIVOOM_ASTRO_MAIN_MENU_SRC_ICON_AUX;
    }
#endif
    if (mode == DIVOOM_MUSIC_CTRL_MODE_UAC)
    {
        return DIVOOM_ASTRO_MAIN_MENU_SRC_ICON_UAC;
    }
    return DIVOOM_ASTRO_MAIN_MENU_SRC_ICON_BT_DISCONNECTED;
}

static void divoom_astrootoo_main_menu_update_header_source(Pdivoom_astrootoo_main_menu_ctrl phandle)
{
    int pick;

    if (!phandle || !phandle->header_source_img)
    {
        return;
    }

    pick = divoom_astrootoo_main_menu_pick_source_icon();
    if (pick < 0 || pick >= DIVOOM_ASTRO_MAIN_MENU_HEADER_SOURCE_CNT)
    {
        return;
    }
    if (!phandle->header_source_dsc[pick].data)
    {
        return;
    }
    if (pick == phandle->header_source_icon_idx)
    {
        return;
    }

    phandle->header_source_icon_idx = pick;
    lv_img_set_src(phandle->header_source_img, &phandle->header_source_dsc[pick]);
}

static void divoom_astrootoo_main_menu_tws_fill_rect(uint8 *rgb, uint8 *alpha,
    int canvas_w, int canvas_h, uint8 x, uint8 y, uint8 w, uint8 h, uint16 color)
{
    int ix;
    int iy;

    if (!rgb || !alpha)
    {
        return;
    }
    for (iy = 0; iy < (int)h; iy++)
    {
        int py = (int)y + iy;

        if (py < 0 || py >= canvas_h)
        {
            continue;
        }
        for (ix = 0; ix < (int)w; ix++)
        {
            int px = (int)x + ix;
            int idx;

            if (px < 0 || px >= canvas_w)
            {
                continue;
            }
            idx = py * canvas_w + px;
            divoom_store_rgb565_le(&rgb[idx * 2], color);
            alpha[idx] = 0xFF;
        }
    }
}

static void divoom_astrootoo_main_menu_tws_fill_rects(uint8 *rgb, uint8 *alpha,
    const divoom_astrootoo_main_tws_rect_t *rects, int n, uint16 color)
{
    int i;

    for (i = 0; i < n; i++)
    {
        divoom_astrootoo_main_menu_tws_fill_rect(rgb, alpha,
            DIVOOM_ASTRO_MAIN_MENU_HEADER_TWS_W, DIVOOM_ASTRO_MAIN_MENU_HEADER_TWS_H,
            rects[i].x, rects[i].y, rects[i].w, rects[i].h, color);
    }
}

/* 真源 status-tws-l/r.svg 43×12 像素块 */
static void divoom_astrootoo_main_menu_tws_blit(uint8 *buf, uint8 is_right)
{
    static const divoom_astrootoo_main_tws_rect_t k_body[] = {
        {3, 0, 2, 12}, {0, 0, 8, 2},
        {10, 0, 2, 10}, {18, 0, 2, 10}, {14, 0, 2, 10}, {12, 10, 2, 2}, {16, 10, 2, 2},
        {24, 0, 4, 2}, {24, 5, 4, 2}, {24, 10, 4, 2},
        {22, 2, 2, 3}, {28, 2, 2, 2}, {28, 7, 2, 3}, {22, 8, 2, 2},
        {31, 5, 2, 2},
    };
    static const divoom_astrootoo_main_tws_rect_t k_l[] = {
        {35, 0, 2, 12}, {37, 10, 5, 2},
    };
    static const divoom_astrootoo_main_tws_rect_t k_r[] = {
        {35, 0, 6, 2}, {41, 2, 2, 4}, {37, 6, 4, 2},
        {40, 8, 2, 2}, {41, 10, 2, 2}, {35, 2, 2, 10},
    };
    uint8 *rgb;
    uint8 *alpha;
    uint16 body_c;
    uint16 ch_c;
    uint32 px;

    if (!buf)
    {
        return;
    }
    px = (uint32)DIVOOM_ASTRO_MAIN_MENU_HEADER_TWS_W * DIVOOM_ASTRO_MAIN_MENU_HEADER_TWS_H;
    memset(buf, 0, px * 3u);
    rgb = buf;
    alpha = buf + px * 2u;
    body_c = lv_color_to_u16(lv_color_hex(DIVOOM_ASTRO_MAIN_MENU_HEADER_TWS_FG));
    ch_c = lv_color_to_u16(lv_color_hex(DIVOOM_ASTRO_MAIN_MENU_HEADER_TWS_CH));
    divoom_astrootoo_main_menu_tws_fill_rects(rgb, alpha, k_body,
        (int)(sizeof(k_body) / sizeof(k_body[0])), body_c);
    if (is_right)
    {
        divoom_astrootoo_main_menu_tws_fill_rects(rgb, alpha, k_r,
            (int)(sizeof(k_r) / sizeof(k_r[0])), ch_c);
    }
    else
    {
        divoom_astrootoo_main_menu_tws_fill_rects(rgb, alpha, k_l,
            (int)(sizeof(k_l) / sizeof(k_l[0])), ch_c);
    }
}

static int divoom_astrootoo_main_menu_tws_ensure_dsc(Pdivoom_astrootoo_main_menu_ctrl phandle)
{
    uint32 size;

    if (!phandle)
    {
        return 0;
    }
    if (phandle->header_tws_dsc.data)
    {
        return 1;
    }
    size = divoom_pixel_format_size(DIVOOM_ASTRO_MAIN_MENU_HEADER_TWS_W,
        DIVOOM_ASTRO_MAIN_MENU_HEADER_TWS_H, LV_COLOR_FORMAT_RGB565A8);
    if (size == 0)
    {
        return 0;
    }
    phandle->header_tws_dsc.data = divoom_system_malloc(size);
    if (!phandle->header_tws_dsc.data)
    {
        return 0;
    }
    divoom_lv_img_dsc_init_fmt(&phandle->header_tws_dsc,
        DIVOOM_ASTRO_MAIN_MENU_HEADER_TWS_W, DIVOOM_ASTRO_MAIN_MENU_HEADER_TWS_H,
        LV_COLOR_FORMAT_RGB565A8, (void *)phandle->header_tws_dsc.data);
    phandle->header_tws_dsc.header.stride = divoom_pixel_format_stride(
        DIVOOM_ASTRO_MAIN_MENU_HEADER_TWS_W, LV_COLOR_FORMAT_RGB565A8);
    return 1;
}

static void divoom_astrootoo_main_menu_update_header_tws(Pdivoom_astrootoo_main_menu_ctrl phandle)
{
    uint8 role;

    if (!phandle || !phandle->header_tws_img)
    {
        return;
    }
    role = divoom_tws_host_linked_role();
    if (role != DIVOOM_BTCORE_TWS_ROLE_LEFT && role != DIVOOM_BTCORE_TWS_ROLE_RIGHT)
    {
        if (phandle->header_tws_role != 0)
        {
            lv_obj_add_flag(phandle->header_tws_img, LV_OBJ_FLAG_HIDDEN);
            phandle->header_tws_role = 0;
        }
        return;
    }
    if (role == phandle->header_tws_role)
    {
        return;
    }
    if (!divoom_astrootoo_main_menu_tws_ensure_dsc(phandle))
    {
        return;
    }
    divoom_astrootoo_main_menu_tws_blit((uint8 *)phandle->header_tws_dsc.data,
        (role == DIVOOM_BTCORE_TWS_ROLE_RIGHT) ? 1 : 0);
    lv_img_set_src(phandle->header_tws_img, &phandle->header_tws_dsc);
    lv_obj_clear_flag(phandle->header_tws_img, LV_OBJ_FLAG_HIDDEN);
    lv_obj_invalidate(phandle->header_tws_img);
    phandle->header_tws_role = role;
}

/* 嵌入式 WiFi：满格 -55，比手机侧 -45 更贴近 AIC8800 实测 */
static uint8 divoom_astrootoo_main_menu_wifi_rssi_bars(int rssi)
{
    if (rssi == 0 || rssi <= -100)
    {
        return 1;
    }
    if (rssi >= -55)
    {
        return 4;
    }
    if (rssi >= -65)
    {
        return 3;
    }
    if (rssi >= -75)
    {
        return 2;
    }
    return 1;
}

static uint8 divoom_astrootoo_main_menu_wifi_icon_sig(void)
{
    uint8 connected;
    uint8 bars;
    int8 rssi;

    connected = divoom_wifi_model_get_sta_link_connected() ? 1u : 0u;
    if (!connected)
    {
        return 0u;
    }
    if (divoom_wifi_model_get_sta_rssi(&rssi) != 0)
    {
        bars = 1;
    }
    else
    {
        bars = divoom_astrootoo_main_menu_wifi_rssi_bars((int)rssi);
    }
    if (bars < 1)
    {
        bars = 1;
    }
    if (bars > 4)
    {
        bars = 4;
    }
    return bars;
}

static void divoom_astrootoo_main_menu_update_header_wifi(Pdivoom_astrootoo_main_menu_ctrl phandle)
{
    uint8 sig;
    uint8 pick;

    if (!phandle || !phandle->header_wifi_img)
    {
        return;
    }

    divoom_wifi_model_refresh_sta_link();
    sig = divoom_astrootoo_main_menu_wifi_icon_sig();

    pick = sig;
    if (pick >= DIVOOM_ASTRO_MAIN_MENU_HEADER_WIFI_CNT)
    {
        pick = (uint8)(DIVOOM_ASTRO_MAIN_MENU_HEADER_WIFI_CNT - 1);
    }
    if (!phandle->header_wifi_dsc[pick].data)
    {
        return;
    }

    if (sig == phandle->header_wifi_icon_sig)
    {
        const void *src = lv_img_get_src(phandle->header_wifi_img);

        if (src == &phandle->header_wifi_dsc[pick])
        {
            return;
        }
    }

    phandle->header_wifi_icon_sig = sig;
    lv_img_set_src(phandle->header_wifi_img, &phandle->header_wifi_dsc[pick]);
    lv_obj_invalidate(phandle->header_wifi_img);
}

static uint8 divoom_astrootoo_main_menu_battery_icon_sig(void)
{
    int level;
    uint8 charge;

    level = divoom_get_cur_battery_level();
    if (level < 0)
    {
        level = 0;
    }
    if (level > 9)
    {
        level = 9;
    }
    charge = divoom_is_charge_sta() ? 1u : 0u;
    return (uint8)(((uint8)level << 1) | charge);
}

static void divoom_astrootoo_main_menu_fill_bat_rect(lv_layer_t *layer, const lv_area_t *base,
    lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h, uint32 color)
{
    lv_draw_rect_dsc_t dsc;
    lv_area_t area;

    if (!layer || !base || w <= 0 || h <= 0)
    {
        return;
    }

    lv_draw_rect_dsc_init(&dsc);
    dsc.bg_color = lv_color_hex(color);
    dsc.bg_opa = LV_OPA_COVER;
    dsc.border_width = 0;
    dsc.radius = 0;
    area.x1 = base->x1 + x;
    area.y1 = base->y1 + y;
    area.x2 = area.x1 + w - 1;
    area.y2 = area.y1 + h - 1;
    lv_draw_rect(layer, &dsc, &area);
}

/* 外框仍按出厂 PNG；内腔按 phone 0～9 画连续色块，无格间分隔。 */
static void divoom_astrootoo_main_menu_header_battery_draw_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_main_menu_ctrl phandle;
    lv_layer_t *layer;
    lv_obj_t *obj;
    lv_area_t coords;
    uint8 sig;
    int level;
    int fill_w;
    uint32 fill;

    if (lv_event_get_code(e) != LV_EVENT_DRAW_MAIN)
    {
        return;
    }

    phandle = (Pdivoom_astrootoo_main_menu_ctrl)lv_event_get_user_data(e);
    obj = lv_event_get_target(e);
    layer = lv_event_get_layer(e);
    if (!phandle || !obj || !layer)
    {
        return;
    }

    lv_obj_get_coords(obj, &coords);
    sig = phandle->header_battery_icon_sig;
    if (sig == 0xFF)
    {
        sig = divoom_astrootoo_main_menu_battery_icon_sig();
    }
    level = (int)(sig >> 1);
    if (level < 0)
    {
        level = 0;
    }
    if (level > DIVOOM_ASTRO_MAIN_BAT_LEVEL_MAX)
    {
        level = DIVOOM_ASTRO_MAIN_BAT_LEVEL_MAX;
    }
    fill_w = (level + 1) * DIVOOM_ASTRO_MAIN_BAT_FILL_W / (DIVOOM_ASTRO_MAIN_BAT_LEVEL_MAX + 1);

    /* 充电绿；放电 10% 红；放电 20%～100% 米色。 */
    if (sig & 1)
    {
        fill = DIVOOM_ASTRO_MAIN_BAT_GREEN_COLOR;
    }
    else if (level == 0)
    {
        fill = DIVOOM_ASTRO_MAIN_BAT_RED_COLOR;
    }
    else
    {
        fill = DIVOOM_ASTRO_MAIN_BAT_FRAME_COLOR;
    }

    divoom_astrootoo_main_menu_fill_bat_rect(layer, &coords, 1, 0, 22, 1,
        DIVOOM_ASTRO_MAIN_BAT_FRAME_COLOR);
    divoom_astrootoo_main_menu_fill_bat_rect(layer, &coords, 1, 13, 22, 1,
        DIVOOM_ASTRO_MAIN_BAT_FRAME_COLOR);
    divoom_astrootoo_main_menu_fill_bat_rect(layer, &coords, 0, 1, 1, 12,
        DIVOOM_ASTRO_MAIN_BAT_FRAME_COLOR);
    divoom_astrootoo_main_menu_fill_bat_rect(layer, &coords, 23, 1, 1, 12,
        DIVOOM_ASTRO_MAIN_BAT_FRAME_COLOR);
    divoom_astrootoo_main_menu_fill_bat_rect(layer, &coords, 24, 5, 2, 4,
        DIVOOM_ASTRO_MAIN_BAT_FRAME_COLOR);
    divoom_astrootoo_main_menu_fill_bat_rect(layer, &coords,
        DIVOOM_ASTRO_MAIN_BAT_FILL_X, DIVOOM_ASTRO_MAIN_BAT_FILL_Y,
        fill_w, DIVOOM_ASTRO_MAIN_BAT_FILL_H, fill);
}

static void divoom_astrootoo_main_menu_update_header_battery(Pdivoom_astrootoo_main_menu_ctrl phandle)
{
    uint8 sig;

    if (!phandle || !phandle->header_battery_img)
    {
        return;
    }

    sig = divoom_astrootoo_main_menu_battery_icon_sig();
    if (sig == phandle->header_battery_icon_sig)
    {
        return;
    }

    phandle->header_battery_icon_sig = sig;
    lv_obj_invalidate(phandle->header_battery_img);
}

#if DIVOOM_MAIN_MENU_SHOW_BAT_MV
/* 顶栏调试：杰理 0x34 电池 mV，无单位（6800 = 6.8V） */
static void divoom_astrootoo_main_menu_update_header_volt(Pdivoom_astrootoo_main_menu_ctrl phandle)
{
    uint16 mv;

    if (!phandle || !phandle->header_volt_label)
    {
        return;
    }

    mv = PowerVolGet();
    if (mv == phandle->header_volt_mv)
    {
        return;
    }
    phandle->header_volt_mv = mv;
    lv_label_set_text_fmt(phandle->header_volt_label, "%u", (unsigned)mv);
}
#endif

static void divoom_astrootoo_main_menu_update_header_status(Pdivoom_astrootoo_main_menu_ctrl phandle)
{
    divoom_astrootoo_main_menu_update_header_wifi(phandle);
    divoom_astrootoo_main_menu_update_header_battery(phandle);
#if DIVOOM_MAIN_MENU_SHOW_BAT_MV
    divoom_astrootoo_main_menu_update_header_volt(phandle);
#endif
}

static void divoom_astrootoo_main_menu_update_time(Pdivoom_astrootoo_main_menu_ctrl phandle)
{
    time_t now;
    struct tm *tm_info;
    char buf[32];

    if (!phandle || !phandle->status_time_label)
    {
        return;
    }

    time(&now);
    tm_info = localtime(&now);
    if (!tm_info)
    {
        return;
    }

    divoom_astrootoo_time_pick_format_clock_label(buf, (int)sizeof(buf),
        tm_info->tm_hour, tm_info->tm_min);
    lv_label_set_text(phandle->status_time_label, buf);
}

static void divoom_astrootoo_main_menu_tick_cb(lv_timer_t *timer)
{
    Pdivoom_astrootoo_main_menu_ctrl phandle;

    phandle = (Pdivoom_astrootoo_main_menu_ctrl)lv_timer_get_user_data(timer);
    divoom_astrootoo_main_menu_update_time(phandle);
    divoom_astrootoo_main_menu_update_header_source(phandle);
    divoom_astrootoo_main_menu_update_header_tws(phandle);
    divoom_astrootoo_main_menu_update_header_status(phandle);
}

static float divoom_astrootoo_main_menu_wrap_dist(float d, int n)
{
    float half = (float)n / 2.0f;

    while (d > half)
    {
        d -= (float)n;
    }
    while (d <= -half)
    {
        d += (float)n;
    }
    return d;
}

static void divoom_astrootoo_main_menu_layout(Pdivoom_astrootoo_main_menu_ctrl phandle)
{
    int i;
    int n;
    int top_item = -1;
    float top_abs = 1e9f;
    float limit;

    if (!phandle || phandle->item_count <= 0 || !phandle->icons_loaded)
    {
        return;
    }

    n = phandle->item_count;
    limit = DIVOOM_ASTRO_MAIN_VISIBLE_D
        + (phandle->anim_active ? DIVOOM_ASTRO_MAIN_ANIM_BLEED : 0.0f);

    for (i = 0; i < n; i++)
    {
        divoom_astrootoo_main_slide_t *slide = &phandle->slides[i];
        float d;
        float a;
        float edge;
        lv_coord_t cw;
        lv_coord_t ch;
        lv_coord_t cx;
        float scale;
        uint16_t zoom;
        uint8_t opa;
        float m;

        if (!slide->slide)
        {
            continue;
        }

        d = divoom_astrootoo_main_menu_wrap_dist((float)i - phandle->cur, n);
        a = fabsf(d);
        edge = divoom_astrootoo_main_menu_edge_reveal(a, phandle->anim_active);

        if (edge <= 0.0f && a > limit + 0.001f)
        {
            lv_obj_add_flag(slide->slide, LV_OBJ_FLAG_HIDDEN);
            continue;
        }
        lv_obj_clear_flag(slide->slide, LV_OBJ_FLAG_HIDDEN);

        divoom_astrootoo_main_menu_interpolated_dims(a, &cw, &ch);
        cx = (lv_coord_t)(DIVOOM_ASTRO_MAIN_CENTER_X
            + divoom_astrootoo_main_menu_center_offset_for_distance(d));
        scale = (float)cw / (float)DIVOOM_ASTRO_MAIN_CARD_LG_W;
        zoom = (uint16_t)(256.0f * scale);
        m = (a < 1.0f) ? a : 1.0f;
        opa = (uint8_t)(255.0f * edge * (1.0f - (1.0f - (float)DIVOOM_ASTRO_MAIN_PEEK_OPA / 255.0f) * m));

        lv_obj_set_size(slide->slide, DIVOOM_ASTRO_MAIN_CARD_LG_W, DIVOOM_ASTRO_MAIN_CARD_LG_H);
        lv_obj_set_pos(slide->slide,
            cx - DIVOOM_ASTRO_MAIN_CARD_LG_W / 2,
            DIVOOM_ASTRO_MAIN_CARD_CY - DIVOOM_ASTRO_MAIN_CARD_LG_H / 2);

        if (slide->card_img)
        {
            lv_img_set_zoom(slide->card_img, zoom);
            lv_img_set_pivot(slide->card_img,
                DIVOOM_ASTRO_MAIN_CARD_LG_W / 2, DIVOOM_ASTRO_MAIN_CARD_LG_H / 2);
            lv_obj_set_style_opa(slide->card_img, opa, 0);
        }

        if (slide->focus_frame)
        {
            lv_coord_t fw = cw + DIVOOM_ASTRO_MAIN_FOCUS_FRAME_OUTSET * 2;
            lv_coord_t fh = ch + DIVOOM_ASTRO_MAIN_FOCUS_FRAME_OUTSET * 2;
            uint16_t frame_zoom = (uint16_t)(256.0f * (float)fw / 120.0f);

            if (a < 0.08f)
            {
                lv_obj_clear_flag(slide->focus_frame, LV_OBJ_FLAG_HIDDEN);
                lv_obj_set_size(slide->focus_frame, 120, 202);
                lv_img_set_zoom(slide->focus_frame, frame_zoom);
                lv_img_set_pivot(slide->focus_frame, 60, 101);
                lv_obj_set_pos(slide->focus_frame,
                    (DIVOOM_ASTRO_MAIN_CARD_LG_W - fw) / 2,
                    (DIVOOM_ASTRO_MAIN_CARD_LG_H - fh) / 2);
                lv_obj_set_style_opa(slide->focus_frame, opa, 0);
            }
            else
            {
                lv_obj_add_flag(slide->focus_frame, LV_OBJ_FLAG_HIDDEN);
            }
        }

        if (a < top_abs)
        {
            top_abs = a;
            top_item = i;
        }
    }

    if (top_item >= 0 && phandle->slides[top_item].slide)
    {
        lv_obj_move_foreground(phandle->slides[top_item].slide);
    }
}

static void divoom_astrootoo_main_menu_anim_cb(void *var, int32_t v)
{
    Pdivoom_astrootoo_main_menu_ctrl phandle = (Pdivoom_astrootoo_main_menu_ctrl)var;

    if (!phandle)
    {
        return;
    }
    phandle->cur = (float)v / (float)DIVOOM_ASTRO_MAIN_CUR_SCALE;
    divoom_astrootoo_main_menu_layout(phandle);
}

static void divoom_astrootoo_main_menu_anim_ready_cb(lv_anim_t *a)
{
    Pdivoom_astrootoo_main_menu_ctrl phandle = (Pdivoom_astrootoo_main_menu_ctrl)a->var;

    if (!phandle)
    {
        return;
    }
    phandle->anim_active = 0;
    divoom_astrootoo_main_menu_layout(phandle);
}

static void divoom_astrootoo_main_menu_animate_to_target_ms(Pdivoom_astrootoo_main_menu_ctrl phandle, uint32_t anim_ms)
{
    lv_anim_t a;
    int32_t from;
    int32_t to;

    if (!phandle)
    {
        return;
    }

    lv_anim_del(phandle, divoom_astrootoo_main_menu_anim_cb);
    from = (int32_t)(phandle->cur * DIVOOM_ASTRO_MAIN_CUR_SCALE);
    to = (int32_t)(phandle->cur_target * DIVOOM_ASTRO_MAIN_CUR_SCALE);
    phandle->anim_active = 1;

    lv_anim_init(&a);
    lv_anim_set_var(&a, phandle);
    lv_anim_set_exec_cb(&a, divoom_astrootoo_main_menu_anim_cb);
    lv_anim_set_values(&a, from, to);
    lv_anim_set_time(&a, anim_ms);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_set_ready_cb(&a, divoom_astrootoo_main_menu_anim_ready_cb);
    lv_anim_start(&a);
}

static void divoom_astrootoo_main_menu_animate_to_target(Pdivoom_astrootoo_main_menu_ctrl phandle)
{
    divoom_astrootoo_main_menu_animate_to_target_ms(phandle, DIVOOM_ASTRO_MAIN_ANIM_MS);
}

static void divoom_astrootoo_main_menu_commit_focus(Pdivoom_astrootoo_main_menu_ctrl phandle)
{
    int n = phandle->item_count;
    long r;
    const char *label_key;
    const char *label_text;

    if (n <= 0)
    {
        return;
    }
    r = lroundf(phandle->cur_target);
    phandle->focus_index = (int)(((r % n) + n) % n);
    label_key = g_astrootoo_main_items[phandle->focus_index].label_key;
    label_text = divoom_menu_control_get_disp_str(label_key);
    lv_label_set_text(phandle->caption_label, label_text);
}

static void divoom_astrootoo_main_menu_load_next_icon(Pdivoom_astrootoo_main_menu_ctrl phandle)
{
    divoom_astrootoo_main_slide_t *slide;
    lv_obj_t *img;
    char path[96];
    char rel[48];
    int item;
    int i;

    if (!phandle || phandle->icons_loaded)
    {
        return;
    }
    if (divoom_menu_control_get_clock_mode_menu_handle() != phandle)
    {
        return;
    }

    if (phandle->load_bg_pending)
    {
        snprintf(rel, sizeof(rel), "%s%s",
            DIVOOM_ASTROTOO_MAIN_MENU_UI_DIR, DIVOOM_ASTROTOO_MAIN_MENU_BG_FILE);
        if (divoom_astrootoo_main_menu_cache_png_rel(rel, &phandle->bg_dsc)
            && phandle->bg_img && phandle->bg_dsc.data)
        {
            lv_img_set_src(phandle->bg_img, &phandle->bg_dsc);
        }
        phandle->load_bg_pending = 0;
        return;
    }

    if (phandle->icon_load_item < phandle->item_count)
    {
        item = phandle->icon_load_item;
        slide = &phandle->slides[item];
        img = slide->card_img;

        if (!img || !lv_obj_is_valid(img))
        {
            phandle->icon_load_item++;
            return;
        }

        divoom_astrootoo_main_menu_build_card_src(path, sizeof(path),
            g_astrootoo_main_items[item].slug);
        if (divoom_astrootoo_main_menu_cache_png_rel(path, &phandle->card_dsc[item]))
        {
            lv_img_set_src(img, &phandle->card_dsc[item]);
        }

        phandle->icon_load_item++;
        divoom_astrootoo_main_menu_layout(phandle);
        return;
    }

    if (!phandle->focus_load_done)
    {
        phandle->focus_load_done = 1;
        snprintf(rel, sizeof(rel), "%s%s",
            DIVOOM_ASTROTOO_MAIN_MENU_UI_DIR, DIVOOM_ASTROTOO_MAIN_MENU_FOCUS_FRAME_FILE);
        if (divoom_astrootoo_main_menu_cache_png_rel(rel, &phandle->focus_frame_dsc))
        {
            for (i = 0; i < phandle->item_count; i++)
            {
                if (phandle->slides[i].focus_frame)
                {
                    lv_img_set_src(phandle->slides[i].focus_frame, &phandle->focus_frame_dsc);
                }
            }
        }
        divoom_astrootoo_main_menu_layout(phandle);
        return;
    }

    if (phandle->load_nav_pending)
    {
        snprintf(rel, sizeof(rel), "%s%s",
            DIVOOM_ASTROTOO_MAIN_MENU_UI_DIR, DIVOOM_ASTROTOO_MAIN_MENU_NAV_CHEVRON_L_FILE);
        if (divoom_astrootoo_main_menu_cache_png_rel(rel, &phandle->nav_chevron_l_dsc)
            && phandle->nav_chevron_left && phandle->nav_chevron_l_dsc.data)
        {
            lv_img_set_src(phandle->nav_chevron_left, &phandle->nav_chevron_l_dsc);
        }
        snprintf(rel, sizeof(rel), "%s%s",
            DIVOOM_ASTROTOO_MAIN_MENU_UI_DIR, DIVOOM_ASTROTOO_MAIN_MENU_NAV_CHEVRON_R_FILE);
        if (divoom_astrootoo_main_menu_cache_png_rel(rel, &phandle->nav_chevron_r_dsc)
            && phandle->nav_chevron_right && phandle->nav_chevron_r_dsc.data)
        {
            lv_img_set_src(phandle->nav_chevron_right, &phandle->nav_chevron_r_dsc);
        }
        phandle->load_nav_pending = 0;
        divoom_astrootoo_main_menu_layout_nav_hint(phandle);
        return;
    }

    if (phandle->header_source_load_idx < DIVOOM_ASTRO_MAIN_MENU_HEADER_SOURCE_CNT)
    {
        snprintf(rel, sizeof(rel), "%s%s",
            DIVOOM_ASTROTOO_MAIN_MENU_UI_DIR,
            g_astrootoo_main_header_source_files[phandle->header_source_load_idx]);
        if (divoom_astrootoo_main_menu_cache_png_rel(rel,
            &phandle->header_source_dsc[phandle->header_source_load_idx]))
        {
            phandle->header_source_load_idx++;
        }
        else
        {
            phandle->header_source_load_idx++;
        }
        if (phandle->header_source_load_idx >= DIVOOM_ASTRO_MAIN_MENU_HEADER_SOURCE_CNT)
        {
            divoom_astrootoo_main_menu_update_header_source(phandle);
        }
        return;
    }

    if (phandle->header_wifi_load_idx < DIVOOM_ASTRO_MAIN_MENU_HEADER_WIFI_CNT)
    {
        snprintf(rel, sizeof(rel), "%s%s",
            DIVOOM_ASTROTOO_MAIN_MENU_UI_DIR,
            g_astrootoo_main_header_wifi_files[phandle->header_wifi_load_idx]);
        if (divoom_astrootoo_main_menu_cache_png_rel(rel,
            &phandle->header_wifi_dsc[phandle->header_wifi_load_idx]))
        {
            phandle->header_wifi_load_idx++;
            divoom_astrootoo_main_menu_update_header_wifi(phandle);
        }
        else
        {
            phandle->header_wifi_load_idx++;
        }
        if (phandle->header_wifi_load_idx >= DIVOOM_ASTRO_MAIN_MENU_HEADER_WIFI_CNT)
        {
            phandle->icons_loaded = 1;
            divoom_astrootoo_main_menu_update_header_status(phandle);
            divoom_astrootoo_main_menu_layout_nav_hint(phandle);
            divoom_astrootoo_main_menu_layout(phandle);
        }
        return;
    }
}

static void divoom_astrootoo_main_menu_load_icons_cb(lv_timer_t *timer);

static uint8 divoom_astrootoo_main_menu_has_png_holes(Pdivoom_astrootoo_main_menu_ctrl phandle)
{
    int i;

    if (!phandle)
    {
        return 0;
    }
    if (phandle->bg_dsc.data == NULL ||
        phandle->focus_frame_dsc.data == NULL ||
        phandle->nav_chevron_l_dsc.data == NULL ||
        phandle->nav_chevron_r_dsc.data == NULL)
    {
        return 1;
    }
    for (i = 0; i < phandle->item_count; i++)
    {
        if (phandle->card_dsc[i].data == NULL)
        {
            return 1;
        }
    }
    for (i = 0; i < DIVOOM_ASTRO_MAIN_MENU_HEADER_SOURCE_CNT; i++)
    {
        if (phandle->header_source_dsc[i].data == NULL)
        {
            return 1;
        }
    }
    for (i = 0; i < DIVOOM_ASTRO_MAIN_MENU_HEADER_WIFI_CNT; i++)
    {
        if (phandle->header_wifi_dsc[i].data == NULL)
        {
            return 1;
        }
    }
    return 0;
}

static uint8 divoom_astrootoo_main_menu_cache_png_if_empty(const char *rel,
    lv_img_dsc_t *img_cache, lv_obj_t *img)
{
    if (!rel || !img_cache || img_cache->data != NULL)
    {
        return 1;
    }
    if (!divoom_astrootoo_main_menu_cache_png_rel(rel, img_cache) ||
        img_cache->data == NULL)
    {
        return 0;
    }
    if (img && lv_obj_is_valid(img))
    {
        lv_img_set_src(img, img_cache);
    }
    return 1;
}

static void divoom_astrootoo_main_menu_retry_missing_pngs(Pdivoom_astrootoo_main_menu_ctrl phandle)
{
    char path[96];
    char rel[48];
    int i;

    if (!phandle || !phandle->root_obj || !lv_obj_is_valid(phandle->root_obj))
    {
        return;
    }

    snprintf(rel, sizeof(rel), "%s%s",
        DIVOOM_ASTROTOO_MAIN_MENU_UI_DIR, DIVOOM_ASTROTOO_MAIN_MENU_BG_FILE);
    (void)divoom_astrootoo_main_menu_cache_png_if_empty(rel, &phandle->bg_dsc,
        phandle->bg_img);

    for (i = 0; i < phandle->item_count; i++)
    {
        divoom_astrootoo_main_menu_build_card_src(path, sizeof(path),
            g_astrootoo_main_items[i].slug);
        (void)divoom_astrootoo_main_menu_cache_png_if_empty(path,
            &phandle->card_dsc[i], phandle->slides[i].card_img);
    }

    snprintf(rel, sizeof(rel), "%s%s",
        DIVOOM_ASTROTOO_MAIN_MENU_UI_DIR, DIVOOM_ASTROTOO_MAIN_MENU_FOCUS_FRAME_FILE);
    if (phandle->focus_frame_dsc.data == NULL &&
        divoom_astrootoo_main_menu_cache_png_rel(rel, &phandle->focus_frame_dsc) &&
        phandle->focus_frame_dsc.data)
    {
        for (i = 0; i < phandle->item_count; i++)
        {
            if (phandle->slides[i].focus_frame &&
                lv_obj_is_valid(phandle->slides[i].focus_frame))
            {
                lv_img_set_src(phandle->slides[i].focus_frame,
                    &phandle->focus_frame_dsc);
            }
        }
    }

    snprintf(rel, sizeof(rel), "%s%s",
        DIVOOM_ASTROTOO_MAIN_MENU_UI_DIR, DIVOOM_ASTROTOO_MAIN_MENU_NAV_CHEVRON_L_FILE);
    (void)divoom_astrootoo_main_menu_cache_png_if_empty(rel,
        &phandle->nav_chevron_l_dsc, phandle->nav_chevron_left);
    snprintf(rel, sizeof(rel), "%s%s",
        DIVOOM_ASTROTOO_MAIN_MENU_UI_DIR, DIVOOM_ASTROTOO_MAIN_MENU_NAV_CHEVRON_R_FILE);
    (void)divoom_astrootoo_main_menu_cache_png_if_empty(rel,
        &phandle->nav_chevron_r_dsc, phandle->nav_chevron_right);

    for (i = 0; i < DIVOOM_ASTRO_MAIN_MENU_HEADER_SOURCE_CNT; i++)
    {
        snprintf(rel, sizeof(rel), "%s%s",
            DIVOOM_ASTROTOO_MAIN_MENU_UI_DIR,
            g_astrootoo_main_header_source_files[i]);
        (void)divoom_astrootoo_main_menu_cache_png_if_empty(rel,
            &phandle->header_source_dsc[i], NULL);
    }
    for (i = 0; i < DIVOOM_ASTRO_MAIN_MENU_HEADER_WIFI_CNT; i++)
    {
        snprintf(rel, sizeof(rel), "%s%s",
            DIVOOM_ASTROTOO_MAIN_MENU_UI_DIR,
            g_astrootoo_main_header_wifi_files[i]);
        (void)divoom_astrootoo_main_menu_cache_png_if_empty(rel,
            &phandle->header_wifi_dsc[i], NULL);
    }

    divoom_astrootoo_main_menu_update_header_source(phandle);
    divoom_astrootoo_main_menu_update_header_wifi(phandle);
    divoom_astrootoo_main_menu_update_header_status(phandle);
    divoom_astrootoo_main_menu_layout_nav_hint(phandle);
    divoom_astrootoo_main_menu_layout(phandle);
}

static void divoom_astrootoo_main_menu_stop_load_timer(Pdivoom_astrootoo_main_menu_ctrl phandle,
    lv_timer_t *timer)
{
    if (phandle)
    {
        phandle->load_timer = NULL;
        phandle->icon_retry_pending = 0;
    }
    lv_timer_del(timer);
}

static void divoom_astrootoo_main_menu_arm_icon_retry(Pdivoom_astrootoo_main_menu_ctrl phandle)
{
    if (!phandle || !divoom_astrootoo_main_menu_has_png_holes(phandle))
    {
        return;
    }

    phandle->icon_retry_pending = 1;
    phandle->icon_retry_start_ms = divoom_system_get_tickms();
    if (phandle->load_timer)
    {
        lv_timer_set_period(phandle->load_timer, DIVOOM_ASTRO_MAIN_ICON_RETRY_PERIOD_MS);
        return;
    }
    phandle->load_timer = lv_timer_create(divoom_astrootoo_main_menu_load_icons_cb,
        DIVOOM_ASTRO_MAIN_ICON_RETRY_PERIOD_MS, phandle);
}

static void divoom_astrootoo_main_menu_scan_anim_cb(void *var, int32_t v)
{
    lv_obj_t *bar = (lv_obj_t *)var;

    if (!bar)
    {
        return;
    }
    lv_obj_set_y(bar, v);
    if (v < DIVOOM_SCREEN_WIDTH)
    {
        uint8_t opa = (uint8_t)(160 - 160 * v / DIVOOM_SCREEN_WIDTH);
        lv_obj_set_style_bg_opa(bar, opa, 0);
    }
}

static void divoom_astrootoo_main_menu_scan_ready_cb(lv_anim_t *a)
{
    lv_obj_t *bar = (lv_obj_t *)a->var;
    Pdivoom_astrootoo_main_menu_ctrl phandle;

    if (!bar)
    {
        return;
    }
    phandle = (Pdivoom_astrootoo_main_menu_ctrl)lv_obj_get_user_data(bar);
    if (phandle && phandle->scan_bar == bar)
    {
        phandle->scan_bar = NULL;
    }
    lv_obj_del(bar);
}

static void divoom_astrootoo_main_menu_play_enter_scan(Pdivoom_astrootoo_main_menu_ctrl phandle)
{
    lv_obj_t *bar;
    lv_anim_t a;

    if (!phandle || !phandle->root_obj)
    {
        return;
    }

    if (phandle->scan_bar)
    {
        lv_anim_del(phandle->scan_bar, divoom_astrootoo_main_menu_scan_anim_cb);
        lv_obj_del(phandle->scan_bar);
        phandle->scan_bar = NULL;
    }

    bar = lv_obj_create(phandle->root_obj);
    lv_obj_remove_style_all(bar);
    lv_obj_set_size(bar, DIVOOM_SCREEN_WIDTH, 96);
    lv_obj_set_pos(bar, 0, -96);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(bar, 160, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_user_data(bar, phandle);
    lv_obj_move_foreground(bar);
    phandle->scan_bar = bar;

    lv_anim_init(&a);
    lv_anim_set_var(&a, bar);
    lv_anim_set_exec_cb(&a, divoom_astrootoo_main_menu_scan_anim_cb);
    lv_anim_set_values(&a, -96, DIVOOM_SCREEN_WIDTH);
    lv_anim_set_time(&a, 200);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_set_ready_cb(&a, divoom_astrootoo_main_menu_scan_ready_cb);
    lv_anim_start(&a);
}

static void divoom_astrootoo_main_menu_load_icons_cb(lv_timer_t *timer)
{
    Pdivoom_astrootoo_main_menu_ctrl phandle;
    uint32 now_ms;
    uint8 movie_idle;

    phandle = (Pdivoom_astrootoo_main_menu_ctrl)lv_timer_get_user_data(timer);
    if (!phandle || divoom_menu_control_get_clock_mode_menu_handle() != phandle)
    {
        divoom_astrootoo_main_menu_stop_load_timer(phandle, timer);
        return;
    }
    if (!phandle->root_obj || !lv_obj_is_valid(phandle->root_obj))
    {
        divoom_astrootoo_main_menu_stop_load_timer(phandle, timer);
        return;
    }

    if (!phandle->icons_loaded)
    {
        divoom_astrootoo_main_menu_load_next_icon(phandle);
        if (!phandle->icons_loaded)
        {
            return;
        }
        if (divoom_astrootoo_main_menu_has_png_holes(phandle))
        {
            divoom_astrootoo_main_menu_arm_icon_retry(phandle);
            return;
        }
        divoom_astrootoo_main_menu_stop_load_timer(phandle, timer);
        return;
    }

    if (!phandle->icon_retry_pending ||
        !divoom_astrootoo_main_menu_has_png_holes(phandle))
    {
        divoom_astrootoo_main_menu_stop_load_timer(phandle, timer);
        return;
    }

    now_ms = divoom_system_get_tickms();
#if defined(DIVOOM_SIMULATOR) && DIVOOM_SIMULATOR
    movie_idle = 1;
#else
    movie_idle = divoom_local_movie_play_is_idle() ? 1 : 0;
#endif
    if (!movie_idle)
    {
        if ((uint32)(now_ms - phandle->icon_retry_start_ms) >=
            DIVOOM_ASTRO_MAIN_ICON_RETRY_WAIT_MS)
        {
            divoom_print("main menu icon retry give up, movie busy\n");
            divoom_astrootoo_main_menu_stop_load_timer(phandle, timer);
        }
        return;
    }
    if ((uint32)(now_ms - phandle->icon_retry_start_ms) <
        DIVOOM_ASTRO_MAIN_ICON_RETRY_COOLDOWN_MS)
    {
        return;
    }

    divoom_print("main menu retry missing png\n");
    divoom_astrootoo_main_menu_retry_missing_pngs(phandle);
    divoom_astrootoo_main_menu_stop_load_timer(phandle, timer);
}

static void divoom_astrootoo_main_menu_apply_target(Pdivoom_astrootoo_main_menu_ctrl phandle, uint8_t press)
{
    uint32_t anim_ms;

    if (!phandle)
    {
        return;
    }

    divoom_astrootoo_main_menu_commit_focus(phandle);
    anim_ms = (press == DIVOOM_PRESS_SHORT || press == DIVOOM_NO_PRESS)
        ? DIVOOM_ASTRO_MAIN_ANIM_MS
        : DIVOOM_ASTRO_MAIN_ANIM_REPEAT_MS;
    divoom_astrootoo_main_menu_animate_to_target_ms(phandle, anim_ms);
}

static void divoom_astrootoo_main_menu_key_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_main_menu_ctrl phandle;
    uint8_t key_code;

    phandle = (Pdivoom_astrootoo_main_menu_ctrl)lv_event_get_user_data(e);
    if (!phandle || lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }

    key_code = divoom_astrootoo_ui_key_code_from_event(e);
    if (!divoom_astrootoo_ui_key_is_menu_press(e, key_code))
    {
        return;
    }

    if (divoom_astrootoo_ui_key_is_esc(key_code))
    {
        /* 表盘 clearall 进主菜单后栈为空，return_pre 仅投 event 77 在配网 schedule 忙时会失效。 */
        if (divoom_menu_control_get_stack_depth() == 0)
        {
            divoom_main_return_startup_dial();
        }
        else
        {
            divoom_menu_control_return_pre();
        }
        return;
    }

    if (divoom_astrootoo_ui_key_is_prev(key_code))
    {
        phandle->cur_target -= 1.0f;
        divoom_astrootoo_main_menu_apply_target(phandle, divoom_astrootoo_ui_key_press_from_event(e));
        return;
    }

    if (divoom_astrootoo_ui_key_is_next(key_code))
    {
        phandle->cur_target += 1.0f;
        divoom_astrootoo_main_menu_apply_target(phandle, divoom_astrootoo_ui_key_press_from_event(e));
        return;
    }

    if (divoom_astrootoo_ui_key_is_enter(key_code))
    {
        if (phandle->enter_guard_ms != 0
            && divoom_system_get_tickms() < phandle->enter_guard_ms)
        {
            return;
        }
        divoom_menu_control_load_next(g_astrootoo_main_items[phandle->focus_index].menu_id, NULL, 0, 0);
    }
}

static void divoom_astrootoo_main_menu_create_slides(Pdivoom_astrootoo_main_menu_ctrl phandle)
{
    int i;

    for (i = 0; i < phandle->item_count; i++)
    {
        divoom_astrootoo_main_slide_t *slide = &phandle->slides[i];
        lv_obj_t *cont;

        cont = lv_obj_create(phandle->root_obj);
        lv_obj_add_style(cont, &phandle->ui.content_style, 0);
        lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(cont, 0, 0);
        lv_obj_set_size(cont, DIVOOM_ASTRO_MAIN_CARD_LG_W, DIVOOM_ASTRO_MAIN_CARD_LG_H);
        lv_obj_add_flag(cont, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
        slide->slide = cont;

        slide->card_img = lv_img_create(cont);
        lv_obj_set_size(slide->card_img,
            DIVOOM_ASTRO_MAIN_CARD_LG_W, DIVOOM_ASTRO_MAIN_CARD_LG_H);
        lv_obj_set_pos(slide->card_img, 0, 0);

        slide->focus_frame = lv_img_create(cont);
        lv_obj_add_flag(slide->focus_frame, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_size(slide->focus_frame, 120, 202);
    }
}

void *divoom_astrootoo_main_menu_init(void *user_data, int menu_id)
{
    Pdivoom_astrootoo_main_menu_ctrl phandle;

    (void)user_data;

    phandle = divoom_system_malloc_out(sizeof(divoom_astrootoo_main_menu_ctrl));
    if (!phandle)
    {
        return NULL;
    }

    phandle->menu_id = menu_id;
    phandle->item_count = DIVOOM_ASTROTOO_MAIN_ITEM_MAX;
    if (phandle->item_count > DIVOOM_ASTRO_MAIN_ICON_MAX)
    {
        phandle->item_count = DIVOOM_ASTRO_MAIN_ICON_MAX;
    }
    phandle->focus_index = 0;
    phandle->icons_loaded = 0;
    phandle->icon_retry_pending = 0;
    phandle->icon_retry_start_ms = 0;
    phandle->load_bg_pending = 1;
    phandle->load_nav_pending = 1;
    phandle->icon_load_item = 0;
    phandle->header_source_load_idx = 0;
    phandle->header_wifi_load_idx = 0;
    phandle->header_source_icon_idx = -1;
    phandle->header_tws_role = 0xFF;
    phandle->header_wifi_icon_sig = 0xFF;
    phandle->header_battery_icon_sig = 0xFF;
#if DIVOOM_MAIN_MENU_SHOW_BAT_MV
    phandle->header_volt_label = NULL;
    phandle->header_volt_mv = 0xFFFF;
#endif
    phandle->anim_active = 0;
    phandle->cur = 0.0f;
    phandle->cur_target = 0.0f;
    phandle->root_obj = divoom_astrootoo_ui_create_root(&phandle->ui);
    lv_obj_set_style_bg_color(phandle->root_obj, lv_color_hex(0x020B18), 0);
    lv_obj_set_style_bg_opa(phandle->root_obj, LV_OPA_COVER, 0);

    phandle->bg_img = lv_img_create(phandle->root_obj);
    lv_obj_set_size(phandle->bg_img, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    lv_obj_set_pos(phandle->bg_img, 0, 0);
    lv_obj_move_background(phandle->bg_img);

    divoom_astrootoo_main_menu_create_slides(phandle);

    phandle->caption_label = lv_label_create(phandle->root_obj);
    lv_obj_set_style_text_color(phandle->caption_label,
        lv_color_hex(DIVOOM_ASTRO_MAIN_CAPTION_COLOR), 0);
    lv_obj_set_style_text_font(phandle->caption_label,
        divoom_menu_control_get_font_info(28, phandle->menu_id), 0);
    lv_obj_set_width(phandle->caption_label, DIVOOM_SCREEN_WIDTH);
    lv_obj_set_style_text_align(phandle->caption_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(phandle->caption_label, 0, DIVOOM_ASTRO_MAIN_CAPTION_Y);

    phandle->nav_hint_cont = lv_obj_create(phandle->root_obj);
    lv_obj_add_style(phandle->nav_hint_cont, &phandle->ui.content_style, 0);
    lv_obj_set_size(phandle->nav_hint_cont,
        DIVOOM_ASTRO_MAIN_NAV_HINT_W, DIVOOM_ASTRO_MAIN_NAV_HINT_H);
    lv_obj_set_pos(phandle->nav_hint_cont,
        DIVOOM_ASTRO_MAIN_NAV_HINT_X, DIVOOM_ASTRO_MAIN_NAV_HINT_Y);
    lv_obj_set_style_bg_opa(phandle->nav_hint_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(phandle->nav_hint_cont, 0, 0);
    lv_obj_clear_flag(phandle->nav_hint_cont, LV_OBJ_FLAG_SCROLLABLE);

    phandle->nav_chevron_left = lv_img_create(phandle->nav_hint_cont);
    lv_obj_set_size(phandle->nav_chevron_left,
        DIVOOM_ASTRO_MAIN_NAV_CHEVRON_W, DIVOOM_ASTRO_MAIN_NAV_CHEVRON_H);

    phandle->nav_chevron_right = lv_img_create(phandle->nav_hint_cont);
    lv_obj_set_size(phandle->nav_chevron_right,
        DIVOOM_ASTRO_MAIN_NAV_CHEVRON_W, DIVOOM_ASTRO_MAIN_NAV_CHEVRON_H);

    phandle->nav_hint_label = lv_label_create(phandle->nav_hint_cont);
    lv_obj_set_style_text_color(phandle->nav_hint_label,
        lv_color_hex(DIVOOM_ASTRO_MAIN_NAV_HINT_COLOR), 0);
    lv_obj_set_style_text_font(phandle->nav_hint_label,
        divoom_menu_control_get_font_info(16, phandle->menu_id), 0);
    lv_label_set_long_mode(phandle->nav_hint_label, LV_LABEL_LONG_CLIP);
    lv_label_set_text(phandle->nav_hint_label,
        divoom_menu_control_get_disp_str("carouselNavHintSuffix"));
    divoom_astrootoo_main_menu_layout_nav_hint(phandle);

    phandle->header_bar = divoom_astrootoo_ui_create_main_menu_header(phandle->root_obj,
        &phandle->ui, phandle->menu_id, &phandle->status_time_label, &phandle->header_source_img,
        &phandle->header_wifi_img, &phandle->header_battery_img);
    phandle->header_tws_img = lv_img_create(phandle->header_bar);
    lv_obj_set_size(phandle->header_tws_img,
        DIVOOM_ASTRO_MAIN_MENU_HEADER_TWS_W, DIVOOM_ASTRO_MAIN_MENU_HEADER_TWS_H);
    lv_obj_align(phandle->header_tws_img, LV_ALIGN_LEFT_MID,
        16 + DIVOOM_ASTRO_MAIN_MENU_HEADER_SOURCE_W + DIVOOM_ASTRO_MAIN_MENU_HEADER_TWS_GAP, 0);
    lv_obj_add_flag(phandle->header_tws_img, LV_OBJ_FLAG_HIDDEN);
    phandle->header_tws_role = 0;
    divoom_astrootoo_main_menu_update_header_tws(phandle);
    if (phandle->header_battery_img)
    {
        lv_obj_add_event(phandle->header_battery_img,
            divoom_astrootoo_main_menu_header_battery_draw_cb, LV_EVENT_DRAW_MAIN, phandle);
        divoom_astrootoo_main_menu_update_header_battery(phandle);
    }
#if DIVOOM_MAIN_MENU_SHOW_BAT_MV
    phandle->header_volt_label = lv_label_create(phandle->header_bar);
    lv_obj_set_style_text_color(phandle->header_volt_label,
        lv_color_hex(DIVOOM_ASTRO_MAIN_MENU_HEADER_FG), 0);
    lv_obj_set_style_text_font(phandle->header_volt_label,
        divoom_menu_control_get_font_info(16, phandle->menu_id), 0);
    lv_label_set_text(phandle->header_volt_label, "--");
    lv_obj_align(phandle->header_volt_label, LV_ALIGN_RIGHT_MID,
        -(16 + DIVOOM_ASTRO_MAIN_MENU_HEADER_BATTERY_W
          + DIVOOM_ASTRO_MAIN_MENU_HEADER_STATUS_GAP
          + DIVOOM_ASTRO_MAIN_MENU_HEADER_SOURCE_W
          + DIVOOM_ASTRO_MAIN_MENU_HEADER_STATUS_GAP), 0);
#endif
    lv_obj_move_foreground(phandle->header_bar);

    phandle->key_btn = divoom_astrootoo_ui_create_key_capture_btn(phandle->root_obj, &phandle->ui,
        divoom_astrootoo_main_menu_key_cb, phandle);
    lv_obj_move_foreground(phandle->key_btn);
    lv_group_add_obj(lv_group_get_default(), phandle->key_btn);

    divoom_astrootoo_main_menu_update_time(phandle);
    divoom_astrootoo_main_menu_commit_focus(phandle);
    phandle->tick_timer = lv_timer_create(divoom_astrootoo_main_menu_tick_cb, 1000, phandle);
    phandle->load_timer = lv_timer_create(divoom_astrootoo_main_menu_load_icons_cb, 16, phandle);

    return phandle;
}

void divoom_astrootoo_main_menu_disp(void *handle, void *active_obj)
{
    Pdivoom_astrootoo_main_menu_ctrl phandle;
    uint8 first_enter;

    phandle = (Pdivoom_astrootoo_main_menu_ctrl)handle;
    if (!phandle)
    {
        return;
    }

    if (phandle->free_timer)
    {
        lv_timer_del(phandle->free_timer);
        phandle->free_timer = NULL;
    }
    /* 当前页面已由 menu_control 接管；在切屏前备齐首屏资源。
     * 失败的 PNG 不挡出屏，idle 后再补解一次。 */
    if (divoom_menu_control_get_clock_mode_menu_handle() != phandle)
    {
        return;
    }
    if (phandle->load_timer)
    {
        lv_timer_del(phandle->load_timer);
        phandle->load_timer = NULL;
    }
    divoom_astrootoo_main_menu_ensure_bg_ready(phandle);
    while (!phandle->icons_loaded)
    {
        divoom_astrootoo_main_menu_load_next_icon(phandle);
    }
    divoom_astrootoo_main_menu_arm_icon_retry(phandle);
    divoom_astrootoo_main_menu_update_header_source(phandle);
    divoom_astrootoo_main_menu_update_header_tws(phandle);
    divoom_astrootoo_main_menu_update_header_status(phandle);
    divoom_astrootoo_main_menu_layout(phandle);
    lv_obj_update_layout(phandle->root_obj);
    /* 旧根屏不再强制刷新；完整菜单加载后只提交一次首帧。 */
    lv_scr_load(phandle->root_obj);
    divoom_astrootoo_main_menu_force_full_refresh(phandle->root_obj);
    phandle->enter_guard_ms = divoom_system_get_tickms() + 500u;
    if (phandle->tick_timer)
    {
        lv_timer_resume(phandle->tick_timer);
    }
    divoom_astrootoo_main_menu_update_header_source(phandle);
    divoom_astrootoo_main_menu_update_header_tws(phandle);
    divoom_astrootoo_main_menu_update_header_status(phandle);
    if (phandle->key_btn)
    {
        lv_group_t *g = lv_group_get_default();

        if (lv_obj_get_group(phandle->key_btn) != g)
        {
            lv_group_add_obj(g, phandle->key_btn);
        }
        lv_group_focus_obj(phandle->key_btn);
    }
    /* menu_control：首次 load 传 active_obj=NULL；return_pre 恢复栈页传 cur_active_obj */
    first_enter = (active_obj == NULL) ? 1u : 0u;
    (void)first_enter;
#if DIVOOM_BUILD_SIMULATOR
    if (first_enter)
    {
        divoom_astrootoo_main_menu_play_enter_scan(phandle);
    }
#endif
}

void divoom_astrootoo_main_menu_clean(void *handle)
{
    Pdivoom_astrootoo_main_menu_ctrl phandle;

    phandle = (Pdivoom_astrootoo_main_menu_ctrl)handle;
    if (!phandle)
    {
        return;
    }

    if (phandle->tick_timer)
    {
        lv_timer_del(phandle->tick_timer);
        phandle->tick_timer = NULL;
    }

    if (phandle->load_timer)
    {
        lv_timer_del(phandle->load_timer);
        phandle->load_timer = NULL;
    }
    if (phandle->free_timer)
    {
        lv_timer_del(phandle->free_timer);
        phandle->free_timer = NULL;
    }

    lv_anim_del(phandle, divoom_astrootoo_main_menu_anim_cb);
    if (phandle->scan_bar)
    {
        lv_anim_del(phandle->scan_bar, divoom_astrootoo_main_menu_scan_anim_cb);
        phandle->scan_bar = NULL;
    }

    if (phandle->root_obj)
    {
        lv_obj_del(phandle->root_obj);
        phandle->root_obj = NULL;
    }
    /* 对象树已销毁，不再有 lv_img 引用模块自有 dsc，可直接释放缓存。 */
    divoom_astrootoo_main_menu_free_img_cache(phandle, 0);

    divoom_astrootoo_ui_base_reset_styles(&phandle->ui);
    divoom_system_free(phandle);
}

void divoom_astrootoo_main_menu_pause(void *handle)
{
    Pdivoom_astrootoo_main_menu_ctrl phandle;

    phandle = (Pdivoom_astrootoo_main_menu_ctrl)handle;
    if (!phandle)
    {
        return;
    }
    if (phandle->load_timer)
    {
        lv_timer_del(phandle->load_timer);
        phandle->load_timer = NULL;
    }
    phandle->icon_retry_pending = 0;
    if (phandle->tick_timer)
    {
        lv_timer_pause(phandle->tick_timer);
    }
    lv_anim_del(phandle, divoom_astrootoo_main_menu_anim_cb);
    if (phandle->scan_bar)
    {
        lv_anim_del(phandle->scan_bar, divoom_astrootoo_main_menu_scan_anim_cb);
        lv_obj_del(phandle->scan_bar);
        phandle->scan_bar = NULL;
    }
    if (phandle->free_timer)
    {
        lv_timer_del(phandle->free_timer);
        phandle->free_timer = NULL;
    }
    /* 切屏后的延迟释放等待旧帧完成，返回菜单时由 disp 取消并继续复用缓存。 */
    phandle->free_timer = lv_timer_create(divoom_astrootoo_main_menu_deferred_free_cb, 50, phandle);
    if (phandle->free_timer)
    {
        lv_timer_set_repeat_count(phandle->free_timer, 1);
    }
}

int divoom_astrootoo_main_menu_whitebox_get_focus_index(void)
{
    Pdivoom_astrootoo_main_menu_ctrl phandle;
    int n;
    long r;

    if (divoom_menu_control_get_active_menu_id() != DIVOOM_MENU_CONTROL_ID_MAIN)
    {
        return -1;
    }
    phandle = (Pdivoom_astrootoo_main_menu_ctrl)divoom_menu_control_get_clock_mode_menu_handle();
    if (!phandle)
    {
        return -1;
    }
    n = phandle->item_count;
    if (n <= 0)
    {
        return -1;
    }
    r = lroundf(phandle->cur_target);
    return (int)(((r % n) + n) % n);
}

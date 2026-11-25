/*
 * Buddy 表情图层 LVGL 控件：多图层 PNG 叠加 + 状态驱动 JSON 配置 + 独立通道随机调度。
 */
#include "divoom_buddy.h"
#include "divoom_buddy_config.h"
#include "divoom_clock_ato_resources.h"
#include "divoom_buddy_ato_sequences.h"
#include "divoom_menu_control.h"
#include "divoom_log.h"
#include "divoom_system.h"
#include "divoom.h"
#include "middle/include/divoom_aw_pic_decode.h"
#include "divoom_pixel_format.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DIVOOM_BUDDY_CLASS                &divoom_buddy_class
#define DIVOOM_BUDDY_TICK_MS              30
#define DIVOOM_BUDDY_MICRO_CHANNEL_MAX    8
#define DIVOOM_BUDDY_FULL_FRAME_LAYER_ID  "full_frame"
#define DIVOOM_BUDDY_TEST_MODE_MS         900
#define DIVOOM_BUDDY_TEST_REFRESH_MIN_MS  120
#define DIVOOM_BUDDY_TEST_REFRESH_MAX_MS  360
#define DIVOOM_BUDDY_DECODE_RETRY_MS      500
#if defined(DIVOOM_SIMULATOR) && DIVOOM_SIMULATOR
#define DIVOOM_BUDDY_DECODE_BASE_PATH     "A:/usr/share/divoom_app/ui/"
#else
#define DIVOOM_BUDDY_DECODE_BASE_PATH     "/usr/share/divoom_app/ui/"
#endif

typedef enum
{
    DIVOOM_BUDDY_CH_EXPR = 0,
    DIVOOM_BUDDY_CH_BASE,
    DIVOOM_BUDDY_CH_MICRO0,
    DIVOOM_BUDDY_CH_MICRO1 = DIVOOM_BUDDY_CH_MICRO0 + 1,
    DIVOOM_BUDDY_CH_MICRO2 = DIVOOM_BUDDY_CH_MICRO0 + 2,
    DIVOOM_BUDDY_CH_MICRO3 = DIVOOM_BUDDY_CH_MICRO0 + 3,
    DIVOOM_BUDDY_CH_MICRO4 = DIVOOM_BUDDY_CH_MICRO0 + 4,
    DIVOOM_BUDDY_CH_MICRO5 = DIVOOM_BUDDY_CH_MICRO0 + 5,
    DIVOOM_BUDDY_CH_MICRO6 = DIVOOM_BUDDY_CH_MICRO0 + 6,
    DIVOOM_BUDDY_CH_MICRO7 = DIVOOM_BUDDY_CH_MICRO0 + 7,
    DIVOOM_BUDDY_CH_ACTION,
    DIVOOM_BUDDY_CH_COUNT
} divoom_buddy_channel_id_t;

typedef struct
{
    uint8 active;
    uint8 overlay;
    uint8 session_once;
    uint8 resting;
    divoom_buddy_clip_cfg_t clip;
    int layer_idx;
    int micro_idx;
    int frame_idx;
    int8 frame_step;
    uint32 frame_acc_ms;
    uint32 next_event_ms;
    float speed;
    int last_pool_idx;
} divoom_buddy_channel_runtime_t;

typedef struct
{
    lv_obj_t obj;
    lv_obj_t *layer_img[DIVOOM_BUDDY_MAX_LAYERS];
    char layer_src[DIVOOM_BUDDY_MAX_LAYERS][160];
    char layer_full_src[DIVOOM_BUDDY_MAX_LAYERS][224];
    lv_img_dsc_t layer_dsc[DIVOOM_BUDDY_MAX_LAYERS];
    divoom_aw_png_decode_buf_t layer_decode[DIVOOM_BUDDY_MAX_LAYERS];
    uint32_t layer_decode_retry_after_ms[DIVOOM_BUDDY_MAX_LAYERS];
    divoom_buddy_layers_cfg_t layers;
    divoom_buddy_channel_runtime_t channels[DIVOOM_BUDDY_CH_COUNT];
    divoom_buddy_state_cfg_t state;
    lv_timer_t *tick_timer;
    char cfg_dir[96];
    char mode_id[DIVOOM_BUDDY_ID_MAX];
    char pending_mode_id[DIVOOM_BUDDY_ID_MAX];
    /* ato 业务状态独立于 enter/hold/exit 视觉阶段，停止采样不等动画。 */
    char ato_business_mode[DIVOOM_BUDDY_ID_MAX];
    uint8 ato_listen_phase;
    uint8 ato_resource_lease; /* Buddy lifetime pins the unpacked 3499 directory. */
    uint8 pending_mode;
    uint8 layers_ready;
    uint8 paused;
    uint8 test_fast;
    uint8 test_mode_cycle;
    uint32 last_action_sequence;
#if defined(DIVOOM_SIMULATOR)
    uint8 test_coverage;
    uint32 whitebox_coverage_mask;
    uint32 whitebox_action_start_count;
    char whitebox_last_action_clip[DIVOOM_BUDDY_ID_MAX];
    char whitebox_forced_mode_id[DIVOOM_BUDDY_ID_MAX];
#endif
    int test_mode_idx;
    uint32 test_next_mode_ms;
    uint32 rand_seed;
    uint32 diag_last_ms;
    uint32 diag_set_src_count;
    uint32 diag_skip_src_count;
    uint32 diag_decode_fail_count;
    uint32 diag_tick_count;
} divoom_buddy_t;

static void divoom_buddy_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj);
static void divoom_buddy_destructor(const lv_obj_class_t *class_p, lv_obj_t *obj);
static void divoom_buddy_tick_cb(lv_timer_t *timer);
static int divoom_buddy_ensure_layers(divoom_buddy_t *ph);
static void divoom_buddy_channel_stop(divoom_buddy_channel_runtime_t *ch);
static void divoom_buddy_channel_schedule_refresh(divoom_buddy_t *ph, divoom_buddy_channel_runtime_t *ch,
    int refresh_min, int refresh_max);
static void divoom_buddy_apply_mode(divoom_buddy_t *ph);

#ifdef DIVOOM_SIMULATOR
static int divoom_buddy_count_active_channels(divoom_buddy_t *ph)
{
    int i;
    int count = 0;

    if (!ph)
    {
        return 0;
    }

    for (i = 0; i < DIVOOM_BUDDY_CH_COUNT; i++)
    {
        if (ph->channels[i].active || ph->channels[i].resting)
        {
            count++;
        }
    }

    return count;
}
#endif

static void divoom_buddy_print_diag(divoom_buddy_t *ph, const char *tag, uint32 now)
{
    /* 真机也保留低频渲染心跳：区别业务已切换、动画未走、解码失败及图层被隐藏。 */
    if (ph && tag && strcmp(ph->cfg_dir, "ato/") == 0)
    {
        divoom_buddy_channel_runtime_t *expression = &ph->channels[DIVOOM_BUDDY_CH_EXPR];
        int layer = expression->layer_idx;
        int visible = 0;
        if (strcmp(tag, "tick") == 0 && now - ph->diag_last_ms < 2000u)
        {
            return;
        }
        if (layer >= 0 && layer < ph->layers.layer_count && ph->layer_img[layer] &&
            lv_obj_is_valid(ph->layer_img[layer]))
        {
            visible = lv_obj_is_visible(ph->layer_img[layer]);
        }
        divoom_print("ato3499 render tag=%s mode=%s clip=%s frame=%d/%d active=%u visible=%d sets=%u decode_fail=%u ticks=%u\n",
            tag, ph->mode_id, expression->clip.id, expression->frame_idx + 1,
            expression->clip.frame_count, expression->active, visible,
            ph->diag_set_src_count, ph->diag_decode_fail_count, ph->diag_tick_count);
        ph->diag_last_ms = now;
        return;
    }
#ifdef DIVOOM_SIMULATOR
    const char *diag_env;

    if (!ph || !tag)
    {
        return;
    }

    diag_env = getenv("DIVOOM_SIM_BUDDY_DIAG");
    if (!diag_env || diag_env[0] == '\0' || diag_env[0] == '0')
    {
        return;
    }

    divoom_print("[buddy_mem] %s obj=%p mode=%s layers=%d active=%d tick=%lu set=%lu skip=%lu fail=%lu free=%ldKB largest=%ldKB\n",
        tag,
        (void *)ph,
        ph->mode_id,
        ph->layers.layer_count,
        divoom_buddy_count_active_channels(ph),
        (unsigned long)ph->diag_tick_count,
        (unsigned long)ph->diag_set_src_count,
        (unsigned long)ph->diag_skip_src_count,
        (unsigned long)ph->diag_decode_fail_count,
        divoom_system_get_ram_free_size(),
        divoom_system_get_ram_largest_free_block_size());

    ph->diag_last_ms = now;
    ph->diag_set_src_count = 0;
    ph->diag_skip_src_count = 0;
    ph->diag_decode_fail_count = 0;
    ph->diag_tick_count = 0;
#else
    (void)ph;
    (void)tag;
    (void)now;
#endif
}

static int divoom_buddy_env_enabled(const char *name)
{
    const char *value;

    value = getenv(name);
    return value && value[0] != '\0' && value[0] != '0';
}

static void divoom_buddy_init_test_flags(divoom_buddy_t *ph)
{
    if (!ph)
    {
        return;
    }

    ph->test_mode_cycle = divoom_buddy_env_enabled("DIVOOM_BUDDY_MODE_TEST") ? 1 : 0;
    ph->test_fast = (ph->test_mode_cycle || divoom_buddy_env_enabled("DIVOOM_BUDDY_FAST_TEST")) ? 1 : 0;
#if defined(DIVOOM_SIMULATOR)
    ph->test_coverage = divoom_buddy_env_enabled("DIVOOM_BUDDY_COVERAGE_TEST") ? 1 : 0;
    if (ph->test_coverage)
    {
        ph->test_fast = 1;
        divoom_print("[buddy_wb] fast=%d coverage=1\n", ph->test_fast);
    }
#endif
    ph->test_mode_idx = 0;
    ph->test_next_mode_ms = 0;
}

const lv_obj_class_t divoom_buddy_class = {
    .constructor_cb = divoom_buddy_constructor,
    .destructor_cb = divoom_buddy_destructor,
    /* 与 buddy_layers.json region 默认一致；表盘 ItemList 可再 set_size 覆盖 */
    .width_def = 193,
    .height_def = 230,
    .instance_size = sizeof(divoom_buddy_t),
    .base_class = &lv_obj_class,
};

static divoom_buddy_t *divoom_buddy_get_obj(lv_obj_t *obj)
{
    if (!obj)
    {
        return NULL;
    }

    return (divoom_buddy_t *)obj;
}

#if defined(DIVOOM_SIMULATOR)
static uint32 divoom_buddy_whitebox_clip_mask(const char *clip_id)
{
    static const struct
    {
        const char *clip_id;
        uint32 mask;
    } clip_masks[] = {
        {"expr_sit_neutral", DIVOOM_BUDDY_WB_SIT_NEUTRAL},
        {"expr_sit_happy", DIVOOM_BUDDY_WB_SIT_HAPPY},
        {"expr_sit_curious", DIVOOM_BUDDY_WB_SIT_CURIOUS},
        {"expr_sit_sniff", DIVOOM_BUDDY_WB_SIT_SNIFF},
        {"expr_sit_sleepy", DIVOOM_BUDDY_WB_SIT_SLEEPY},
        {"expr_prone_neutral", DIVOOM_BUDDY_WB_PRONE_NEUTRAL},
        {"expr_prone_sleepy", DIVOOM_BUDDY_WB_PRONE_SLEEPY},
        {"expr_prone_sad", DIVOOM_BUDDY_WB_PRONE_SAD},
        {"act_stretch", DIVOOM_BUDDY_WB_STRETCH},
        {"act_turn_back", DIVOOM_BUDDY_WB_TURN_BACK},
        {"act_play_ball", DIVOOM_BUDDY_WB_PLAY_BALL},
        {"act_dog_sing", DIVOOM_BUDDY_WB_SING},
    };
    int i;

    if (clip_id == NULL)
    {
        return 0;
    }
    for (i = 0; i < (int)(sizeof(clip_masks) / sizeof(clip_masks[0])); i++)
    {
        if (strcmp(clip_id, clip_masks[i].clip_id) == 0)
        {
            return clip_masks[i].mask;
        }
    }
    return 0;
}

static void divoom_buddy_whitebox_mark_clip(divoom_buddy_t *ph,
    const divoom_buddy_clip_cfg_t *clip)
{
    if (ph != NULL && clip != NULL && ph->test_coverage)
    {
        ph->whitebox_coverage_mask |= divoom_buddy_whitebox_clip_mask(clip->id);
        divoom_print("[buddy_wb] state=%s clip=%s mask=%u\n", ph->mode_id,
            clip->id, ph->whitebox_coverage_mask);
    }
}
#endif

static void divoom_buddy_release_layer_buf(divoom_buddy_t *ph, int layer_idx)
{
    if (!ph || layer_idx < 0 || layer_idx >= DIVOOM_BUDDY_MAX_LAYERS)
    {
        return;
    }

    if (ph->layer_img[layer_idx] && lv_obj_is_valid(ph->layer_img[layer_idx]))
    {
        /*
         * 当前 LVGL9 仿真 fork 不接受 NULL 图像源，会持续打印 unknown image type。
         * 释放像素缓冲前先隐藏对象，确保后续渲染不会访问旧描述符。
         */
        lv_obj_add_flag(ph->layer_img[layer_idx], LV_OBJ_FLAG_HIDDEN);
    }

    lv_img_cache_invalidate_src(&ph->layer_dsc[layer_idx]);
    divoom_aw_png_decode_buf_release(&ph->layer_decode[layer_idx]);
    memset(&ph->layer_dsc[layer_idx], 0, sizeof(ph->layer_dsc[layer_idx]));
    ph->layer_src[layer_idx][0] = '\0';
    ph->layer_full_src[layer_idx][0] = '\0';
    ph->layer_decode_retry_after_ms[layer_idx] = 0;
}

static void divoom_buddy_release_all_layer_buf(divoom_buddy_t *ph)
{
    int i;

    if (!ph)
    {
        return;
    }

    for (i = 0; i < DIVOOM_BUDDY_MAX_LAYERS; i++)
    {
        divoom_buddy_release_layer_buf(ph, i);
    }
}

static uint32_t divoom_buddy_rand_u32(divoom_buddy_t *ph)
{
    ph->rand_seed = ph->rand_seed * 1103515245u + 12345u;
    return ph->rand_seed;
}

static uint32_t divoom_buddy_rand_range(divoom_buddy_t *ph, uint32_t lo, uint32_t hi)
{
    if (hi <= lo)
    {
        return lo;
    }

    return lo + (divoom_buddy_rand_u32(ph) % (hi - lo + 1u));
}

static float divoom_buddy_rand_float_range(divoom_buddy_t *ph, float lo, float hi)
{
    uint32_t r;

    if (hi <= lo)
    {
        return lo;
    }

    r = divoom_buddy_rand_u32(ph) % 10001u;
    return lo + (hi - lo) * ((float)r / 10000.0f);
}

static uint8 divoom_buddy_is_full_frame_layer(divoom_buddy_t *ph, int layer_idx)
{
    if (!ph || layer_idx < 0 || layer_idx >= ph->layers.layer_count)
    {
        return 0;
    }

    return (strcmp(ph->layers.layers[layer_idx].id, DIVOOM_BUDDY_FULL_FRAME_LAYER_ID) == 0) ? 1 : 0;
}

static uint8 divoom_buddy_is_rest_pose_layer(divoom_buddy_t *ph, int layer_idx)
{
    const char *id;

    if (!ph || layer_idx < 0 || layer_idx >= ph->layers.layer_count)
    {
        return 0;
    }

    id = ph->layers.layers[layer_idx].id;
    return (strcmp(id, "tail") == 0 || strcmp(id, "ears") == 0) ? 1 : 0;
}

static int divoom_buddy_find_active_full_frame_layer(divoom_buddy_t *ph)
{
    int i;

    if (!ph)
    {
        return -1;
    }

    for (i = 0; i < DIVOOM_BUDDY_CH_COUNT; i++)
    {
        if (ph->channels[i].active && divoom_buddy_is_full_frame_layer(ph, ph->channels[i].layer_idx))
        {
            return ph->channels[i].layer_idx;
        }
    }

    return -1;
}

static void divoom_buddy_set_pose_layers_hidden(divoom_buddy_t *ph, uint8 hidden)
{
    int i;

    if (!ph)
    {
        return;
    }

    for (i = 0; i < ph->layers.layer_count; i++)
    {
        if (divoom_buddy_is_full_frame_layer(ph, i) || !ph->layer_img[i])
        {
            continue;
        }

        if (hidden)
        {
            lv_obj_add_flag(ph->layer_img[i], LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_clear_flag(ph->layer_img[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static int divoom_buddy_pick_weighted(divoom_buddy_t *ph, divoom_buddy_pool_opt_t *opts, int cnt, int avoid_idx)
{
    int i;
    int total = 0;
    int pick;
    int acc;

    if (cnt <= 0)
    {
        return -1;
    }

#if defined(DIVOOM_SIMULATOR)
    /* 覆盖测试按池顺序轮转，仍经过正式 state/clip/timer 通道，消除随机漏测。 */
    if (ph != NULL && ph->test_coverage && cnt > 1)
    {
        return (avoid_idx >= 0 && avoid_idx < cnt) ? (avoid_idx + 1) % cnt : 0;
    }
#endif

    for (i = 0; i < cnt; i++)
    {
        total += opts[i].weight;
    }

    if (total <= 0)
    {
        return 0;
    }

    if (cnt > 1 && avoid_idx >= 0 && avoid_idx < cnt)
    {
        pick = (int)divoom_buddy_rand_range(ph, 0, (uint32_t)(total - opts[avoid_idx].weight - 1));
        acc = 0;
        for (i = 0; i < cnt; i++)
        {
            if (i == avoid_idx)
            {
                continue;
            }

            acc += opts[i].weight;
            if (pick < acc)
            {
                return i;
            }
        }
    }

    pick = (int)divoom_buddy_rand_range(ph, 0, (uint32_t)(total - 1));
    acc = 0;
    for (i = 0; i < cnt; i++)
    {
        acc += opts[i].weight;
        if (pick < acc)
        {
            return i;
        }
    }

    return cnt - 1;
}

static void divoom_buddy_set_layer_pos_idx(divoom_buddy_t *ph, int layer_idx, int x, int y)
{
    if (layer_idx < 0 || layer_idx >= ph->layers.layer_count)
    {
        return;
    }

    if (!ph->layer_img[layer_idx])
    {
        return;
    }

    lv_obj_set_pos(ph->layer_img[layer_idx], (lv_coord_t)x, (lv_coord_t)y);
}

static void divoom_buddy_apply_default_layer_pos(divoom_buddy_t *ph)
{
    int i;

    for (i = 0; i < ph->layers.layer_count; i++)
    {
        divoom_buddy_set_layer_pos_idx(ph, i, ph->layers.layers[i].x, ph->layers.layers[i].y);
    }
}

static void divoom_buddy_apply_layer_pos_from_clip(divoom_buddy_t *ph, const divoom_buddy_clip_cfg_t *clip)
{
    int i;

    divoom_buddy_apply_default_layer_pos(ph);
    if (!clip || !clip->has_layer_pos)
    {
        return;
    }

    for (i = 0; i < ph->layers.layer_count; i++)
    {
        if (clip->layer_pos_mask & (uint8)(1u << i))
        {
            divoom_buddy_set_layer_pos_idx(ph, i, clip->layer_pos_x[i], clip->layer_pos_y[i]);
        }
    }
}

/*
 * 角色资源目录变化时，旧角色的 layer 子对象、状态和运行通道都不能复用。
 * 例如 255 狗狗切到 1721 妈妈时，layer 数量和坐标不同，必须让下一次 apply mode 重建图层。
 */
static void divoom_buddy_reset_layers_for_config(divoom_buddy_t *ph)
{
    int i;

    if (!ph)
    {
        return;
    }

    for (i = 0; i < DIVOOM_BUDDY_MAX_LAYERS; i++)
    {
        divoom_buddy_release_layer_buf(ph, i);
        if (ph->layer_img[i])
        {
            lv_obj_del(ph->layer_img[i]);
            ph->layer_img[i] = NULL;
        }
    }

    memset(ph->channels, 0, sizeof(ph->channels));
    memset(&ph->layers, 0, sizeof(ph->layers));
    memset(&ph->state, 0, sizeof(ph->state));
    ph->layers_ready = 0;
    ph->mode_id[0] = '\0';
    ph->pending_mode = 0;
    ph->pending_mode_id[0] = '\0';
}

static void divoom_buddy_show_layer_frame(divoom_buddy_t *ph, int layer_idx, const divoom_buddy_clip_cfg_t *clip, int frame_idx)
{
    char rel[160];
    char full_src[224];
    uint8 *img_buf;
    uint32 data_size;
    int image_w = 0;
    int image_h = 0;
    int active_full_frame_layer;
    uint8 is_full_frame_layer;
    uint32_t now_ms;

    if (!ph || ph->paused || !clip || layer_idx < 0 || layer_idx >= ph->layers.layer_count)
    {
        return;
    }

    if (!ph->layer_img[layer_idx] || !lv_obj_is_valid(ph->layer_img[layer_idx]))
    {
        return;
    }

    if (frame_idx < 0 || frame_idx >= clip->frame_count)
    {
        frame_idx = 0;
    }

    snprintf(rel, sizeof(rel), "%s%s", ph->cfg_dir, clip->frames[frame_idx]);
    if (strncmp(rel, "ato/", 4) == 0)
    {
        if (!divoom_clock_ato_resources_path(rel, full_src, sizeof(full_src), 1))
        {
            return;
        }
    }
    else
    {
        snprintf(full_src, sizeof(full_src), "%s%s", DIVOOM_BUDDY_DECODE_BASE_PATH, rel);
    }
    now_ms = divoom_system_get_tickms();
    if (ph->layer_decode_retry_after_ms[layer_idx] != 0 &&
        (int32_t)(now_ms - ph->layer_decode_retry_after_ms[layer_idx]) < 0)
    {
        ph->diag_skip_src_count++;
        return;
    }
    ph->layer_decode_retry_after_ms[layer_idx] = 0;
    if (strcmp(ph->layer_src[layer_idx], rel) == 0)
    {
        ph->diag_skip_src_count++;
    }
    else
    {
        /*
         * 换帧在 LVGL timer 上下文内同步完成，不需要用 NULL 解绑图像源。
         * 先隐藏并失效旧缓存，解码成功后再设置同一 layer 的新描述符。
         */
        lv_obj_add_flag(ph->layer_img[layer_idx], LV_OBJ_FLAG_HIDDEN);
        lv_img_cache_invalidate_src(&ph->layer_dsc[layer_idx]);

        /*
         * Buddy 帧动画是高频切换，不能把 PNG 文件名交给 LVGL 缓存反复申请。
         * 每个 layer 只持有一块可复用 RGB565A8 缓冲，PNG 解码后直接写入该缓冲。
         */
        img_buf = divoom_aw_decode_png_file_to_lvfmt_reuse(full_src, &image_w,
            &image_h, &ph->layer_decode[layer_idx], LV_COLOR_FORMAT_RGB565A8);
        if (img_buf == NULL || image_w <= 0 || image_h <= 0)
        {
            ph->diag_decode_fail_count++;
            /* 首次失败及每 16 次重试记录路径，避免每个动画帧刷屏。 */
            if (strcmp(ph->cfg_dir, "ato/") == 0 &&
                (ph->diag_decode_fail_count == 1 || (ph->diag_decode_fail_count & 15u) == 0))
            {
                divoom_print("ato3499 buddy frame_failed mode=%s clip=%s frame=%d path=%s failures=%u\n",
                    ph->mode_id, clip->id, frame_idx, full_src, ph->diag_decode_fail_count);
            }
            ph->layer_decode_retry_after_ms[layer_idx] =
                now_ms + DIVOOM_BUDDY_DECODE_RETRY_MS;
            ph->layer_src[layer_idx][0] = '\0';
            ph->layer_full_src[layer_idx][0] = '\0';
            lv_obj_add_flag(ph->layer_img[layer_idx], LV_OBJ_FLAG_HIDDEN);
            return;
        }

        /*
         * 不能手工只填 cf/w/h/data。LVGL9 真机需要 header.stride；
         * helper 会按 RGB565A8 的平面布局补齐 stride/data_size。
         */
        divoom_lv_img_dsc_init_fmt(&ph->layer_dsc[layer_idx], (uint32)image_w,
            (uint32)image_h, LV_COLOR_FORMAT_RGB565A8, img_buf);
        data_size = ph->layer_dsc[layer_idx].data_size;
        if (data_size == 0 || data_size > ph->layer_decode[layer_idx].out_buf_size)
        {
            ph->diag_decode_fail_count++;
            /* 首次失败及每 16 次重试记录路径，避免每个动画帧刷屏。 */
            if (strcmp(ph->cfg_dir, "ato/") == 0 &&
                (ph->diag_decode_fail_count == 1 || (ph->diag_decode_fail_count & 15u) == 0))
            {
                divoom_print("ato3499 buddy frame_failed mode=%s clip=%s frame=%d path=%s failures=%u\n",
                    ph->mode_id, clip->id, frame_idx, full_src, ph->diag_decode_fail_count);
            }
            ph->layer_decode_retry_after_ms[layer_idx] =
                now_ms + DIVOOM_BUDDY_DECODE_RETRY_MS;
            ph->layer_src[layer_idx][0] = '\0';
            ph->layer_full_src[layer_idx][0] = '\0';
            lv_obj_add_flag(ph->layer_img[layer_idx], LV_OBJ_FLAG_HIDDEN);
            return;
        }

        ph->layer_decode_retry_after_ms[layer_idx] = 0;
        lv_img_set_src(ph->layer_img[layer_idx], &ph->layer_dsc[layer_idx]);
        strncpy(ph->layer_src[layer_idx], rel, sizeof(ph->layer_src[layer_idx]) - 1);
        ph->layer_src[layer_idx][sizeof(ph->layer_src[layer_idx]) - 1] = '\0';
        strncpy(ph->layer_full_src[layer_idx], full_src, sizeof(ph->layer_full_src[layer_idx]) - 1);
        ph->layer_full_src[layer_idx][sizeof(ph->layer_full_src[layer_idx]) - 1] = '\0';
        ph->diag_set_src_count++;
    }
    active_full_frame_layer = divoom_buddy_find_active_full_frame_layer(ph);
    is_full_frame_layer = divoom_buddy_is_full_frame_layer(ph, layer_idx);
    if (is_full_frame_layer)
    {
        divoom_buddy_set_pose_layers_hidden(ph, 1);
        lv_obj_clear_flag(ph->layer_img[layer_idx], LV_OBJ_FLAG_HIDDEN);
    }
    else if (active_full_frame_layer >= 0)
    {
        lv_obj_add_flag(ph->layer_img[layer_idx], LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        lv_obj_clear_flag(ph->layer_img[layer_idx], LV_OBJ_FLAG_HIDDEN);
    }
#if defined(DIVOOM_SIMULATOR)
    if (!lv_obj_has_flag(ph->layer_img[layer_idx], LV_OBJ_FLAG_HIDDEN))
    {
        /* 覆盖位只在首帧真实解码并进入可见层后记录，资源失败不能误判通过。 */
        divoom_buddy_whitebox_mark_clip(ph, clip);
    }
#endif
}

static void divoom_buddy_clear_layer_frame(divoom_buddy_t *ph, int layer_idx)
{
    if (!ph || ph->paused || layer_idx < 0 || layer_idx >= ph->layers.layer_count)
    {
        return;
    }

    if (!ph->layer_img[layer_idx] || !lv_obj_is_valid(ph->layer_img[layer_idx]))
    {
        return;
    }

    divoom_buddy_release_layer_buf(ph, layer_idx);
    lv_obj_add_flag(ph->layer_img[layer_idx], LV_OBJ_FLAG_HIDDEN);
}

static uint8 divoom_buddy_layer_has_active_channel(divoom_buddy_t *ph, int layer_idx)
{
    int i;

    if (!ph || layer_idx < 0 || layer_idx >= ph->layers.layer_count)
    {
        return 0;
    }

    for (i = 0; i < DIVOOM_BUDDY_CH_COUNT; i++)
    {
        if ((ph->channels[i].active || ph->channels[i].resting) && ph->channels[i].layer_idx == layer_idx)
        {
            return 1;
        }
    }

    return 0;
}

static divoom_buddy_channel_runtime_t *divoom_buddy_find_free_pose_rest_channel(divoom_buddy_t *ph)
{
    int i;

    if (!ph)
    {
        return NULL;
    }

    for (i = DIVOOM_BUDDY_CH_MICRO0; i < DIVOOM_BUDDY_CH_COUNT; i++)
    {
        if (!ph->channels[i].active && !ph->channels[i].resting && ph->channels[i].next_event_ms == 0)
        {
            return &ph->channels[i];
        }
    }

    return NULL;
}

static const char *divoom_buddy_get_current_pose_id(divoom_buddy_t *ph)
{
    if (!ph)
    {
        return NULL;
    }

    if (ph->channels[DIVOOM_BUDDY_CH_BASE].clip.pose_id[0] != '\0')
    {
        return ph->channels[DIVOOM_BUDDY_CH_BASE].clip.pose_id;
    }

    if (ph->channels[DIVOOM_BUDDY_CH_EXPR].clip.pose_id[0] != '\0')
    {
        return ph->channels[DIVOOM_BUDDY_CH_EXPR].clip.pose_id;
    }

    return NULL;
}

static const char *divoom_buddy_get_pose_rest_clip_id(const char *pose_id, const char *layer_id)
{
    if (!pose_id || !layer_id)
    {
        return NULL;
    }

    if (strcmp(pose_id, "sit_pose") == 0)
    {
        if (strcmp(layer_id, "ears") == 0)
        {
            return "act_sit_ears_flop";
        }
        if (strcmp(layer_id, "tail") == 0)
        {
            return "act_sit_tail_wag";
        }
    }
    else if (strcmp(pose_id, "prone_pose") == 0)
    {
        if (strcmp(layer_id, "ears") == 0)
        {
            return "act_prone_ears_flop";
        }
        if (strcmp(layer_id, "tail") == 0)
        {
            return "act_prone_tail_wag";
        }
    }

    return NULL;
}

static void divoom_buddy_prepare_pose_rest_layers(divoom_buddy_t *ph)
{
    const char *pose_id;
    const char *clip_id;
    divoom_buddy_clip_cfg_t clip;
    divoom_buddy_channel_runtime_t *ch;
    int i;
    int layer_idx;

    if (!ph)
    {
        return;
    }

    pose_id = divoom_buddy_get_current_pose_id(ph);
    if (!pose_id)
    {
        return;
    }

    /*
     * tail/ears 是当前 pose 的组成层。即使某个 state 没把它们声明成
     * micro action，也要建立 resting channel，让清理逻辑知道这些层属于
     * 当前姿态，避免坐姿 body/face 留下但狗头图层被删。
     */
    for (i = 0; i < ph->layers.layer_count; i++)
    {
        if (!divoom_buddy_is_rest_pose_layer(ph, i) || divoom_buddy_layer_has_active_channel(ph, i))
        {
            continue;
        }

        clip_id = divoom_buddy_get_pose_rest_clip_id(pose_id, ph->layers.layers[i].id);
        if (!clip_id || divoom_buddy_config_get_clip(clip_id, &clip) != 0)
        {
            continue;
        }

        layer_idx = divoom_buddy_config_find_layer_index(&ph->layers, clip.layer_id);
        if (layer_idx != i)
        {
            continue;
        }

        ch = divoom_buddy_find_free_pose_rest_channel(ph);
        if (!ch)
        {
            return;
        }

        memset(ch, 0, sizeof(*ch));
        memcpy(&ch->clip, &clip, sizeof(ch->clip));
        ch->resting = 1;
        ch->layer_idx = i;
        ch->micro_idx = -1;
        ch->frame_idx = 0;
        divoom_buddy_show_layer_frame(ph, i, &ch->clip, 0);
    }
}

static void divoom_buddy_clear_inactive_layers(divoom_buddy_t *ph)
{
    int i;

    if (!ph)
    {
        return;
    }

    for (i = 0; i < ph->layers.layer_count; i++)
    {
        if (!divoom_buddy_layer_has_active_channel(ph, i))
        {
            divoom_buddy_clear_layer_frame(ph, i);
        }
    }
}

static int divoom_buddy_clip_frame_delay_ms(const divoom_buddy_clip_cfg_t *clip, int frame_idx, float speed)
{
    int delay_ms;

    if (!clip || frame_idx < 0 || frame_idx >= clip->frame_count)
    {
        return 100;
    }

    if (clip->has_frame_delays)
    {
        delay_ms = clip->frame_delays_ms[frame_idx];
    }
    else if (clip->fps_is_range)
    {
        int fps = clip->fps_min;
        if (clip->fps_max > clip->fps_min)
        {
            fps = clip->fps_min + (frame_idx % (clip->fps_max - clip->fps_min + 1));
        }

        delay_ms = (fps > 0) ? (1000 / fps) : 1000;
    }
    else if (clip->fps > 0)
    {
        delay_ms = 1000 / clip->fps;
    }
    else
    {
        delay_ms = 1000;
    }

    if (speed <= 0.01f)
    {
        speed = 1.0f;
    }

    delay_ms = (int)((float)delay_ms / speed);
    if (delay_ms < 1)
    {
        delay_ms = 1;
    }

    return delay_ms;
}

static void divoom_buddy_channel_start(divoom_buddy_t *ph, divoom_buddy_channel_runtime_t *ch,
    const divoom_buddy_clip_cfg_t *clip, float speed, uint8 overlay)
{
    if (!ph || !ch || !clip)
    {
        return;
    }

    memcpy(&ch->clip, clip, sizeof(ch->clip));
    ch->active = 1;
    ch->overlay = overlay;
    ch->session_once = 0;
    ch->resting = 0;
    ch->frame_idx = 0;
    ch->frame_step = 1;
    ch->frame_acc_ms = 0;
    ch->speed = (speed <= 0.01f) ? 1.0f : speed;
    ch->micro_idx = -1;
    ch->layer_idx = divoom_buddy_config_find_layer_index(&ph->layers, clip->layer_id);
    if (ch->layer_idx < 0)
    {
        divoom_buddy_channel_stop(ch);
        return;
    }

    divoom_buddy_show_layer_frame(ph, ch->layer_idx, &ch->clip, ch->frame_idx);
}

static void divoom_buddy_channel_stop(divoom_buddy_channel_runtime_t *ch)
{
    if (!ch)
    {
        return;
    }

    memset(ch, 0, sizeof(*ch));
    ch->micro_idx = -1;
    ch->layer_idx = -1;
}

static divoom_buddy_channel_runtime_t *divoom_buddy_find_overlay_on_layer(divoom_buddy_t *ph, int layer_idx)
{
    int i;

    for (i = DIVOOM_BUDDY_CH_MICRO0; i < DIVOOM_BUDDY_CH_COUNT; i++)
    {
        if (ph->channels[i].active && ph->channels[i].overlay && ph->channels[i].layer_idx == layer_idx)
        {
            return &ph->channels[i];
        }
    }

    return NULL;
}

static divoom_buddy_channel_runtime_t *divoom_buddy_find_base_on_layer(divoom_buddy_t *ph, int layer_idx)
{
    int i;

    if (ph->channels[DIVOOM_BUDDY_CH_EXPR].active && ph->channels[DIVOOM_BUDDY_CH_EXPR].layer_idx == layer_idx)
    {
        return &ph->channels[DIVOOM_BUDDY_CH_EXPR];
    }

    if (ph->channels[DIVOOM_BUDDY_CH_BASE].active && ph->channels[DIVOOM_BUDDY_CH_BASE].layer_idx == layer_idx)
    {
        return &ph->channels[DIVOOM_BUDDY_CH_BASE];
    }

    for (i = DIVOOM_BUDDY_CH_MICRO0; i < DIVOOM_BUDDY_CH_COUNT; i++)
    {
        if (ph->channels[i].active && !ph->channels[i].overlay && ph->channels[i].layer_idx == layer_idx)
        {
            return &ph->channels[i];
        }
    }

    return NULL;
}

static divoom_buddy_channel_runtime_t *divoom_buddy_find_resting_on_layer(divoom_buddy_t *ph, int layer_idx)
{
    int i;

    if (!ph)
    {
        return NULL;
    }

    for (i = DIVOOM_BUDDY_CH_MICRO0; i < DIVOOM_BUDDY_CH_COUNT; i++)
    {
        if (ph->channels[i].resting && ph->channels[i].layer_idx == layer_idx)
        {
            return &ph->channels[i];
        }
    }

    return NULL;
}

static void divoom_buddy_refresh_layer_visible(divoom_buddy_t *ph, int layer_idx)
{
    divoom_buddy_channel_runtime_t *overlay;
    divoom_buddy_channel_runtime_t *base;

    overlay = divoom_buddy_find_overlay_on_layer(ph, layer_idx);
    if (overlay)
    {
        divoom_buddy_show_layer_frame(ph, layer_idx, &overlay->clip, overlay->frame_idx);
        return;
    }

    base = divoom_buddy_find_base_on_layer(ph, layer_idx);
    if (base)
    {
        divoom_buddy_show_layer_frame(ph, layer_idx, &base->clip, base->frame_idx);
        return;
    }

    base = divoom_buddy_find_resting_on_layer(ph, layer_idx);
    if (base)
    {
        divoom_buddy_show_layer_frame(ph, layer_idx, &base->clip, base->frame_idx);
        return;
    }

    divoom_buddy_clear_layer_frame(ph, layer_idx);
}

static void divoom_buddy_micro_finish_session(divoom_buddy_t *ph, divoom_buddy_channel_runtime_t *ch)
{
    divoom_buddy_clip_cfg_t clip;
    int layer_idx;
    int micro_idx;
    uint8 was_full_frame;
    uint8 keep_rest_frame;

    if (!ph || !ch)
    {
        return;
    }

    memcpy(&clip, &ch->clip, sizeof(clip));
    layer_idx = ch->layer_idx;
    micro_idx = ch->micro_idx;
    was_full_frame = divoom_buddy_is_full_frame_layer(ph, layer_idx);
    keep_rest_frame = divoom_buddy_is_rest_pose_layer(ph, layer_idx);
    /*
     * interval=[0,0] 的分层 micro 是当前状态的常驻组成层，不是一次性特效。
     * 例如 Divo 的 arms_wave 本身不循环；播放完成后必须停回首帧，否则
     * arms 图层会被清掉，并且零间隔通道不会再次调度，最终两只手一起消失。
     */
    if (!was_full_frame && micro_idx >= 0 && micro_idx < ph->state.micro_action_count &&
        ph->state.micro_actions[micro_idx].interval_min_ms == 0 &&
        ph->state.micro_actions[micro_idx].interval_max_ms == 0)
    {
        keep_rest_frame = 1;
    }
    divoom_buddy_channel_stop(ch);
    if (keep_rest_frame)
    {
        memcpy(&ch->clip, &clip, sizeof(ch->clip));
        ch->layer_idx = layer_idx;
        ch->micro_idx = micro_idx;
        ch->frame_idx = 0;
        ch->resting = 1;
        divoom_buddy_show_layer_frame(ph, layer_idx, &ch->clip, 0);
    }
    else
    {
        divoom_buddy_refresh_layer_visible(ph, layer_idx);
    }
    if (was_full_frame)
    {
        int i;

        divoom_buddy_set_pose_layers_hidden(ph, 0);
        for (i = 0; i < ph->layers.layer_count; i++)
        {
            if (!divoom_buddy_is_full_frame_layer(ph, i))
            {
                divoom_buddy_refresh_layer_visible(ph, i);
            }
        }
    }

    if (micro_idx >= 0 && micro_idx < ph->state.micro_action_count)
    {
        ch->micro_idx = micro_idx;
        if (ph->state.micro_actions[micro_idx].once_per_mode)
        {
            /* 照片/像素等场景进入动作只执行一次，场景内不重新排期。 */
            ch->next_event_ms = 0;
        }
        else if (ph->state.micro_actions[micro_idx].interval_min_ms != 0 ||
            ph->state.micro_actions[micro_idx].interval_max_ms != 0)
        {
            divoom_buddy_channel_schedule_refresh(ph, ch,
                ph->state.micro_actions[micro_idx].interval_min_ms,
                ph->state.micro_actions[micro_idx].interval_max_ms);
        }
        else
        {
            ch->next_event_ms = 0;
        }
    }
}

/* 仅 ato 使用资源契约的序列控制；其它角色保持原通道调度。 */
static int divoom_buddy_ato_complete(divoom_buddy_t *ph, divoom_buddy_channel_runtime_t *ch)
{
    uint32 i;
    const char *mode = NULL;
    divoom_buddy_clip_cfg_t clip;
    if (strcmp(ph->cfg_dir, "ato/") != 0)
    {
        return 0;
    }
    if (ch == &ph->channels[DIVOOM_BUDDY_CH_EXPR] && ph->ato_listen_phase)
    {
        if (ph->ato_listen_phase == 1)
        {
            ph->ato_listen_phase = 2;
            mode = "listening_hold";
        }
        else if (ph->ato_listen_phase == 2 && strcmp(ph->ato_business_mode, "listening") != 0)
        {
            ph->ato_listen_phase = 3;
            mode = "listening_exit";
        }
        else if (ph->ato_listen_phase == 3)
        {
            ph->ato_listen_phase = 0;
            mode = ph->ato_business_mode;
        }
        if (mode)
        {
            snprintf(ph->pending_mode_id, sizeof(ph->pending_mode_id), "%s", mode);
            ph->pending_mode = 1;
            divoom_buddy_apply_mode(ph);
            return 1;
        }
    }
    for (i = 0; i < sizeof(divoom_buddy_ato_links) / sizeof(divoom_buddy_ato_links[0]); i++)
    {
        if (strcmp(ch->clip.id, divoom_buddy_ato_links[i].clip) == 0)
        {
            if (divoom_buddy_config_get_clip(divoom_buddy_ato_links[i].next, &clip) == 0)
            {
                /* 沿真实末帧完成事件换片，不用估算总时长，也不重置其它层。 */
                ch->clip = clip;
                ch->frame_idx = 0;
                ch->frame_acc_ms = 0;
                divoom_buddy_show_layer_frame(ph, ch->layer_idx, &ch->clip, 0);
                return 1;
            }
            divoom_buddy_channel_stop(ch);
            return 1;
        }
    }
    return 0;
}

static void divoom_buddy_channel_advance_frame(divoom_buddy_t *ph, divoom_buddy_channel_runtime_t *ch)
{
    int next;

    if (!ph || !ch || !ch->active)
    {
        return;
    }

    next = ch->frame_idx + (ch->frame_step < 0 ? -1 : 1);
    if ((ch->frame_step < 0 && next < 0) ||
        (ch->frame_step >= 0 && next >= ch->clip.frame_count))
    {
        if (divoom_buddy_ato_complete(ph, ch))
        {
            return;
        }
        if (ch->clip.loop)
        {
            next = ch->frame_step < 0 ? ch->clip.frame_count - 1 : 0;
        }
        else
        {
            if (ch->session_once)
            {
                divoom_buddy_micro_finish_session(ph, ch);
                return;
            }

            if (ch->overlay)
            {
                int layer_idx = ch->layer_idx;

                divoom_buddy_channel_stop(ch);
                divoom_buddy_refresh_layer_visible(ph, layer_idx);
                return;
            }

            next = ch->frame_step < 0 ? 0 : ch->clip.frame_count - 1;
        }
    }

    ch->frame_idx = next;
    ch->frame_acc_ms = 0;
    if (!ch->overlay || divoom_buddy_find_overlay_on_layer(ph, ch->layer_idx) == ch)
    {
        divoom_buddy_show_layer_frame(ph, ch->layer_idx, &ch->clip, ch->frame_idx);
    }
}

static void divoom_buddy_channel_schedule_refresh(divoom_buddy_t *ph, divoom_buddy_channel_runtime_t *ch,
    int refresh_min, int refresh_max)
{
    uint32_t now;

    if (!ph || !ch)
    {
        return;
    }

    now = lv_tick_get();
    if (ph->test_fast)
    {
        refresh_min = DIVOOM_BUDDY_TEST_REFRESH_MIN_MS;
        refresh_max = DIVOOM_BUDDY_TEST_REFRESH_MAX_MS;
    }
    if (refresh_max <= refresh_min)
    {
        ch->next_event_ms = now + (uint32_t)refresh_min;
    }
    else
    {
        ch->next_event_ms = now + divoom_buddy_rand_range(ph, (uint32_t)refresh_min, (uint32_t)refresh_max);
    }
}

static void divoom_buddy_test_mode_cycle(divoom_buddy_t *ph, uint32_t now)
{
    static const char *test_modes[] = {
        "idle_life",
        "scene_sunny",
        "scene_rainy",
        "scene_music",
        "scene_night",
        "scene_life",
    };
    const char *next_mode;
    int mode_count;

    if (!ph || !ph->test_mode_cycle)
    {
        return;
    }

    if (ph->test_next_mode_ms != 0 && now < ph->test_next_mode_ms)
    {
        return;
    }

    mode_count = (int)(sizeof(test_modes) / sizeof(test_modes[0]));
    next_mode = test_modes[ph->test_mode_idx % mode_count];
    ph->test_mode_idx++;
    ph->test_next_mode_ms = now + DIVOOM_BUDDY_TEST_MODE_MS;

    strncpy(ph->pending_mode_id, next_mode, DIVOOM_BUDDY_ID_MAX - 1);
    ph->pending_mode_id[DIVOOM_BUDDY_ID_MAX - 1] = '\0';
    ph->pending_mode = 1;
    divoom_buddy_apply_mode(ph);
}

static void divoom_buddy_start_expression_channel(divoom_buddy_t *ph)
{
    divoom_buddy_clip_cfg_t clip;
    int idx;
    float speed = 1.0f;
    divoom_buddy_pool_opt_t *opt;

    idx = divoom_buddy_pick_weighted(ph, ph->state.expression_opts, ph->state.expression_opt_count,
        ph->channels[DIVOOM_BUDDY_CH_EXPR].last_pool_idx);
    if (idx < 0)
    {
        return;
    }

    opt = &ph->state.expression_opts[idx];
    if (divoom_buddy_config_get_clip(opt->clip, &clip) != 0)
    {
        return;
    }

    if (opt->has_speed)
    {
        speed = divoom_buddy_rand_float_range(ph, opt->speed_min, opt->speed_max);
    }

    divoom_buddy_channel_start(ph, &ph->channels[DIVOOM_BUDDY_CH_EXPR], &clip, speed, 0);
    ph->channels[DIVOOM_BUDDY_CH_EXPR].last_pool_idx = idx;
    divoom_buddy_channel_schedule_refresh(ph, &ph->channels[DIVOOM_BUDDY_CH_EXPR],
        ph->state.expression_refresh_min_ms, ph->state.expression_refresh_max_ms);
}

static void divoom_buddy_start_base_channel(divoom_buddy_t *ph)
{
    divoom_buddy_clip_cfg_t clip;
    int idx;
    float speed = 1.0f;
    divoom_buddy_pool_opt_t *opt;

    idx = divoom_buddy_pick_weighted(ph, ph->state.base_action_opts, ph->state.base_action_opt_count,
        ph->channels[DIVOOM_BUDDY_CH_BASE].last_pool_idx);
    if (idx < 0)
    {
        return;
    }

    opt = &ph->state.base_action_opts[idx];
    if (divoom_buddy_config_get_clip(opt->clip, &clip) != 0)
    {
        return;
    }

    if (opt->has_speed)
    {
        speed = divoom_buddy_rand_float_range(ph, opt->speed_min, opt->speed_max);
    }

    divoom_buddy_apply_layer_pos_from_clip(ph, &clip);
    divoom_buddy_channel_start(ph, &ph->channels[DIVOOM_BUDDY_CH_BASE], &clip, speed, 0);
    ph->channels[DIVOOM_BUDDY_CH_BASE].last_pool_idx = idx;
    divoom_buddy_channel_schedule_refresh(ph, &ph->channels[DIVOOM_BUDDY_CH_BASE],
        ph->state.base_action_refresh_min_ms, ph->state.base_action_refresh_max_ms);
}

static void divoom_buddy_prepare_micro_rest_frame(divoom_buddy_t *ph, int micro_idx)
{
    divoom_buddy_clip_cfg_t clip;
    divoom_buddy_micro_def_t *def;
    divoom_buddy_channel_runtime_t *ch;
    int ch_id;
    int layer_idx;

    if (!ph || micro_idx < 0 || micro_idx >= ph->state.micro_action_count)
    {
        return;
    }

    ch_id = DIVOOM_BUDDY_CH_MICRO0 + micro_idx;
    if (ch_id >= DIVOOM_BUDDY_CH_COUNT)
    {
        return;
    }

    def = &ph->state.micro_actions[micro_idx];
    ch = &ph->channels[ch_id];
    memset(ch, 0, sizeof(*ch));
    ch->micro_idx = micro_idx;
    ch->layer_idx = -1;

    if (divoom_buddy_config_get_clip(def->clip, &clip) == 0)
    {
        layer_idx = divoom_buddy_config_find_layer_index(&ph->layers, clip.layer_id);
        if (layer_idx >= 0 && divoom_buddy_is_rest_pose_layer(ph, layer_idx))
        {
            memcpy(&ch->clip, &clip, sizeof(ch->clip));
            ch->layer_idx = layer_idx;
            ch->frame_idx = 0;
            ch->resting = 1;
            divoom_buddy_show_layer_frame(ph, layer_idx, &ch->clip, 0);
        }
    }

    divoom_buddy_channel_schedule_refresh(ph, ch, def->interval_min_ms, def->interval_max_ms);
}

static void divoom_buddy_start_micro_channel(divoom_buddy_t *ph, int micro_idx)
{
    divoom_buddy_clip_cfg_t clip;
    divoom_buddy_micro_def_t *def;
    divoom_buddy_channel_runtime_t *ch;
    float speed = 1.0f;
    int ch_id;
    int expr_layer;
    int layer_idx;
    uint8 overlay;

    if (micro_idx < 0 || micro_idx >= ph->state.micro_action_count)
    {
        return;
    }

    def = &ph->state.micro_actions[micro_idx];
    if (divoom_buddy_config_get_clip(def->clip, &clip) != 0)
    {
        return;
    }

    if (def->has_speed)
    {
        speed = divoom_buddy_rand_float_range(ph, def->speed_min, def->speed_max);
    }

    ch_id = DIVOOM_BUDDY_CH_MICRO0 + micro_idx;
    if (ch_id >= DIVOOM_BUDDY_CH_COUNT)
    {
        return;
    }

    layer_idx = divoom_buddy_config_find_layer_index(&ph->layers, clip.layer_id);
    if (layer_idx >= 0 &&
        divoom_buddy_is_full_frame_layer(ph, layer_idx) &&
        divoom_buddy_find_active_full_frame_layer(ph) >= 0)
    {
        return;
    }

    ch = &ph->channels[ch_id];
    expr_layer = ph->channels[DIVOOM_BUDDY_CH_EXPR].layer_idx;
    overlay = 0;
    if (expr_layer >= 0 && strcmp(clip.layer_id, ph->layers.layers[expr_layer].id) == 0 && !clip.loop)
    {
        overlay = 1;
    }

    divoom_buddy_channel_start(ph, ch, &clip, speed, overlay);
    ch->micro_idx = micro_idx;
    ch->session_once = (def->interval_min_ms != 0 || def->interval_max_ms != 0 || !clip.loop) ? 1 : 0;
    ch->next_event_ms = 0;
}

static int divoom_buddy_is_music_only_mode(const char *mode_id)
{
    if (!mode_id)
    {
        return 0;
    }

    return strcmp(mode_id, "scene_music") == 0 ||
        strcmp(mode_id, "music") == 0 ||
        strcmp(mode_id, "singing") == 0;
}

static int divoom_buddy_state_is_prone(const divoom_buddy_state_cfg_t *state)
{
    divoom_buddy_clip_cfg_t clip;

    if (!state || state->base_action_opt_count <= 0)
    {
        return -1;
    }
    if (divoom_buddy_config_get_clip(state->base_action_opts[0].clip, &clip) != 0)
    {
        return -1;
    }
    return strcmp(clip.pose_id, "prone_pose") == 0 ? 1 : 0;
}

/*
 * 姿态切换复用同一套全帧资源：坐到趴正播，趴到坐倒播。
 * 目标姿态已经在底层准备好，序列结束后只需恢复分层图像。
 */
static void divoom_buddy_start_pose_transition(divoom_buddy_t *ph, uint8 reverse)
{
    divoom_buddy_channel_runtime_t *ch;
    divoom_buddy_clip_cfg_t clip;

    if (!ph || divoom_buddy_find_active_full_frame_layer(ph) >= 0 ||
        divoom_buddy_config_get_clip("act_sit_to_lie_down", &clip) != 0)
    {
        return;
    }
    ch = divoom_buddy_find_free_pose_rest_channel(ph);
    if (!ch)
    {
        return;
    }

    divoom_buddy_channel_start(ph, ch, &clip, 1.0f, 0);
    if (!ch->active)
    {
        return;
    }
    ch->session_once = 1;
    ch->micro_idx = -1;
    if (reverse)
    {
        ch->frame_step = -1;
        ch->frame_idx = ch->clip.frame_count - 1;
        ch->frame_acc_ms = 0;
        divoom_buddy_show_layer_frame(ph, ch->layer_idx, &ch->clip, ch->frame_idx);
    }
#if defined(DIVOOM_SIMULATOR)
    if (ph->test_coverage && ch->layer_idx >= 0 &&
        !lv_obj_has_flag(ph->layer_img[ch->layer_idx], LV_OBJ_FLAG_HIDDEN))
    {
        ph->whitebox_coverage_mask |= reverse ? DIVOOM_BUDDY_WB_PRONE_TO_SIT :
            DIVOOM_BUDDY_WB_SIT_TO_PRONE;
    }
#endif
    divoom_print("[buddy] pose_transition=%s target=%s\n",
        reverse ? "prone_to_sit" : "sit_to_prone", ph->mode_id);
}

static void divoom_buddy_apply_mode(divoom_buddy_t *ph)
{
    int i;
    int old_prone;
    int next_prone;
    uint8 posture_changed;
    divoom_buddy_state_cfg_t next_state;

    if (!ph || ph->paused || ph->pending_mode_id[0] == '\0')
    {
        return;
    }

    if (divoom_buddy_ensure_layers(ph) != 0)
    {
        divoom_print("[buddy] set_mode failed: layers not ready\n");
        ph->pending_mode = 0;
        return;
    }

    memset(&next_state, 0, sizeof(next_state));
    if (divoom_buddy_config_get_state(ph->pending_mode_id, &next_state) != 0)
    {
        divoom_print("[buddy] set_mode failed: %s\n", ph->pending_mode_id);
        ph->pending_mode = 0;
        return;
    }

    old_prone = divoom_buddy_state_is_prone(&ph->state);
    next_prone = divoom_buddy_state_is_prone(&next_state);
    posture_changed = strcmp(ph->cfg_dir, DIVOOM_BUDDY_CFG_DIR_DEFAULT) == 0 &&
        ph->mode_id[0] != '\0' && old_prone >= 0 && next_prone >= 0 &&
        old_prone != next_prone && !divoom_buddy_is_music_only_mode(ph->mode_id) &&
        !divoom_buddy_is_music_only_mode(ph->pending_mode_id);

    memset(ph->channels, 0, sizeof(ph->channels));
    memcpy(&ph->state, &next_state, sizeof(ph->state));
    strncpy(ph->mode_id, ph->pending_mode_id, DIVOOM_BUDDY_ID_MAX - 1);
    ph->mode_id[DIVOOM_BUDDY_ID_MAX - 1] = '\0';
    ph->pending_mode = 0;
    ph->pending_mode_id[0] = '\0';

    divoom_buddy_apply_default_layer_pos(ph);
    /* ato 音乐全身帧位于 base 通道；旧 Buddy 音乐位于 micro，不能共用跳过规则。 */
    if (!divoom_buddy_is_music_only_mode(ph->mode_id) || strcmp(ph->cfg_dir, "ato/") == 0)
    {
        divoom_buddy_start_base_channel(ph);
        divoom_buddy_start_expression_channel(ph);
    }

    for (i = 0; i < ph->state.micro_action_count && i < DIVOOM_BUDDY_MICRO_CHANNEL_MAX; i++)
    {
        if (ph->state.micro_actions[i].interval_min_ms == 0 && ph->state.micro_actions[i].interval_max_ms == 0)
        {
            divoom_buddy_start_micro_channel(ph, i);
        }
        else
        {
            divoom_buddy_prepare_micro_rest_frame(ph, i);
        }
    }

    divoom_buddy_prepare_pose_rest_layers(ph);
    divoom_buddy_clear_inactive_layers(ph);
    if (strcmp(ph->cfg_dir, "ato/") == 0)
    {
        divoom_print("ato3499 buddy mode=%s base_active=%u base=%s expr_active=%u expr=%s\n",
            ph->mode_id, ph->channels[DIVOOM_BUDDY_CH_BASE].active,
            ph->channels[DIVOOM_BUDDY_CH_BASE].clip.id,
            ph->channels[DIVOOM_BUDDY_CH_EXPR].active,
            ph->channels[DIVOOM_BUDDY_CH_EXPR].clip.id);
    }
    if (posture_changed)
    {
        divoom_buddy_start_pose_transition(ph, old_prone ? 1 : 0);
    }
}

/* 表盘嵌入：持 LVGL 锁时立即 apply pending mode（由 dial_menu_add_buddy 调用） */
void divoom_buddy_flush_pending_mode(lv_obj_t *obj)
{
    divoom_buddy_t *ph;

    ph = divoom_buddy_get_obj(obj);
    if (!ph || ph->paused || !ph->pending_mode)
    {
        return;
    }

    divoom_buddy_apply_mode(ph);
}

/*
 * 场景动作使用独立通道，不修改持续表情状态。新场景动作优先于正在播放的
 * 随机 full_frame；被打断的随机通道按原规则恢复和重新排期。
 */
int divoom_buddy_trigger_action(lv_obj_t *obj, const char *clip_id,
    uint32 sequence)
{
    divoom_buddy_t *ph;
    divoom_buddy_channel_runtime_t *action_ch;
    divoom_buddy_clip_cfg_t clip;
    int i;

    ph = divoom_buddy_get_obj(obj);
    if (ph == NULL || ph->paused || clip_id == NULL || clip_id[0] == '\0' ||
        sequence == 0 || sequence == ph->last_action_sequence)
    {
        return 0;
    }
    if (ph->pending_mode)
    {
        divoom_buddy_apply_mode(ph);
    }
    if (divoom_buddy_is_music_only_mode(ph->mode_id))
    {
        /* scene_music 独占全帧层，任何迟到的一次性动作都不得抢占唱歌。 */
        divoom_print("[buddy_action] suppress in music clip=%s sequence=%u\n",
            clip_id, (unsigned)sequence);
        return 0;
    }
    if (divoom_buddy_ensure_layers(ph) != 0 ||
        divoom_buddy_config_get_clip(clip_id, &clip) != 0)
    {
        divoom_print("[buddy_action] load failed cfg=%s clip=%s sequence=%u\n",
            ph->cfg_dir, clip_id, (unsigned)sequence);
        return 0;
    }
    if (clip.loop)
    {
        /* 持续动作应由状态配置管理，外部一次性通道不接管 loop 资源。 */
        divoom_print("[buddy_action] reject loop clip=%s sequence=%u\n",
            clip_id, (unsigned)sequence);
        return 0;
    }

    for (i = 0; i < DIVOOM_BUDDY_CH_COUNT; i++)
    {
        divoom_buddy_channel_runtime_t *ch = &ph->channels[i];

        if (i == DIVOOM_BUDDY_CH_ACTION || !ch->active ||
            !divoom_buddy_is_full_frame_layer(ph, ch->layer_idx))
        {
            continue;
        }
        divoom_buddy_micro_finish_session(ph, ch);
    }

    action_ch = &ph->channels[DIVOOM_BUDDY_CH_ACTION];
    if (action_ch->active || action_ch->resting)
    {
        divoom_buddy_micro_finish_session(ph, action_ch);
    }
    divoom_buddy_channel_start(ph, action_ch, &clip, 1.0f, 0);
    if (!action_ch->active)
    {
        return 0;
    }
    action_ch->session_once = 1;
    action_ch->micro_idx = -1;
    action_ch->next_event_ms = 0;
    ph->last_action_sequence = sequence;
#if defined(DIVOOM_SIMULATOR)
    if (action_ch->layer_idx >= 0 && ph->layer_img[action_ch->layer_idx] != NULL &&
        !lv_obj_has_flag(ph->layer_img[action_ch->layer_idx], LV_OBJ_FLAG_HIDDEN))
    {
        ph->whitebox_action_start_count++;
        strncpy(ph->whitebox_last_action_clip, clip_id,
            sizeof(ph->whitebox_last_action_clip) - 1);
        ph->whitebox_last_action_clip[
            sizeof(ph->whitebox_last_action_clip) - 1] = '\0';
    }
#endif
    divoom_print("[buddy_action] start cfg=%s mode=%s clip=%s sequence=%u\n",
        ph->cfg_dir, ph->mode_id, clip_id, (unsigned)sequence);
    return 1;
}

/*
 * 懒加载图层子对象：禁止在 constructor 内调用 lv_obj_set_size / lv_img_create，
 * LVGL 对象尚未完成 init 时触发布局会卡死在 lv_obj_set_size（表盘 disp497 联调已复现）。
 */
static int divoom_buddy_ensure_layers(divoom_buddy_t *ph)
{
    int i;
    lv_obj_t *img;
    lv_obj_t *root;

    if (!ph || ph->paused)
    {
        return -1;
    }

    if (ph->layers_ready)
    {
        return 0;
    }

    if (divoom_buddy_config_load_layers(&ph->layers) != 0)
    {
        return -1;
    }
    if (ph->layers.layer_count > DIVOOM_BUDDY_MAX_LAYERS)
    {
        divoom_print("[buddy] clamp layer_count %d to %d\n", ph->layers.layer_count, DIVOOM_BUDDY_MAX_LAYERS);
        ph->layers.layer_count = DIVOOM_BUDDY_MAX_LAYERS;
    }

    root = (lv_obj_t *)ph;
    /*
     * root 的 x/y/w/h 来自表盘元素配置，不能 remove_style_all(root)，否则会清掉坐标样式。
     * 子图层 img 仍然 remove_style_all，避免图片控件默认样式影响叠图。
     */
    lv_obj_set_style_bg_opa(root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_pad_all(root, 0, 0);

    for (i = 0; i < ph->layers.layer_count; i++)
    {
        img = lv_img_create(root);
        ph->layer_img[i] = img;
        lv_obj_remove_style_all(img);
        lv_obj_set_pos(img, (lv_coord_t)ph->layers.layers[i].x, (lv_coord_t)ph->layers.layers[i].y);
    }

    ph->layers_ready = 1;
    divoom_buddy_print_diag(ph, "ensure_layers", lv_tick_get());
    return 0;
}

static void divoom_buddy_tick_cb(lv_timer_t *timer)
{
    divoom_buddy_t *ph;
    uint32_t now;
    int i;
    int delay_ms;
    divoom_buddy_channel_runtime_t *ch;

    ph = (divoom_buddy_t *)lv_timer_get_user_data(timer);
    if (!ph || ph->paused)
    {
        return;
    }

    ph->diag_tick_count++;
    if (ph->pending_mode)
    {
        divoom_buddy_apply_mode(ph);
    }

    now = lv_tick_get();
    if (ph->diag_last_ms == 0 || now - ph->diag_last_ms >= 1000)
    {
        divoom_buddy_print_diag(ph, "tick", now);
    }

    divoom_buddy_test_mode_cycle(ph, now);

    for (i = 0; i < ph->state.micro_action_count && i < DIVOOM_BUDDY_MICRO_CHANNEL_MAX; i++)
    {
        int ch_id = DIVOOM_BUDDY_CH_MICRO0 + i;
        divoom_buddy_channel_runtime_t *micro_ch;

        if (ch_id >= DIVOOM_BUDDY_CH_COUNT)
        {
            break;
        }

        micro_ch = &ph->channels[ch_id];
        if (micro_ch->active)
        {
            continue;
        }

        if (micro_ch->next_event_ms != 0 && now >= micro_ch->next_event_ms)
        {
            if ((float)divoom_buddy_rand_u32(ph) / 4294967295.0f <= ph->state.micro_actions[i].chance)
            {
                divoom_buddy_start_micro_channel(ph, i);
                if (micro_ch->active)
                {
                    continue;
                }
            }

            divoom_buddy_channel_schedule_refresh(ph, micro_ch,
                ph->state.micro_actions[i].interval_min_ms,
                ph->state.micro_actions[i].interval_max_ms);
        }
    }

    for (i = 0; i < DIVOOM_BUDDY_CH_COUNT; i++)
    {
        ch = &ph->channels[i];
        if (!ch->active)
        {
            continue;
        }

        if (ch->next_event_ms != 0 && now >= ch->next_event_ms)
        {
            if (i == DIVOOM_BUDDY_CH_EXPR)
            {
                divoom_buddy_start_expression_channel(ph);
            }
            else if (i == DIVOOM_BUDDY_CH_BASE)
            {
                divoom_buddy_start_base_channel(ph);
            }
            else if (i >= DIVOOM_BUDDY_CH_MICRO0)
            {
                /* 已在上方独立处理 micro 触发 */
            }
        }

        if (!ch->active)
        {
            continue;
        }

        if (strcmp(ph->cfg_dir, "ato/") == 0 &&
            ((ph->ato_listen_phase && strcmp(ch->clip.layer_id, "head") == 0) ||
             (strcmp(ph->mode_id, "confused") == 0 && strcmp(ch->clip.layer_id, "decoration") == 0)))
        {
            divoom_buddy_channel_runtime_t *master = &ph->channels[DIVOOM_BUDDY_CH_EXPR];
            const char *peer = NULL;
            if (strcmp(ph->mode_id, "confused") == 0)
            {
                peer = strstr(master->clip.id, "p02") ? "decoration_confused_p02" : "decoration_confused_p01";
                if (strcmp(ch->clip.id, peer) != 0)
                {
                    divoom_buddy_config_get_clip(peer, &ch->clip);
                }
            }
            ch->frame_idx = master->frame_idx;
            ch->frame_acc_ms = master->frame_acc_ms;
            divoom_buddy_show_layer_frame(ph, ch->layer_idx, &ch->clip, ch->frame_idx);
            continue;
        }
        delay_ms = divoom_buddy_clip_frame_delay_ms(&ch->clip, ch->frame_idx, ch->speed);
        ch->frame_acc_ms += DIVOOM_BUDDY_TICK_MS;
        if (ch->frame_acc_ms >= (uint32)delay_ms)
        {
            divoom_buddy_channel_advance_frame(ph, ch);
        }
    }
}

static void divoom_buddy_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj)
{
    divoom_buddy_t *ph;

    (void)class_p;
    ph = divoom_buddy_get_obj(obj);
    if (!ph)
    {
        return;
    }

    /*
     * 禁止 memset(ph) ！LVGL 在 lv_obj_class_create_obj 已对整块对象 lv_memzero，
     * 且基类 lv_obj_constructor + lv_theme_apply 已写好 class_p/parent/coords/flags/styles。
     * 这里若再清零会损坏基类对象，导致随后 lv_obj_refresh_style 卡死。
     * 只设置本控件自己需要非零默认值的字段，其余字段已是 0。
     */
    strncpy(ph->cfg_dir, DIVOOM_BUDDY_CFG_DIR_DEFAULT, sizeof(ph->cfg_dir) - 1);
    ph->cfg_dir[sizeof(ph->cfg_dir) - 1] = '\0';
    ph->rand_seed = (uint32)lv_tick_get() ^ 0xB0DD0001u;
    divoom_buddy_init_test_flags(ph);
    divoom_buddy_config_retain();
    /* 图层/子 img 在 ensure_layers 懒加载；帧动画 timer 在控件内维护 */
    ph->tick_timer = lv_timer_create(divoom_buddy_tick_cb, DIVOOM_BUDDY_TICK_MS, ph);
    divoom_buddy_print_diag(ph, "create", lv_tick_get());
}

static void divoom_buddy_destructor(const lv_obj_class_t *class_p, lv_obj_t *obj)
{
    divoom_buddy_t *ph;

    (void)class_p;
    ph = divoom_buddy_get_obj(obj);
    if (!ph)
    {
        return;
    }

    uint8 was_active = !ph->paused;
    ph->paused = 1;
    if (ph->tick_timer)
    {
        lv_timer_del(ph->tick_timer);
        ph->tick_timer = NULL;
    }

    divoom_buddy_release_all_layer_buf(ph);
    if (was_active)
    {
        divoom_buddy_config_release();
    }
    if (ph->ato_resource_lease)
    {
        divoom_clock_ato_resources_release();
        ph->ato_resource_lease = 0;
    }
    divoom_buddy_print_diag(ph, "destroy", lv_tick_get());
}

void divoom_buddy_pause(lv_obj_t *obj)
{
    divoom_buddy_t *ph;

    ph = divoom_buddy_get_obj(obj);
    if (!ph)
    {
        return;
    }

    uint8 was_active = !ph->paused;
    ph->paused = 1;
    ph->pending_mode = 0;
    ph->pending_mode_id[0] = '\0';
    memset(ph->channels, 0, sizeof(ph->channels));

    if (ph->tick_timer)
    {
        lv_timer_del(ph->tick_timer);
        ph->tick_timer = NULL;
    }

    if (was_active)
    {
        divoom_buddy_config_release();
    }
    divoom_buddy_print_diag(ph, "pause", lv_tick_get());
}

void divoom_buddy_resume(lv_obj_t *obj)
{
    divoom_buddy_t *ph;

    ph = divoom_buddy_get_obj(obj);
    if (!ph)
    {
        return;
    }
    if (!ph->paused && ph->tick_timer != NULL)
    {
        return;
    }

    if (ph->paused)
    {
        divoom_buddy_config_retain();
    }
    ph->paused = 0;
    if (ph->tick_timer == NULL)
    {
        ph->tick_timer = lv_timer_create(divoom_buddy_tick_cb,
            DIVOOM_BUDDY_TICK_MS, ph);
    }

    /* pause 会清空所有运行通道；复用对象时必须从原状态重建，不能只恢复 timer。 */
    if (ph->mode_id[0] != '\0')
    {
        strncpy(ph->pending_mode_id, ph->mode_id,
            sizeof(ph->pending_mode_id) - 1);
        ph->pending_mode_id[sizeof(ph->pending_mode_id) - 1] = '\0';
        ph->pending_mode = 1;
    }
    divoom_buddy_print_diag(ph, "resume", lv_tick_get());
}

lv_obj_t *divoom_buddy_create(lv_obj_t *parent)
{
    lv_obj_t *obj;

    obj = lv_obj_class_create_obj(DIVOOM_BUDDY_CLASS, parent);
    if (!obj)
    {
        return NULL;
    }
    lv_obj_class_init_obj(obj);
    return obj;
}

void divoom_buddy_set_config_dir(lv_obj_t *obj, const char *cfg_dir)
{
    divoom_buddy_t *ph;
    const char *next_dir;

    ph = divoom_buddy_get_obj(obj);
    if (!ph)
    {
        return;
    }

    next_dir = cfg_dir;
    if (!next_dir || next_dir[0] == '\0')
    {
        next_dir = DIVOOM_BUDDY_CFG_DIR_DEFAULT;
    }

    if (!strcmp(next_dir, "ato/") && !ph->ato_resource_lease)
    {
        if (!divoom_clock_ato_resources_retain())
        {
            divoom_print("ato3499 buddy resource_not_ready\n");
            return;
        }
        ph->ato_resource_lease = 1;
        divoom_buddy_config_invalidate();
    }
    if (strcmp(ph->cfg_dir, next_dir) != 0)
    {
        divoom_buddy_reset_layers_for_config(ph);
        if (strcmp(next_dir, "ato/") && ph->ato_resource_lease)
        {
            divoom_clock_ato_resources_release();
            ph->ato_resource_lease = 0;
        }
        ph->last_action_sequence = 0;
        ph->ato_business_mode[0] = '\0';
        ph->ato_listen_phase = 0;
#if defined(DIVOOM_SIMULATOR)
        ph->whitebox_last_action_clip[0] = '\0';
#endif
    }

    strncpy(ph->cfg_dir, next_dir, sizeof(ph->cfg_dir) - 1);
    ph->cfg_dir[sizeof(ph->cfg_dir) - 1] = '\0';
    divoom_buddy_config_set_dir(next_dir);
}

void divoom_buddy_set_mode(lv_obj_t *obj, const char *mode_id)
{
    divoom_buddy_t *ph;

    ph = divoom_buddy_get_obj(obj);
    if (!ph || !mode_id || mode_id[0] == '\0')
    {
        return;
    }
#if defined(DIVOOM_SIMULATOR)
    if (ph->test_coverage && ph->whitebox_forced_mode_id[0] != '\0' &&
        strcmp(ph->whitebox_forced_mode_id, mode_id) != 0)
    {
        return;
    }
#endif

    if (strcmp(ph->cfg_dir, "ato/") == 0)
    {
        if (strcmp(ph->ato_business_mode, mode_id) == 0)
        {
            return;
        }
        snprintf(ph->ato_business_mode, sizeof(ph->ato_business_mode), "%s", mode_id);
        if (strcmp(mode_id, "listening") == 0)
        {
            ph->ato_listen_phase = 1;
            mode_id = "listening_enter";
        }
        else if (ph->ato_listen_phase &&
            (strcmp(mode_id, "thinking") == 0 || strcmp(mode_id, "idle") == 0 ||
             strcmp(mode_id, "hold_limit") == 0 || strcmp(mode_id, "not_clear") == 0 ||
             strcmp(mode_id, "confused") == 0))
        {
            /* 提前松开仍完成 enter + 至少一轮120ms hold + exit；目标保留真实业务状态。 */
            return;
        }
        else
        {
            /* 打断、失败和页面取消优先，不能被旧收音动画拖住。 */
            ph->ato_listen_phase = 0;
        }
    }
    /* 文本A/B刷新和同场景重绘不能重启动作计时。 */
    if ((!ph->pending_mode && strcmp(ph->mode_id, mode_id) == 0) ||
        (ph->pending_mode && strcmp(ph->pending_mode_id, mode_id) == 0))
    {
        return;
    }

    strncpy(ph->pending_mode_id, mode_id, DIVOOM_BUDDY_ID_MAX - 1);
    ph->pending_mode_id[DIVOOM_BUDDY_ID_MAX - 1] = '\0';
    ph->pending_mode = 1;
}

const char *divoom_buddy_get_mode(lv_obj_t *obj)
{
    divoom_buddy_t *ph;

    ph = divoom_buddy_get_obj(obj);
    if (!ph)
    {
        return "";
    }

    return ph->mode_id;
}

#if defined(DIVOOM_SIMULATOR)
int divoom_buddy_whitebox_get_image_buffer_count(lv_obj_t *obj)
{
    divoom_buddy_t *ph;
    int count;
    int i;

    ph = divoom_buddy_get_obj(obj);
    if (ph == NULL)
    {
        return 0;
    }
    count = 0;
    for (i = 0; i < DIVOOM_BUDDY_MAX_LAYERS; i++)
    {
        if (ph->layer_decode[i].out_buf != NULL)
        {
            count++;
        }
    }
    return count;
}

int divoom_buddy_whitebox_get_timer_count(lv_obj_t *obj)
{
    divoom_buddy_t *ph;

    ph = divoom_buddy_get_obj(obj);
    return ph != NULL && ph->tick_timer != NULL ? 1 : 0;
}

int divoom_buddy_whitebox_get_active_channel_count(lv_obj_t *obj)
{
    divoom_buddy_t *ph;
    int count;
    int i;

    ph = divoom_buddy_get_obj(obj);
    if (ph == NULL)
    {
        return 0;
    }
    count = 0;
    for (i = 0; i < DIVOOM_BUDDY_CH_COUNT; i++)
    {
        if (ph->channels[i].active)
        {
            count++;
        }
    }
    return count;
}

uint32 divoom_buddy_whitebox_get_decode_fail_count(lv_obj_t *obj)
{
    divoom_buddy_t *ph;

    ph = divoom_buddy_get_obj(obj);
    return ph != NULL ? ph->diag_decode_fail_count : 0;
}

void divoom_buddy_whitebox_reset_coverage(lv_obj_t *obj)
{
    divoom_buddy_t *ph;

    ph = divoom_buddy_get_obj(obj);
    if (ph != NULL)
    {
        ph->whitebox_coverage_mask = 0;
        ph->whitebox_action_start_count = 0;
        ph->whitebox_last_action_clip[0] = '\0';
        ph->whitebox_forced_mode_id[0] = '\0';
    }
}

void divoom_buddy_whitebox_set_forced_state(lv_obj_t *obj, const char *state_id)
{
    divoom_buddy_t *ph;

    ph = divoom_buddy_get_obj(obj);
    if (ph == NULL || state_id == NULL || state_id[0] == '\0')
    {
        return;
    }
    strncpy(ph->whitebox_forced_mode_id, state_id,
        sizeof(ph->whitebox_forced_mode_id) - 1);
    ph->whitebox_forced_mode_id[sizeof(ph->whitebox_forced_mode_id) - 1] = '\0';
    /* 即使与当前状态相同也重新创建通道，保证 reset 后从真实首个 clip 开始计数。 */
    strncpy(ph->pending_mode_id, state_id, sizeof(ph->pending_mode_id) - 1);
    ph->pending_mode_id[sizeof(ph->pending_mode_id) - 1] = '\0';
    ph->pending_mode = 1;
}

uint32 divoom_buddy_whitebox_get_coverage(lv_obj_t *obj)
{
    divoom_buddy_t *ph;

    ph = divoom_buddy_get_obj(obj);
    return ph != NULL ? ph->whitebox_coverage_mask : 0;
}

uint32 divoom_buddy_whitebox_get_action_start_count(lv_obj_t *obj)
{
    divoom_buddy_t *ph;

    ph = divoom_buddy_get_obj(obj);
    return ph != NULL ? ph->whitebox_action_start_count : 0;
}

const char *divoom_buddy_whitebox_get_last_action_clip(lv_obj_t *obj)
{
    divoom_buddy_t *ph;

    ph = divoom_buddy_get_obj(obj);
    return ph != NULL ? ph->whitebox_last_action_clip : "";
}
#endif
